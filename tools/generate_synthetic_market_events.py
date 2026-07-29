from __future__ import annotations

import argparse
import csv
import random
from dataclasses import dataclass
from datetime import datetime, timedelta


@dataclass(frozen=True)
class GeneratorConfig:
    events: int
    base_price: float
    tick_size: float
    base_spread_ticks: int
    min_size: float
    max_size: float
    buy_probability: float
    new_probability: float
    reject_probability: float
    seed: int
    start_timestamp: str
    output: str


def _parse_args() -> GeneratorConfig:
    parser = argparse.ArgumentParser(
        description="Generate a synthetic Hyperliquid-style replay CSV fixture."
    )
    parser.add_argument("--events", type=int, default=10000)
    parser.add_argument("--base-price", type=float, default=100000.0)
    parser.add_argument("--tick-size", type=float, default=0.5)
    parser.add_argument("--base-spread-ticks", type=int, default=2)
    parser.add_argument("--min-size", type=float, default=0.001)
    parser.add_argument("--max-size", type=float, default=0.050)
    parser.add_argument("--buy-probability", type=float, default=0.5)
    parser.add_argument("--new-probability", type=float, default=0.92)
    parser.add_argument("--reject-probability", type=float, default=0.04)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument(
        "--start-timestamp",
        type=str,
        default="2025-12-15T11:39:39.000000",
        help="ISO-like timestamp, e.g. 2025-12-15T11:39:39.000000",
    )
    parser.add_argument(
        "--output",
        type=str,
        default="data/synthetic_replay_fixture.csv",
    )
    args = parser.parse_args()

    if args.events <= 0:
        raise ValueError("--events must be positive")
    if args.tick_size <= 0:
        raise ValueError("--tick-size must be positive")
    if args.base_spread_ticks <= 0:
        raise ValueError("--base-spread-ticks must be positive")
    if args.min_size <= 0 or args.max_size <= 0:
        raise ValueError("--min-size and --max-size must be positive")
    if args.min_size > args.max_size:
        raise ValueError("--min-size must be <= --max-size")
    if not 0.0 <= args.buy_probability <= 1.0:
        raise ValueError("--buy-probability must be in [0, 1]")
    if not 0.0 <= args.new_probability <= 1.0:
        raise ValueError("--new-probability must be in [0, 1]")
    if not 0.0 <= args.reject_probability <= 1.0:
        raise ValueError("--reject-probability must be in [0, 1]")
    if args.new_probability + args.reject_probability > 1.0:
        raise ValueError("--new-probability + --reject-probability must be <= 1")

    return GeneratorConfig(
        events=args.events,
        base_price=args.base_price,
        tick_size=args.tick_size,
        base_spread_ticks=args.base_spread_ticks,
        min_size=args.min_size,
        max_size=args.max_size,
        buy_probability=args.buy_probability,
        new_probability=args.new_probability,
        reject_probability=args.reject_probability,
        seed=args.seed,
        start_timestamp=args.start_timestamp,
        output=args.output,
    )


def _format_timestamp(ts: datetime) -> str:
    return ts.strftime("%Y-%m-%d %H:%M:%S.%f")


def _sample_event_type(rng: random.Random, cfg: GeneratorConfig) -> tuple[str, int, str]:
    u = rng.random()
    if u < cfg.new_probability:
        return "New", 1, "open"
    if u < cfg.new_probability + cfg.reject_probability:
        return "Reject", 3, "perpMarginRejected"
    return "Other", 9, "syntheticOther"


def _bounded_random_walk(rng: random.Random, price: float, tick_size: float) -> float:
    step = rng.choice([-2, -1, 0, 1, 2])
    next_price = price + step * tick_size
    return max(tick_size, next_price)


def generate_rows(cfg: GeneratorConfig) -> list[dict[str, object]]:
    rng = random.Random(cfg.seed)
    current_mid = cfg.base_price
    current_ts = datetime.fromisoformat(cfg.start_timestamp)

    rows: list[dict[str, object]] = []

    for _ in range(cfg.events):
        current_mid = _bounded_random_walk(rng, current_mid, cfg.tick_size)
        spread_ticks = max(1, cfg.base_spread_ticks + rng.choice([-1, 0, 0, 1]))
        half_spread = (spread_ticks * cfg.tick_size) / 2.0

        event_type, status_id, status_text = _sample_event_type(rng, cfg)
        is_ask = rng.random() > cfg.buy_probability
        size = round(rng.uniform(cfg.min_size, cfg.max_size), 5)

        if is_ask:
            price = current_mid + half_spread + rng.choice([0, 1, 2]) * cfg.tick_size
        else:
            price = current_mid - half_spread - rng.choice([0, 1, 2]) * cfg.tick_size

        rows.append(
            {
                "ts": _format_timestamp(current_ts),
                "limitPx": round(price, 5),
                "sz": size,
                "isAsk": "True" if is_ask else "False",
                "statusId": status_id,
                "status": status_text,
                "eventType": event_type,
            }
        )

        current_ts += timedelta(microseconds=rng.randint(50, 5000))

    return rows


def write_csv(path: str, rows: list[dict[str, object]]) -> None:
    fieldnames = ["ts", "limitPx", "sz", "isAsk", "statusId", "status", "eventType"]
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    cfg = _parse_args()
    rows = generate_rows(cfg)
    write_csv(cfg.output, rows)
    print(f"Wrote {len(rows)} synthetic events to {cfg.output}")


if __name__ == "__main__":
    main()