# LogForge Architecture

## Overview

LogForge is a high-performance log analytics engine built with Modern C++20. It streams log files line-by-line, parses structured entries, and provides search and statistics capabilities.

## Design Principles

- **Streaming first**: Never load entire files into memory. Files larger than RAM are supported.
- **Interface segregation**: Core components expose abstract interfaces (`IParser`, `IFileReader`, `ISearchEngine`, `IStatisticsEngine`) for testability and future swapping.
- **Move semantics**: `LogEntry` objects are moved, not copied, during pipeline processing.
- **Single responsibility**: Each class has one clear job.

## Component Diagram

```
┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
│  FileReader │────▶│    Parser    │────▶│     Indexer     │
│  (streaming)│     │  (IParser)   │     │  (hash indexes) │
└─────────────┘     └──────────────┘     └────────┬────────┘
                                                   │
                          ┌────────────────────────┼────────────────────────┐
                          ▼                        ▼                        ▼
                   ┌──────────────────┐   ┌──────────────┐         ┌────────────────┐
                   │IndexedSearchEng  │   │StatisticsEng │         │ SearchEngine   │
                   │  (O(1) queries)  │   │              │         │ (linear scan)  │
                   └──────────────────┘   └──────────────┘         └────────────────┘
                                                   ▲
┌──────────────────────┐     ┌─────────────────────┴──────────────────────┐
│  ParallelFileReader  │────▶│              ThreadPool (Phase 4)          │
│  (chunk-based parse) │     └────────────────────────────────────────────┘
└──────────────────────┘
```

## Log Format

```
YYYY-MM-DD HH:MM:SS LEVEL ServiceName message text [key=value ...]
```

Example:
```
2026-08-29 10:15:22 INFO AuthService User login successful user=145
```

Parsed fields:
| Field     | Example              |
|-----------|----------------------|
| timestamp | 2026-08-29 10:15:22  |
| level     | INFO                 |
| service   | AuthService          |
| message   | User login successful|
| metadata  | user=145             |

## Complexity Analysis

| Operation          | Time       | Space      | Phase |
|--------------------|------------|------------|-------|
| Parse single line  | O(n)       | O(n)       | 1     |
| Stream file        | O(lines)   | O(1) buf   | 1     |
| Build index        | O(n·w)     | O(n·w)     | 2     |
| Indexed query      | O(1) avg   | O(k)       | 2     |
| Linear search      | O(n·m)     | O(k)       | 1     |
| Statistics         | O(n)       | O(s+e)     | 1     |
| Advanced stats     | O(n)       | O(s+m+24)  | 3     |
| Parallel parse     | O(n/p)     | O(n)       | 4     |

*n = entry count, p = thread count, m = messages, s = services*

## Indexer Design (Phase 2)

The `Indexer` maintains inverted posting lists:

| Index   | Key Type   | Use Case                    |
|---------|------------|-----------------------------|
| Level   | `LogLevel` | `search app.log ERROR`      |
| Service | `string`   | `search app.log AuthService`|
| Keyword | `string`   | Tokenized message/metadata  |
| Hour    | `int`      | Timeline and time filtering |

`IndexedSearchEngine` uses hash lookups for exact matches, then falls back to
substring scan for partial queries (e.g. `data` matching `Database`).

## Parallel Parsing (Phase 4)

`ParallelFileReader` splits files into newline-aligned byte chunks and dispatches
each chunk to `ThreadPool` workers. Results are merged in chunk order with
sequential line numbers reassigned during merge.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `num_threads` | `hardware_concurrency()` | Worker thread count |
| `min_parallel_bytes` | 64 KB | Serial fallback below this size |

Falls back to `FileReader` for small files where thread overhead exceeds benefit.

## Advanced Statistics (Phase 3)

`AdvancedStatisticsEngine` extends basic counts with:

| Metric | Description |
|--------|-------------|
| Error/warning rates | Overall and per-service percentages |
| Hourly error rates | Failure rate per hour-of-day |
| Volume percentiles | p50, p90, p99 of hourly log volume |
| Spike detection | Hours with error count ≥ 2× mean (min 2 errors) |
| Message frequency | Counts for all messages, not just errors |

Spike detection uses deviation from the 24-hour mean error count, requiring a
minimum of 2 errors to avoid false positives on low-volume uniform distributions.

## Tradeoffs

### Linear search vs. indexing (Phase 1 → 2)
- **Phase 1**: Linear scan for simplicity and correctness.
- **Phase 2**: Hash-based inverted index for O(1) exact-match queries.
- **Hybrid**: `IndexedSearchEngine` uses indexes first, substring fallback for partial matches.

### In-memory storage vs. external index
- **Chosen**: Vector storage with streaming ingestion.
- **Tradeoff**: Memory proportional to entry count.
- **Rationale**: Phase 1 targets moderate files; Phase 2 adds persistent indexes.

### PIMPL for CLI
- **Chosen**: Pointer-to-implementation to reduce compile-time coupling.
- **Tradeoff**: Small heap allocation vs. faster incremental builds.

## Future Phases

- [x] Phase 2: Hash-based indexes (level, service, keyword, timestamp)
- [x] Phase 3: Advanced statistics (percentiles, spike detection)
- [x] Phase 4: Thread pool for parallel chunk processing
- **Phase 5**: Live file watching with `inotify`/`kqueue`
- **Phase 6**: JSON configuration, output formats, ignore patterns
