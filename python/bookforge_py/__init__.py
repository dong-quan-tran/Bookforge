from .dataset import (
    ChronologicalSplit,
    TrainingDataset,
    WalkForwardFold,
    build_training_dataset,
    build_training_dataset_from_frame,
    chronological_split,
    walk_forward_splits,
)
from .impact import (
    KyleLambdaResult,
    compute_price_change,
    estimate_kyle_lambda,
    estimate_kyle_lambda_by_window,
)
from .labels import (
    ClassificationThresholds,
    HorizonSpec,
    classify_return,
    compute_log_return,
    make_labels,
)
from .loaders import (
    DEFAULT_METADATA_COLUMNS,
    feature_column_names,
    load_feature_csv,
    split_feature_columns,
    validate_feature_frame,
)

try:
    from ._native import BookSnapshot, DepthLevelSnapshot, FeatureRow
except ImportError as exc:
    _native_import_error = exc

    def __getattr__(name: str):
        if name in {"BookSnapshot", "DepthLevelSnapshot", "FeatureRow"}:
            raise ImportError(
                "Bookforge native bindings are unavailable. "
                "Configure and build the CMake project before importing native types."
            ) from _native_import_error
        raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


__all__ = [
    "BookSnapshot",
    "DepthLevelSnapshot",
    "FeatureRow",
    "ChronologicalSplit",
    "TrainingDataset",
    "WalkForwardFold",
    "build_training_dataset",
    "build_training_dataset_from_frame",
    "chronological_split",
    "walk_forward_splits",
    "KyleLambdaResult",
    "compute_price_change",
    "estimate_kyle_lambda",
    "estimate_kyle_lambda_by_window",
    "ClassificationThresholds",
    "HorizonSpec",
    "classify_return",
    "compute_log_return",
    "make_labels",
    "DEFAULT_METADATA_COLUMNS",
    "feature_column_names",
    "load_feature_csv",
    "split_feature_columns",
    "validate_feature_frame",
]
