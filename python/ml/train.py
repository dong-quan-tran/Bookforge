from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

import pandas as pd
from bookforge_py.dataset import (
    build_training_dataset,
    chronological_split,
    walk_forward_splits,
)
from sklearn.metrics import (
    accuracy_score,
    classification_report,
    mean_absolute_error,
    mean_squared_error,
    r2_score,
)
from xgboost import XGBClassifier, XGBRegressor


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Train Bookforge ML baselines.")
    parser.add_argument("--features-csv", required=True, help="Path to exported feature CSV")
    parser.add_argument(
        "--label-type",
        choices=["classification", "regression"],
        default="classification",
    )
    parser.add_argument("--horizon-events", type=int, default=50)
    parser.add_argument(
        "--purge-events",
        type=int,
        default=None,
        help="Rows purged between train and test windows; defaults to horizon-events.",
    )
    parser.add_argument("--up-threshold", type=float, default=0.0)
    parser.add_argument("--down-threshold", type=float, default=0.0)
    parser.add_argument("--train-fraction", type=float, default=0.8)
    parser.add_argument(
        "--validation",
        choices=["holdout", "walk_forward"],
        default="holdout",
    )
    parser.add_argument("--wf-initial-train-size", type=int, default=50000)
    parser.add_argument("--wf-test-size", type=int, default=10000)
    parser.add_argument("--wf-step-size", type=int, default=10000)
    parser.add_argument("--wf-max-folds", type=int, default=5)
    parser.add_argument("--output-dir", default="artifacts/ml_baseline")
    parser.add_argument("--enable-mlflow", action="store_true")
    parser.add_argument("--mlflow-experiment", default="bookforge")
    parser.add_argument("--enable-shap", action="store_true")
    parser.add_argument("--shap-sample-size", type=int, default=2000)
    return parser


def _train_classifier(X_train: pd.DataFrame, y_train: pd.Series) -> XGBClassifier:
    n_classes = int(y_train.nunique())
    if n_classes < 2:
        raise ValueError("Classification training requires at least two target classes")

    if n_classes == 2:
        model = XGBClassifier(
            objective="binary:logistic",
            n_estimators=200,
            max_depth=6,
            learning_rate=0.05,
            subsample=0.8,
            colsample_bytree=0.8,
            reg_lambda=1.0,
            random_state=42,
            tree_method="hist",
            eval_metric="logloss",
        )
    else:
        model = XGBClassifier(
            objective="multi:softprob",
            num_class=n_classes,
            n_estimators=200,
            max_depth=6,
            learning_rate=0.05,
            subsample=0.8,
            colsample_bytree=0.8,
            reg_lambda=1.0,
            random_state=42,
            tree_method="hist",
            eval_metric="mlogloss",
        )

    model.fit(X_train, y_train)
    return model


def _train_regressor(X_train: pd.DataFrame, y_train: pd.Series) -> XGBRegressor:
    model = XGBRegressor(
        objective="reg:squarederror",
        n_estimators=200,
        max_depth=6,
        learning_rate=0.05,
        subsample=0.8,
        colsample_bytree=0.8,
        reg_lambda=1.0,
        random_state=42,
        tree_method="hist",
        eval_metric="rmse",
    )
    model.fit(X_train, y_train)
    return model


def _compute_classification_metrics(y_true: pd.Series, y_pred: Any) -> dict[str, Any]:
    return {
        "accuracy": float(accuracy_score(y_true, y_pred)),
        "classification_report": classification_report(
            y_true,
            y_pred,
            output_dict=True,
            zero_division=0,
        ),
    }


def _compute_regression_metrics(y_true: pd.Series, y_pred: Any) -> dict[str, float]:
    return {
        "mae": float(mean_absolute_error(y_true, y_pred)),
        "rmse": float(mean_squared_error(y_true, y_pred) ** 0.5),
        "r2": float(r2_score(y_true, y_pred)),
    }


