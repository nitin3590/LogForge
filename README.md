# LogForge

**High-performance multithreaded log analytics engine built with Modern C++20.**

[![CI](https://github.com/yourusername/LogForge/actions/workflows/ci.yml/badge.svg)](https://github.com/yourusername/LogForge/actions/workflows/ci.yml)

LogForge parses, searches, and analyzes production log files efficiently — like a lightweight combination of `grep`, Splunk, and Datadog Logs. It streams files larger than RAM, extracts structured fields, and answers questions about errors, services, and patterns instantly.

## Why This Project Exists

Modern systems generate enormous log volumes. Developers need fast answers:

- What errors occurred?
- Which service generated the most failures?
- Which hour had the highest error rate?
- Search all ERROR logs instantly.

LogForge demonstrates production-grade C++ engineering: RAII, smart pointers, streaming I/O, multithreading (upcoming), indexing, benchmarking, and comprehensive testing.

## Features (Phase 1–2)

- **Streaming parser** — process files larger than system memory
- **Structured parsing** — timestamp, level, service, message, metadata
- **Hash-based indexing** — O(1) queries by level, service, keyword, and hour
- **Search** — indexed exact-match with substring fallback
- **Statistics** — error/warn/info counts, top services, top errors, hourly timeline
- **CLI** — `stats`, `search`, `timeline`, `top-errors`, `top-services`

## Architecture

```
LogForge/
├── include/logforge/     # Public headers
│   ├── core/             # LogEntry, LogLevel
│   ├── parser/           # IParser, Parser
│   ├── io/               # IFileReader, FileReader
│   ├── index/            # IIndexer, Indexer
│   ├── search/           # ISearchEngine, SearchEngine, IndexedSearchEngine
│   ├── stats/            # IStatisticsEngine, StatisticsEngine
│   └── cli/              # CLI
├── src/                  # Implementations
├── tests/                # GoogleTest unit tests
├── benchmarks/           # Google Benchmark
└── sample_logs/          # Example log files
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for design decisions and complexity analysis.

## Requirements

- C++20 compiler (GCC 11+, Clang 14+, MSVC 19.29+)
- CMake 3.20+
- Git (for FetchContent dependencies)

## Installation

```bash
git clone https://github.com/yourusername/LogForge.git
cd LogForge
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The `logforge` binary will be at `build/logforge` (or `build/Release/logforge.exe` on Windows).

### Run Tests

```bash
cmake --build build --target logforge_tests
ctest --test-dir build --output-on-failure
```

### Run Benchmarks

```bash
cmake -B build -DLOGFORGE_BUILD_BENCHMARKS=ON
cmake --build build --target logforge_bench
./build/benchmarks/logforge_bench
```

## Usage

```bash
# Summary statistics
./build/logforge stats sample_logs/app.log

# Search by level
./build/logforge search sample_logs/app.log ERROR

# Search by service
./build/logforge search sample_logs/app.log PaymentService

# Hourly timeline
./build/logforge timeline sample_logs/app.log

# Top error messages
./build/logforge top-errors sample_logs/app.log

# Top services by volume
./build/logforge top-services sample_logs/app.log
```

### Example Output

```
=== Log Statistics ===
Total entries: 16

By Level:
  ERROR: 6
  WARN:  3
  INFO:  4
  DEBUG: 1
  TRACE: 1
  FATAL: 1
```

## Log Format

```
YYYY-MM-DD HH:MM:SS LEVEL ServiceName message text [key=value ...]
```

```
2026-08-29 10:15:22 INFO AuthService User login successful user=145
2026-08-29 10:15:25 ERROR PaymentService Database timeout transaction=8932
```

## Skills Demonstrated

| Category | Technologies |
|----------|-------------|
| Language | C++20, constexpr, enum class, std::optional, string_view |
| Architecture | SOLID, dependency injection, interface segregation |
| Memory | RAII, smart pointers, move semantics, streaming |
| Concurrency | Thread pool (Phase 4) |
| Data Structures | unordered_map, vector, hash-based indexing (Phase 2) |
| Tooling | CMake, GoogleTest, Google Benchmark, clang-format, clang-tidy |
| CI/CD | GitHub Actions (Linux, macOS, Windows) |
| Documentation | Doxygen, architecture docs |

## Performance

LogForge uses streaming I/O with a 64 KB read buffer. The parser operates in O(n) per line with no heap allocations beyond the `LogEntry` itself. Benchmarks are in `benchmarks/`.

## Roadmap

- [x] Phase 1: Parser, streaming, search, statistics, CLI
- [x] Phase 2: Hash-based indexing for O(1) queries
- [ ] Phase 3: Advanced statistics and spike detection
- [ ] Phase 4: Multithreaded chunk processing with thread pool
- [ ] Phase 5: Live file monitoring (`watch` command)
- [ ] Phase 6: JSON configuration and output formats

## License

MIT — see [LICENSE](LICENSE).
