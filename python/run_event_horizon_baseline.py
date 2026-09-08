from __future__ import annotations

import json
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.ensemble import HistGradientBoostingClassifier
from sklearn.impute import SimpleImputer
from sklearn.metrics import (
    accuracy_score,
    balanced_accuracy_score,
    classification_report,
    confusion_matrix,
    f1_score,
    precision_recall_fscore_support,
)
from sklearn.pipeline import Pipeline

INPUT_PATH = Path("output/features_btc_enriched.csv")
OUTPUT_DIR = Path("output/ml_baseline")

HORIZON_EVENTS = 50
THRESHOLD_BPS = 0.10
SPREAD_CAP = 100.0

TRAIN_FRACTION = 0.70
VALIDATION_FRACTION = 0.10
TEST_FRACTION = 0.20

FEATURE_COLUMNS = [
    "spread",
    "l1_bid_qty",
    "l1_ask_qty",
    "l1_depth_imbalance",
    "lN_bid_qty_sum",
    "lN_ask_qty_sum",
    "lN_depth_imbalance",
    "ofi_l1",
    "ofi_lN",
    "weighted_ofi_lN",
    "rolling_mean_spread",
    "rolling_mean_l1_total_depth",
    "rolling_mean_lN_total_depth",
    "rolling_mid_return",
    "rolling_realized_mid_vol",
    "rolling_mean_abs_ofi_l1",
    "rolling_mean_abs_ofi_lN",
]


def make_labels(mid_price: pd.Series) -> pd.Series:
    future_mid = mid_price.shift(-HORIZON_EVENTS)
    future_return = (future_mid / mid_price) - 1.0
    threshold = THRESHOLD_BPS / 10_000.0

    label = pd.Series(np.nan, index=mid_price.index, dtype="float64")
    label.loc[future_return < -threshold] = -1
    label.loc[future_return.abs() <= threshold] = 0
    label.loc[future_return > threshold] = 1

    return label


