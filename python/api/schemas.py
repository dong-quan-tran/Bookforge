from typing import Any

from pydantic import BaseModel, Field


class HealthResponse(BaseModel):
    status: str = "ok"


class ReplaySummaryResponse(BaseModel):
    features_csv: str
    row_count: int
    column_count: int
    columns: list[str]
    symbol: str | None = None
    start_event_index: int | None = None
    end_event_index: int | None = None
    start_timestamp_ns: int | None = None
    end_timestamp_ns: int | None = None
    avg_spread: float | None = None
    avg_mid_price: float | None = None
    avg_depth_imbalance: float | None = None


class FeatureSampleRow(BaseModel):
    row_index: int
    values: dict[str, Any]


class FeatureSampleResponse(BaseModel):
    features_csv: str
    row_count: int
    returned_count: int
    rows: list[FeatureSampleRow]


class SnapshotSampleRow(BaseModel):
    row_index: int
    values: dict[str, Any]


class SnapshotSampleResponse(BaseModel):
    snapshot_csv: str
    row_count: int
    returned_count: int
    rows: list[SnapshotSampleRow]


class SnapshotLevel(BaseModel):
    level: int
    price: float | None = None
    size: float | None = None


class SnapshotInspectResponse(BaseModel):
    snapshot_csv: str
    row_index: int
    total_rows: int
    symbol: str | None = None
    replay_event_index: int | None = None
    replay_timestamp_ns: int | None = None
    best_bid: float | None = None
    best_ask: float | None = None
    spread: float | None = None
    mid_price: float | None = None
    bids: list[SnapshotLevel]
    asks: list[SnapshotLevel]
    raw_values: dict[str, Any]


class ColumnsResponse(BaseModel):
    features_csv: str
    columns: list[str]


class ErrorResponse(BaseModel):
    detail: str = Field(..., description="Human-readable error message")
