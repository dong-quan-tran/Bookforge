# Strategy Replay Experiments

## Goal

Compare passive versus aggressive execution behavior under deterministic replay.

## Scope

This experiment uses replay plus scheduled injected orders to evaluate two simple execution styles against the same market stream:

- **Passive**: post a limit order at or near the best bid / best ask and wait for fills.
- **Aggressive**: cross the spread immediately to maximize fill certainty.

The goal is not to build a full strategy framework yet. The goal is to create a small, reproducible experiment harness that can answer a practical microstructure question with clear metrics.

## Initial experiment design

### Inputs

- Replay event stream from historical or synthetic CSV.
- Deterministic injected order schedule.
- Fixed parent order size.
- Configurable entry timestamps or replay offsets.
- Strategy mode: `passive` or `aggressive`.

### Passive behavior

The passive version should:

- submit a limit buy at best bid or a limit sell at best ask,
- rest in the book,
- measure whether and when it fills,
- record remaining unfilled quantity if the replay window ends first.

### Aggressive behavior

The aggressive version should:

- submit a marketable order that crosses the spread,
- execute immediately against available resting liquidity,
- measure execution price and total filled quantity,
- record slippage or spread cost relative to a reference price.

## Metrics

Each replay trial should record:

- fill rate,
- filled quantity,
- time to first fill,
- time to complete fill,
- average execution price,
- mid-price at decision time,
- spread at decision time,
- implementation shortfall,
- remaining quantity at replay end.

## Output format

A first version can write one CSV row per trial with fields such as:

`strategy,entry_offset,is_buy,limit_price,requested_qty,filled_qty,remaining_qty,fill_rate,avg_execution_price,decision_mid_price,decision_spread,implementation_shortfall_bps,time_to_first_fill_us,time_to_full_fill_us`

## CLI usage

The `strategy_experiment_main` executable runs a single replay experiment over one input CSV.

Basic usage:

```bash
strategy_experiment_main --input data/synthetic_replay_fixture.csv \
                         --output output/strategy_experiment_results.csv \
                         --mode passive \
                         --side buy \
                         --quantity 10 \
                         --entry-offset 0
```

Arguments:

- `--input`: required, path to the replay CSV fixture.
- `--output`: optional, path to the experiment result CSV (default `output/strategy_experiment_results.csv`).
- `--mode`: `passive` or `aggressive` to select strategy behavior.
- `--side`: `buy` or `sell` for the injected order side.
- `--quantity`: requested order quantity.
- `--entry-offset`: zero-based event index at which the injected order should be scheduled.

The current implementation runs a single trial and writes one row of experiment output, which can be extended later to batch multiple trials or compare passive versus aggressive behavior side by side.

## Suggested implementation plan

1. Add a small experiment runner that reuses replay and injected-order scheduling.
2. Add a strategy enum for passive versus aggressive behavior.
3. Capture per-trial metrics in a dedicated result struct.
4. Write results to CSV for later analysis.
5. Document a simple example command and expected outputs.

## Notes

This experiment is intentionally narrow. It should produce a usable baseline for later work such as queue-position-aware studies, richer execution tactics, or agent-based simulation.