def _aggregate_fold_metrics(
    fold_metrics: list[dict[str, Any]],
    label_type: str,
) -> dict[str, Any]:
    if label_type == "classification":
        accuracies = [metric["accuracy"] for metric in fold_metrics]
        return {
            "mean_accuracy": float(sum(accuracies) / len(accuracies)),
            "min_accuracy": float(min(accuracies)),
            "max_accuracy": float(max(accuracies)),
            "n_folds": len(fold_metrics),
        }

    maes = [metric["mae"] for metric in fold_metrics]
    rmses = [metric["rmse"] for metric in fold_metrics]
    r2s = [metric["r2"] for metric in fold_metrics]
    return {
        "mean_mae": float(sum(maes) / len(maes)),
        "mean_rmse": float(sum(rmses) / len(rmses)),
        "mean_r2": float(sum(r2s) / len(r2s)),
        "n_folds": len(fold_metrics),
    }


def _train_one(X_train: pd.DataFrame, y_train: pd.Series, label_type: str) -> Any:
    if label_type == "classification":
        return _train_classifier(X_train, y_train)
    return _train_regressor(X_train, y_train)


def _predict_and_score(
    model: Any,
    X_test: pd.DataFrame,
    y_test: pd.Series,
    label_type: str,
) -> dict[str, Any]:
    y_pred = model.predict(X_test)
    if label_type == "classification":
        return _compute_classification_metrics(y_test, y_pred)
    return _compute_regression_metrics(y_test, y_pred)


def _save_feature_importance(model: Any, feature_names: list[str], output_dir: Path) -> Path:
    importance = getattr(model, "feature_importances_", None)
    if importance is None:
        return output_dir / "feature_importance.csv"

    importance_frame = pd.DataFrame(
        {
            "feature": feature_names,
            "importance": importance,
        }
    ).sort_values("importance", ascending=False)

    path = output_dir / "feature_importance.csv"
    importance_frame.to_csv(path, index=False)
    return path


def _save_shap_values(
    model: Any,
    X_reference: pd.DataFrame,
    output_dir: Path,
    sample_size: int,
) -> str | None:
    try:
        import shap
    except ImportError:
        return None

    sample = X_reference.head(sample_size).copy()
    explainer = shap.TreeExplainer(model)
    shap_values = explainer.shap_values(sample)

    values = shap_values[0] if isinstance(shap_values, list) else shap_values
    shap_frame = pd.DataFrame(values, columns=sample.columns)
    mean_abs = shap_frame.abs().mean().sort_values(ascending=False)

    output = pd.DataFrame(
        {
            "feature": mean_abs.index,
            "mean_abs_shap": mean_abs.values,
        }
    )

    path = output_dir / "shap_importance.csv"
    output.to_csv(path, index=False)
    return path.as_posix()


def _maybe_start_mlflow(
    args: argparse.Namespace,
    dataset_rows: int,
    purge_events: int,
) -> Any | None:
    if not args.enable_mlflow:
        return None

    try:
        import mlflow
    except ImportError:
        return None

    mlflow.set_experiment(args.mlflow_experiment)
    mlflow.start_run()

    mlflow.log_params(
        {
            "features_csv": args.features_csv,
            "label_type": args.label_type,
            "horizon_events": args.horizon_events,
            "purge_events": purge_events,
            "up_threshold": args.up_threshold,
            "down_threshold": args.down_threshold,
            "validation": args.validation,
            "train_fraction": args.train_fraction,
            "wf_initial_train_size": args.wf_initial_train_size,
            "wf_test_size": args.wf_test_size,
            "wf_step_size": args.wf_step_size,
            "wf_max_folds": args.wf_max_folds,
            "dataset_rows": dataset_rows,
        }
    )

    return mlflow


def _class_balance(y: pd.Series) -> dict[str, int]:
    return {str(label): int(count) for label, count in y.value_counts().sort_index().items()}


