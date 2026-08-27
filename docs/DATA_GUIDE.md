## Hyperliquid enriched CSV â†’ ExternalOrderEvent mapping

This section documents how enriched Hyperliquid order-status CSV rows are mapped into the internal `ExternalOrderEvent` model and downstream replay components.

### CSV schema

The current enriched CSV schema is:

```text
ts,limitPx,sz,isAsk,statusId,status,eventType
2025-12-15 11:39:39.722049503,89691.0,0.01672,True,3,perpMarginRejected,Reject
```

Fields:

- `ts` - wall-clock timestamp (nanosecond-resolution string)
- `limitPx` - limit price
- `sz` - order size (base asset units)
- `isAsk` - side flag (True = sell, False = buy)
- `statusId` - numeric status code
- `status` - textual status description
- `eventType` - higher-level event classification label used during enrichment

### ExternalOrderEvent

`ExternalOrderEvent` is the internal replay event struct used by adapters and replay runners.

Conceptual shape:

```cpp
struct ExternalOrderEvent {
    std::chrono::nanoseconds ts;
    double price;
    double size;
    bool isAsk;
    int statusId;
    std::string statusText;
    EventType eventType;
};
```

### Field-level mapping

Mapping from CSV fields into `ExternalOrderEvent`:

| CSV field  | Type       | ExternalOrderEvent field | Conversion / notes                                      |
|-----------|------------|---------------------------|---------------------------------------------------------|
| `ts`      | string     | `ts`                      | Parsed into `std::chrono::nanoseconds` or stubbed 0    |
| `limitPx` | string     | `price`                   | `std::stod(limitPx)`                                   |
| `sz`      | string     | `size`                    | `std::stod(sz)`                                        |
| `isAsk`   | string     | `isAsk`                   | `"True"/"False"/\"1\"/\"0\"` â†’ `bool` via helper       |
| `statusId`| string/int | `statusId`                | `std::stoi(statusId)`                                  |
| `status`  | string     | `statusText`             | Stored as-is; used for `EventType` classification      |
| `eventType`| string    | (not stored directly)     | Used only during enrichment; `status` drives `EventType` |

### EventType classification

`EventType` is derived from `statusText` inside `HyperliquidCsvReader::map_event_type`.

Current mapping rules:

- `EventType::New`  
  - `statusText` exactly one of: `"open"`, `"resting"`, `"received"`

- `EventType::Cancel`  
  - `statusText` exactly one of: `"canceled"`, `"cancelled"`

- `EventType::Fill`  
  - `statusText` exactly one of: `"filled"`, `"partiallyFilled"`, `"partialFill"`

- `EventType::Trigger`  
  - `statusText` contains `"triggered"` or equals `"trigger"`

- `EventType::Reject`  
  - `statusText` contains `"Rejected"` or `"rejected"`  
    (e.g., `"perpMarginRejected"`)

- `EventType::Other`  
  - Fallback for all unmapped `statusText` values

These rules intentionally treat `statusText` as the single source of truth for `EventType`. The enriched `eventType` CSV column is informational and may diverge from internal classification; any divergence should be resolved by adjusting `map_event_type`.

### Parsing and error handling

Parsing is performed by `HyperliquidCsvReader`:

- Input: path to enriched CSV.
- Behavior:
  - Ignores a header row if it contains both `ts` and `limitPx`.
  - Skips empty lines.
  - Uses a simple comma-based split with trimming.
  - Requires at least 7 fields; fewer fields are treated as malformed and either skipped or cause a throw depending on mode.

Error handling modes:

- **Non-strict** (`strict_mode = false`):
  - Malformed rows are logged (if `log_errors` is true).
  - Events from malformed rows are skipped.
  - Replay remains deterministic over the filtered set.

- **Strict** (`strict_mode = true`):
  - First malformed row causes an exception.
  - Caller (tests or replay main) is responsible for handling the failure.

### ReplayConfig integration

Replay configuration controls how CSV-derived events are consumed:

Relevant `ReplayConfig` fields:

```cpp
struct ReplayConfig {
    std::string path;
    std::string symbol;
    ReplaySource source;        // e.g., ReplaySource::Hyperliquid
    std::uint64_t max_events;   // 0 = no limit
    std::uint64_t start_offset; // skip first N events
    std::uint64_t log_every_n;
    bool log_summary;
    bool log_errors;
    bool strict_mode;
};
```

Usage with `HyperliquidCsvReader` and `ReplayRunner`:

1. `HyperliquidCsvReader reader(config.path);`
2. `auto events = reader.read_all(config.strict_mode, config.log_errors);`
3. `ReplayRunner runner(config);`
4. `runner.Run(adapter, events);`

This ensures:

- CSV parsing and `EventType` classification are source-specific and testable.
- Replay traversal and engine interaction are generic and configured via `ReplayConfig`.

### Invariants and assumptions

- `isAsk == true` â†’ sell-side event; `isAsk == false` â†’ buy-side event.
- `limitPx` and `sz` are in exchange-native units; no normalization is performed at parse time.
- `statusId` is preserved for debugging and potential future use but does not drive `EventType`.
- There is no guaranteed one-to-one mapping between external order IDs and internal engine order IDs yet; cancel/fill modeling is intentionally conservative until that linkage is added.
- Timestamp parsing can be replaced with a real implementation once deterministic replay guarantees that do not depend on wall-clock time are fully defined.

Any future changes to the enrichment schema or Hyperliquid API status taxonomy must be reflected here and in `HyperliquidCsvReader::map_event_type` to keep replay behavior predictable.[web:1290][web:1165]