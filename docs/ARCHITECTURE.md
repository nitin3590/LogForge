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
│  FileReader │────▶│    Parser    │────▶│   LogEntry[]    │
│  (streaming)│     │  (IParser)   │     │                 │
└─────────────┘     └──────────────┘     └────────┬────────┘
                                                   │
                          ┌────────────────────────┼────────────────────────┐
                          ▼                        ▼                        ▼
                   ┌─────────────┐         ┌──────────────┐         ┌────────────────┐
                   │SearchEngine │         │StatisticsEng │         │ Indexer (Ph.2) │
                   └─────────────┘         └──────────────┘         └────────────────┘
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
| Linear search      | O(n·m)     | O(k)       | 1     |
| Statistics         | O(n)       | O(s+e)     | 1     |
| Indexed search     | O(1) avg   | O(n)       | 2     |

*n = entry count, m = query length, k = results, s = services, e = error messages*

## Tradeoffs

### Linear search vs. indexing (Phase 1)
- **Chosen**: Linear scan for Phase 1 simplicity.
- **Tradeoff**: O(n) per query vs. O(1) with hash index.
- **Rationale**: Correctness and clean architecture first; indexing added in Phase 2.

### In-memory storage vs. external index
- **Chosen**: Vector storage with streaming ingestion.
- **Tradeoff**: Memory proportional to entry count.
- **Rationale**: Phase 1 targets moderate files; Phase 2 adds persistent indexes.

### PIMPL for CLI
- **Chosen**: Pointer-to-implementation to reduce compile-time coupling.
- **Tradeoff**: Small heap allocation vs. faster incremental builds.

## Future Phases

- **Phase 2**: Hash-based indexes (level, service, keyword, timestamp)
- **Phase 3**: Advanced statistics (percentiles, spike detection)
- **Phase 4**: Thread pool for parallel chunk processing
- **Phase 5**: Live file watching with `inotify`/`kqueue`
- **Phase 6**: JSON configuration, output formats, ignore patterns