def main() -> None:
    parser = _build_parser()
    args = parser.parse_args()

    if args.horizon_events < 1:
        raise ValueError("--horizon-events must be positive")

    purge_events = args.horizon_events if args.purge_events is None else args.purge_events
    if purge_events < 0:
        raise ValueError("--purge-events must be non-negative")

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    dataset = build_training_dataset(
        args.features_csv,
        label_type=args.label_type,
        horizon_events=args.horizon_events,
        up_threshold=args.up_threshold,
        down_threshold=args.down_threshold,
    )

    mlflow = _maybe_start_mlflow(args, len(dataset.X), purge_events)

    try:
        metrics: dict[str, Any]
        model: Any

        if args.validation == "holdout":
            split = chronological_split(
                dataset,
                train_fraction=args.train_fraction,
                purge_events=purge_events,
            )

            model = _train_one(split.X_train, split.y_train, args.label_type)

            metrics = {
                "task": args.label_type,
                "validation": "holdout",
                "dataset_rows": int(len(dataset.X)),
                "split_index": split.split_index,
                "purge_events": split.purge_events,
                "n_train": int(len(split.X_train)),
                "n_test": int(len(split.X_test)),
            }

            if args.label_type == "classification":
                metrics["train_class_balance"] = _class_balance(split.y_train)
                metrics["test_class_balance"] = _class_balance(split.y_test)

            metrics.update(
                _predict_and_score(
                    model,
                    split.X_test,
                    split.y_test,
                    args.label_type,
                )
            )
        else:
            folds = walk_forward_splits(
                dataset,
                initial_train_size=args.wf_initial_train_size,
                test_size=args.wf_test_size,
                step_size=args.wf_step_size,
                max_folds=args.wf_max_folds,
                purge_events=purge_events,
            )

            fold_metrics: list[dict[str, Any]] = []
            model = None

            for fold in folds:
                model = _train_one(fold.X_train, fold.y_train, args.label_type)
                scored = _predict_and_score(
                    model,
                    fold.X_test,
                    fold.y_test,
                    args.label_type,
                )

                scored.update(
                    {
                        "fold_index": fold.fold_index,
                        "n_train": int(len(fold.X_train)),
                        "n_test": int(len(fold.X_test)),
                        "train_end": fold.train_end,
                        "effective_train_end": fold.effective_train_end,
                        "test_start": fold.test_start,
                        "test_end": fold.test_end,
                        "purge_events": fold.purge_events,
                    }
                )

                if args.label_type == "classification":
                    scored["train_class_balance"] = _class_balance(fold.y_train)
                    scored["test_class_balance"] = _class_balance(fold.y_test)

                fold_metrics.append(scored)

                if mlflow is not None:
                    for key, value in scored.items():
                        if isinstance(value, int | float):
                            mlflow.log_metric(f"fold_{fold.fold_index}_{key}", float(value))

            if model is None:
                raise RuntimeError("Walk-forward validation did not train a model")

            metrics = {
                "task": args.label_type,
                "validation": "walk_forward",
                "dataset_rows": int(len(dataset.X)),
                "purge_events": purge_events,
                "folds": fold_metrics,
                "aggregate": _aggregate_fold_metrics(fold_metrics, args.label_type),
            }

        model_path = output_dir / "xgboost_baseline.json"
        metrics_path = output_dir / "metrics.json"
        feature_columns_path = output_dir / "feature_columns.json"

        model.save_model(model_path.as_posix())

        with metrics_path.open("w", encoding="utf-8") as file:
            json.dump(metrics, file, indent=2)

        with feature_columns_path.open("w", encoding="utf-8") as file:
            json.dump(list(dataset.X.columns), file, indent=2)

        importance_path = _save_feature_importance(model, list(dataset.X.columns), output_dir)

        shap_path = None
        if args.enable_shap:
            shap_path = _save_shap_values(
                model,
                dataset.X,
                output_dir,
                args.shap_sample_size,
            )

        if mlflow is not None:
            aggregate = metrics.get("aggregate", {})
            for key, value in aggregate.items():
                if isinstance(value, int | float):
                    mlflow.log_metric(key, float(value))

            if args.validation == "holdout":
                for key, value in metrics.items():
                    if isinstance(value, int | float):
                        mlflow.log_metric(key, float(value))

            mlflow.log_artifact(model_path.as_posix())
            mlflow.log_artifact(metrics_path.as_posix())
            mlflow.log_artifact(feature_columns_path.as_posix())

            if importance_path.exists():
                mlflow.log_artifact(importance_path.as_posix())

            if shap_path is not None:
                mlflow.log_artifact(shap_path)

        print(json.dumps(metrics, indent=2))
    finally:
        if mlflow is not None:
            mlflow.end_run()


if __name__ == "__main__":
    main()
