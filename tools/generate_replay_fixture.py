from __future__ import annotations

import argparse
import csv
from decimal import ROUND_HALF_UP, Decimal
from pathlib import Path

HEADER = ["ts", "limitPx", "sz", "isAsk", "statusId", "status", "eventType"]


def fmt_price(x: Decimal) -> str:
    return str(x.quantize(Decimal("0.01"), rounding=ROUND_HALF_UP))


def fmt_size(x: Decimal) -> str:
    return str(x.quantize(Decimal("0.00001"), rounding=ROUND_HALF_UP))


def make_row(i: int, base_ts: str, mid: Decimal) -> list[str]:
    cycle = i % 12

    if cycle == 0:
        px = mid - Decimal("0.30")
        sz = Decimal("0.01000")
        is_ask = "False"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 1:
        px = mid + Decimal("0.30")
        sz = Decimal("0.01000")
        is_ask = "True"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 2:
        px = mid - Decimal("0.20")
        sz = Decimal("0.01500")
        is_ask = "False"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 3:
        px = mid + Decimal("0.20")
        sz = Decimal("0.01500")
        is_ask = "True"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 4:
        px = mid - Decimal("0.10")
        sz = Decimal("0.02000")
        is_ask = "False"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 5:
        px = mid + Decimal("0.10")
        sz = Decimal("0.02000")
        is_ask = "True"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 6:
        px = mid
        sz = Decimal("0.01250")
        is_ask = "False"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 7:
        px = mid
        sz = Decimal("0.01250")
        is_ask = "True"
        status_id = "1"
        status = "open"
        event_type = "New"
    elif cycle == 8:
        px = mid + Decimal("0.75")
        sz = Decimal("0.01000")
        is_ask = "True"
        status_id = "3"
        status = "perpMarginRejected"
        event_type = "Reject"
    elif cycle == 9:
        px = mid - Decimal("0.75")
        sz = Decimal("0.01000")
        is_ask = "False"
        status_id = "3"
        status = "perpMarginRejected"
        event_type = "Reject"
    elif cycle == 10:
        px = mid - Decimal("0.05")
        sz = Decimal("0.02500")
        is_ask = "False"
        status_id = "1"
        status = "open"
        event_type = "New"
    else:
        px = mid + Decimal("0.05")
        sz = Decimal("0.02500")
        is_ask = "True"
        status_id = "1"
        status = "open"
        event_type = "New"

    ts = f"{base_ts}{i:09d}"
    return [ts, fmt_price(px), fmt_size(sz), is_ask, status_id, status, event_type]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        default="tests/fixtures/hyperliquid_replay_fixture_large.csv",
    )
    parser.add_argument("--events", type=int, default=10000)
    parser.add_argument("--base-price", type=Decimal, default=Decimal("100.00"))
    args = parser.parse_args()

    out_path = Path(args.output)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(HEADER)
        for i in range(args.events):
            drift = Decimal((i // 500) % 7 - 3) * Decimal("0.05")
            mid = args.base_price + drift
            writer.writerow(make_row(i, "2025-12-15 11:39:39.", mid))


if __name__ == "__main__":
    main()