def split_with_purge(frame: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    total = len(frame)

    train_end = int(total * TRAIN_FRACTION)
    validation_end = int(total * (TRAIN_FRACTION + VALIDATION_FRACTION))

    train = frame.iloc[:train_end].copy()

    validation_start = train_end + HORIZON_EVENTS
    validation = frame.iloc[validation_start:validation_end].copy()

    test_start = validation_end + HORIZON_EVENTS
    test = frame.iloc[test_start:].copy()

    if train.empty or validation.empty or test.empty:
        raise RuntimeError(
            "Split produced an empty partition. Check dataset size and split settings."
        )

    return train, validation, test


def evaluate(
    name: str,
    y_true: pd.Series,
    y_pred: np.ndarray,
) -> dict:
    labels = [-1, 0, 1]
    precision, recall, f1, support = precision_recall_fscore_support(
        y_true,
        y_pred,
        labels=labels,
        zero_division=0,
    )

    return {
        "name": name,
        "accuracy": float(accuracy_score(y_true, y_pred)),
        "balanced_accuracy": float(balanced_accuracy_score(y_true, y_pred)),
        "macro_f1": float(f1_score(y_true, y_pred, average="macro", zero_division=0)),
        "per_class": {
            str(label): {
                "precision": float(precision[index]),
                "recall": float(recall[index]),
                "f1": float(f1[index]),
                "support": int(support[index]),
            }
            for index, label in enumerate(labels)
        },
        "classification_report": classification_report(
            y_true,
            y_pred,
            labels=labels,
            target_names=["down", "flat", "up"],
            zero_division=0,
            output_dict=True,
        ),
    }


def main() -> None:
    if not np.isclose(TRAIN_FRACTION + VALIDATION_FRACTION + TEST_FRACTION, 1.0):
        raise RuntimeError("Train, validation, and test fractions must sum to 1.0.")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    df = pd.read_csv(INPUT_PATH)

    missing_columns = sorted(set(FEATURE_COLUMNS) - set(df.columns))
    if missing_columns:
        raise RuntimeError(f"Missing expected feature columns: {missing_columns}")

    df["target"] = make_labels(df["mid_price"])

    valid_state = (
        df["mid_price"].notna()
        & df["best_bid"].notna()
        & df["best_ask"].notna()
        & df["spread"].notna()
        & (df["best_bid"] < df["best_ask"])
        & (df["spread"] <= SPREAD_CAP)
        & df["target"].notna()
    )

    model_frame = df.loc[valid_state, FEATURE_COLUMNS + ["target"]].copy()
    model_frame["target"] = model_frame["target"].astype("int64")

    train, validation, test = split_with_purge(model_frame)

    x_train = train[FEATURE_COLUMNS]
    y_train = train["target"]

    x_validation = validation[FEATURE_COLUMNS]
    y_validation = validation["target"]

    x_test = test[FEATURE_COLUMNS]
    y_test = test["target"]

    model = Pipeline(
        steps=[
            ("imputer", SimpleImputer(strategy="median")),
            (
                "classifier",
                HistGradientBoostingClassifier(
                    learning_rate=0.08,
                    max_iter=200,
                    max_leaf_nodes=31,
                    l2_regularization=1.0,
                    random_state=42,
                ),
            ),
        ]
    )

    model.fit(x_train, y_train)

    validation_predictions = model.predict(x_validation)
    test_predictions = model.predict(x_test)

    majority_class = int(y_train.value_counts().idxmax())
    majority_predictions = np.full(len(y_test), majority_class, dtype=np.int64)

    metrics = {
        "config": {
            "input_path": str(INPUT_PATH),
            "sequence_order": "native CSV row order",
            "task": "three-class future-mid-price direction classification",
            "horizon_events": HORIZON_EVENTS,
            "threshold_bps": THRESHOLD_BPS,
            "spread_cap": SPREAD_CAP,
            "split": {
                "train_fraction": TRAIN_FRACTION,
                "validation_fraction": VALIDATION_FRACTION,
                "test_fraction": TEST_FRACTION,
                "purge_events_at_each_boundary": HORIZON_EVENTS,
            },
            "feature_columns": FEATURE_COLUMNS,
            "model": {
                "name": "HistGradientBoostingClassifier",
                "learning_rate": 0.08,
                "max_iter": 200,
                "max_leaf_nodes": 31,
                "l2_regularization": 1.0,
                "random_state": 42,
            },
        },
        "rows": {
            "raw_rows": int(len(df)),
            "eligible_rows": int(len(model_frame)),
            "train_rows": int(len(train)),
            "validation_rows": int(len(validation)),
            "test_rows": int(len(test)),
            "excluded_rows": int(len(df) - len(model_frame)),
        },
        "validation": evaluate("model_validation", y_validation, validation_predictions),
        "test": {
            "model": evaluate("model_test", y_test, test_predictions),
            "majority_class_baseline": evaluate(
                "majority_class_baseline_test",
                y_test,
                majority_predictions,
            ),
            "majority_class_from_training": majority_class,
        },
    }

    class_distribution = (
        pd.concat(
            {
                "train": y_train.value_counts().sort_index(),
                "validation": y_validation.value_counts().sort_index(),
                "test": y_test.value_counts().sort_index(),
            },
            axis=1,
        )
        .fillna(0)
        .astype("int64")
    )
    class_distribution.index.name = "target"
    class_distribution.to_csv(OUTPUT_DIR / "class_distribution.csv")

    cm = confusion_matrix(y_test, test_predictions, labels=[-1, 0, 1])
    pd.DataFrame(
        cm,
        index=["actual_down", "actual_flat", "actual_up"],
        columns=["predicted_down", "predicted_flat", "predicted_up"],
    ).to_csv(OUTPUT_DIR / "confusion_matrix.csv")

    predictions = pd.DataFrame(
        {
            "actual_target": y_test.to_numpy(),
            "predicted_target": test_predictions,
        }
    )
    predictions.to_csv(OUTPUT_DIR / "predictions_test.csv", index=False)

    print("Feature importance skipped for HistGradientBoostingClassifier.")

    with (OUTPUT_DIR / "metrics.json").open("w", encoding="utf-8") as handle:
        json.dump(metrics, handle, indent=2)

    print("===== BASELINE COMPLETE =====")
    print(f"Eligible rows: {len(model_frame):,}")
    print(f"Train rows: {len(train):,}")
    print(f"Validation rows: {len(validation):,}")
    print(f"Test rows: {len(test):,}")
    print(f"Majority class from train: {majority_class}")
    print()
    print("Validation macro F1:", metrics["validation"]["macro_f1"])
    print("Test model accuracy:", metrics["test"]["model"]["accuracy"])
    print("Test model balanced accuracy:", metrics["test"]["model"]["balanced_accuracy"])
    print("Test model macro F1:", metrics["test"]["model"]["macro_f1"])
    print(
        "Test majority baseline accuracy:",
        metrics["test"]["majority_class_baseline"]["accuracy"],
    )
    print(
        "Test majority baseline macro F1:",
        metrics["test"]["majority_class_baseline"]["macro_f1"],
    )
    print(f"Artifacts written to: {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
