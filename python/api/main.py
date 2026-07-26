from fastapi import FastAPI, Query
from fastapi.middleware.cors import CORSMiddleware

from .schemas import (
    ColumnsResponse,
    ErrorResponse,
    FeatureSampleResponse,
    HealthResponse,
    ReplaySummaryResponse,
    SnapshotInspectResponse,
    SnapshotSampleResponse,
)
from .services import (
    build_columns_response,
    build_feature_sample,
    build_replay_summary,
    build_snapshot_inspect,
    build_snapshot_sample,
)

app = FastAPI(
    title="Bookforge API",
    version="0.1.0",
    description="Read-only API for replay summaries, feature samples, and snapshot inspection.",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/health", response_model=HealthResponse, tags=["system"])
def health() -> HealthResponse:
    return HealthResponse(status="ok")


@app.get(
    "/api/replay/summary",
    response_model=ReplaySummaryResponse,
    responses={404: {"model": ErrorResponse}, 400: {"model": ErrorResponse}},
    tags=["replay"],
)
def replay_summary(
    features_csv: str | None = Query(default=None, description="Path to exported features CSV"),
) -> ReplaySummaryResponse:
    return ReplaySummaryResponse(**build_replay_summary(features_csv))


@app.get(
    "/api/features/sample",
    response_model=FeatureSampleResponse,
    responses={404: {"model": ErrorResponse}, 400: {"model": ErrorResponse}},
    tags=["features"],
)
def feature_sample(
    features_csv: str | None = Query(default=None, description="Path to exported features CSV"),
    limit: int = Query(default=200, ge=1, le=5000),
    offset: int = Query(default=0, ge=0),
) -> FeatureSampleResponse:
    return FeatureSampleResponse(**build_feature_sample(features_csv, limit, offset))


@app.get(
    "/api/features/columns",
    response_model=ColumnsResponse,
    responses={404: {"model": ErrorResponse}, 400: {"model": ErrorResponse}},
    tags=["features"],
)
def feature_columns(
    features_csv: str | None = Query(default=None, description="Path to exported features CSV"),
) -> ColumnsResponse:
    return ColumnsResponse(**build_columns_response(features_csv))


@app.get(
    "/api/snapshots/sample",
    response_model=SnapshotSampleResponse,
    responses={404: {"model": ErrorResponse}, 400: {"model": ErrorResponse}},
    tags=["snapshots"],
)
def snapshot_sample(
    snapshot_csv: str | None = Query(default=None, description="Path to snapshot CSV"),
    limit: int = Query(default=100, ge=1, le=5000),
    offset: int = Query(default=0, ge=0),
) -> SnapshotSampleResponse:
    return SnapshotSampleResponse(**build_snapshot_sample(snapshot_csv, limit, offset))


@app.get(
    "/api/snapshots/inspect",
    response_model=SnapshotInspectResponse,
    responses={404: {"model": ErrorResponse}, 400: {"model": ErrorResponse}},
    tags=["snapshots"],
)
def snapshot_inspect(
    snapshot_csv: str | None = Query(default=None, description="Path to snapshot CSV"),
    row_index: int = Query(default=0, ge=0),
) -> SnapshotInspectResponse:
    return SnapshotInspectResponse(**build_snapshot_inspect(snapshot_csv, row_index))