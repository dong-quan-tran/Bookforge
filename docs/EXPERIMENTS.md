# Research Experiments

## Scope

Bookforge includes a reproducible short-horizon market-microstructure research workflow built from deterministic C++ replay, feature export, and Python evaluation tooling.

The current reported experiment uses a BTCUSDT.P feature export generated from replayed enriched order-event data. It is an engineering and research validation artifact, not a trading strategy or profitability claim.

## Dataset

| Property | Value |
|---|---:|
| Instrument | BTCUSDT.P |
| Raw feature rows | 2,166,109 |
| Exported feature columns | 23 |
| Valid mid-price rows | 2,166,084 |
| Unique mid-prices | 422 |
| Event-to-event mid-price changes | 1,437 |
| Timestamp range | approximately 2.87 seconds |
| Zero timestamps | 0 |
| Source timestamp reversals | 2 |
| Maximum observed spread | 8,966 |
| Spread-quality cap used for modeling | 100 |

The feature export includes top-of-book prices, spread, L1 and multi-level depth, depth imbalance, L1 and multi-level order-flow imbalance, weighted OFI, and rolling liquidity, return, and volatility context.

## Target

The baseline predicts the future mid-price direction after 50 replay events.

\[
r_{t,t+50} = \frac{\mathrm{mid}_{t+50}}{\mathrm{mid}_t} - 1
\]

Labels use a ±0.10 basis-point threshold:

| Class | Definition |
|---|---|
| Down (-1) | \(r_{t,t+50} < -0.10\) bps |
| Flat (0) | \(\lvert r_{t,t+50} \rvert \le 0.10\) bps |
| Up (+1) | \(r_{t,t+50} > 0.10\) bps |

## Evaluation protocol

- Preserved native feature-export row order because rolling features and event horizons are replay-sequential.
- Excluded rows with unavailable top-of-book state, unavailable current/future mid-price, crossed/invalid top-of-book state, or spread greater than 100.
- Used a 70% / 10% / 20% train/validation/test split by sequence position.
- Purged 50 events at each split boundary to avoid future-label overlap.
- Fit preprocessing only on training data.
- Compared the model against an always-flat majority-class baseline.
- Reported macro F1 and balanced accuracy as primary metrics because flat labels dominate the evaluation window.

## Model

| Parameter | Value |
|---|---|
| Model | HistGradientBoostingClassifier |
| Learning rate | 0.08 |
| Maximum iterations | 200 |
| Maximum leaf nodes | 31 |
| L2 regularization | 1.0 |
| Random seed | 42 |

The model used 17 replay-derived microstructure and rolling-context features.

## Results

| Test metric | Model | Always-flat baseline |
|---|---:|---:|
| Accuracy | 0.9928 | 0.9945 |
| Balanced accuracy | 0.3473 | 0.3333 |
| Macro F1 | 0.3535 | 0.3324 |
| Down recall | 0.0136 | 0.0000 |
| Up recall | 0.0302 | 0.0000 |

The model slightly improves macro F1 and balanced accuracy over the majority baseline, but it does not produce a deployable directional signal. Directional recall remains low, and raw accuracy is not meaningful because the held-out test segment is overwhelmingly flat.

## Interpretation

This experiment validates the replay-to-feature-to-evaluation pipeline and demonstrates leakage-aware temporal evaluation under severe class imbalance.

It does not establish tradable alpha, profitability, cross-regime robustness, or live-execution performance. The timestamp span is short, the source contains two timestamp-order reversals, and directional movement is sparse and temporally unstable.

## Reproduction

Generate features from an available enriched BTC event file:

```powershell
.\build-release\Release\feature_export_main.exe `
    --input data\btc_orders_sample_enriched.csv `
    --output output\features_btc_enriched.csv `
    --symbol BTCUSDT.P `
    --snapshot-depth 10 `
    --imbalance-depth 10 `
    --ofi-depth 10 `
    --rolling-window 50 `
    --strict `
    --log-every 50000
```

Run the baseline:

```powershell
python python\run_event_horizon_baseline.py
```

The script writes metrics, class distributions, a confusion matrix, and held-out predictions under `output\ml_baseline\`.