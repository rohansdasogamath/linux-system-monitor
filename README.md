# Linux System Monitor

[![CI Pipeline](https://github.com/rohansdasogamath/Linux-System-Monitor/actions/workflows/ci.yml/badge.svg)](https://github.com/rohansdasogamath/Linux-System-Monitor/actions/workflows/ci.yml)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Build](https://img.shields.io/badge/Build-CMake%203.16+-brightgreen.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20Ubuntu-orange.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

A modular, high-performance Linux System Monitor written in modern **C++17** targeting **Linux Ubuntu**. This project interfaces directly with Linux virtual filesystems (`/proc` and `/sys`) to monitor CPU, memory, disk, and active process telemetry without third-party dependencies.

Designed with production-grade engineering principles: **dependency injection for testability**, **RAII resource management**, **modular procfs parsing**, **strict compiler warnings**, and **automated CTest test suites on GitHub Actions CI**.

---

## Key Features

- **CPU Utilization Engine**: Time-delta sampling across overall processor and per-core threads via `/proc/stat`.
- **Memory & Swap Tracking**: Accurate physical RAM and swap monitoring using `/proc/meminfo` (`MemAvailable` heuristic with legacy kernel fallbacks).
- **Storage & Filesystems**: Real block filesystem discovery via `/proc/mounts` and storage capacity querying via `std::filesystem::space`.
- **Process Telemetry**: Active PID scanning, status parsing (`/proc/[pid]/stat`, `/proc/[pid]/status`, `/proc/[pid]/cmdline`), CPU/RAM consumption calculations, and UID resolution.
- **Terminal UI & Snapshot Mode**: ANSI color-coded progress bars and formatted tables; includes `--snapshot` for scripting and CI assertions.
- **100% Testable Architecture**: Abstracted procfs root enables deterministic testing using mock procfs fixtures across any operating system or container.

---

## System Architecture

```
+-----------------------------------------------------------------------------+
|                                CLI Layer                                    |
|                       (src/main.cpp - ANSI View)                            |
+-----------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------+
|                        SystemMonitor (Coordinator)                          |
|             (Snapshot aggregation, uptime, kernel & OS details)             |
+-----------------------------------------------------------------------------+
      |                       |                       |                      |
      v                       v                       v                      v
+------------+         +---------------+        +-------------+        +---------------+
| CpuMonitor |         | MemoryMonitor |        | DiskMonitor |        | ProcessMonitor|
+------------+         +---------------+        +-------------+        +---------------+
      \                       /                       |                      /
       \                     /                        |                     /
        v                   v                         v                    v
+-----------------------------------------------------------------------------+
|                        ProcParser & File Utilities                          |
|            (/proc/stat, /proc/meminfo, /proc/mounts, /proc/[pid]/*)         |
+-----------------------------------------------------------------------------+
```

---

## Linux `/proc` Kernel Interfaces

| Metric | Kernel Interface | Technical Detail |
| :--- | :--- | :--- |
| **CPU Usage** | `/proc/stat` | CPU utilization cannot be derived from a single instantaneous sample. The engine samples counters at $t_1$ and $t_2$, computing $\Delta \text{Active} / \Delta \text{Total} \times 100\%$. |
| **Memory** | `/proc/meminfo` | Avoids misleading `MemFree`. Instead reads `MemAvailable` (which accounts for reclaimable slab and page cache). $\text{Used} = \text{Total} - \text{Available}$. |
| **Disk** | `/proc/mounts` | Discovers active mount points, filters virtual pseudofs (`proc`, `sysfs`, `tmpfs`, `cgroup`), and inspects block devices via `std::filesystem::space`. |
| **Processes** | `/proc/[pid]/` | Scans numeric directories. Parses `comm` and jiffies from `stat`, RSS/VMS/UID from `status`, and null-delimited arguments from `cmdline`. |

---

## Directory Layout

```
linux-system-monitor/
├── .github/
│   └── workflows/
│       └── ci.yml             # Matrix CI testing on Ubuntu (GCC & Clang)
├── .gitignore                 # Standard CMake and build artifacts ignore
├── CMakeLists.txt             # Root CMake configuration (enforces C++17)
├── LICENSE                    # MIT License
├── README.md                  # Technical architecture and documentation
├── scripts/
│   └── build.sh               # Shell automation script for builds and tests
├── include/
│   └── sysmonitor/
│       ├── Types.hpp          # Domain structs (CpuSnapshot, MemoryInfo, etc.)
│       ├── Compat.hpp         # Cross-platform compiler compatibility shims
│       ├── ProcParser.hpp     # Procfs file reading & tokenization
│       ├── CpuMonitor.hpp     # CPU time-delta calculations
│       ├── MemoryMonitor.hpp  # Memory usage parser
│       ├── DiskMonitor.hpp    # Filesystem & storage metrics
│       ├── ProcessMonitor.hpp # PID discovery and process metrics
│       └── SystemMonitor.hpp  # Unified system coordinator
├── src/
│   ├── CMakeLists.txt         # Library and binary targets
│   ├── Compat.cpp
│   ├── ProcParser.cpp
│   ├── CpuMonitor.cpp
│   ├── MemoryMonitor.cpp
│   ├── DiskMonitor.cpp
│   ├── ProcessMonitor.cpp
│   ├── SystemMonitor.cpp
│   └── main.cpp               # Interactive CLI and snapshot tool
└── tests/
    ├── CMakeLists.txt         # CTest registration
    ├── TestHelper.hpp         # Lightweight zero-dependency test macros
    ├── test_cpu.cpp           # CPU formula & parser unit tests
    ├── test_memory.cpp        # Meminfo & fallback calculation tests
    ├── test_disk.cpp          # Mount filter and capacity tests
    ├── test_process.cpp       # Process tokenization and stat tests
    └── data/                  # Realistic mock /proc fixtures
        └── proc/
            ├── stat
            ├── meminfo
            ├── mounts
            ├── uptime
            ├── version
            ├── 1337/
            └── 2048/
```

---

## Building and Running

### Prerequisites
- **Compiler**: GCC 9+ or Clang 10+ (supporting C++17)
- **Build System**: CMake 3.16+
- **Build Tools**: Make or Ninja

### Build Instructions

```bash
# 1. Clone the repository
git clone https://github.com/rohandsdasogamath/Linux-System-Monitor.git
cd Linux-System-Monitor

# 2. Configure with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile
cmake --build build --config Release -j$(nproc)
```

Alternatively, use the provided build script:
```bash
./scripts/build.sh
```

---

## Running the Application

### Live Dashboard Mode
Launch the interactive live terminal dashboard (refreshes every second):
```bash
./build/bin/sysmonitor
```

### Snapshot Mode (Single Run)
Print an instantaneous snapshot and exit (ideal for scripts and automated checks):
```bash
./build/bin/sysmonitor --snapshot
```

### CLI Command Options

```
Usage: ./sysmonitor [options]

Options:
  -s, --snapshot        Print a single snapshot and exit
  -i, --interval <sec>  Refresh interval in seconds (default: 1.0)
  -c, --count <n>       Exit after N snapshots (default: infinite)
  -t, --top <n>         Number of processes to display (default: 10)
  --sort <cpu|mem|pid>  Sort processes by metric (default: cpu)
  --procfs <path>       Custom procfs path (default: /proc)
  -h, --help            Show this help message
```

---

## Running Tests with CTest

The project includes unit tests for every subsystem. The test suite uses mock `/proc` directory fixtures to verify parsing logic without needing root privileges.

```bash
# Build with tests enabled
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Execute test suite via CTest
ctest --test-dir build --output-on-failure --verbose
```

Or via the build script:
```bash
./scripts/build.sh test
```

---

## Continuous Integration (CI)

GitHub Actions runs an automated workflow on every push and pull request across:
- **Environments**: `ubuntu-22.04`, `ubuntu-latest`
- **Compilers**: GCC and Clang
- **Build Types**: Release and Debug
- **Validation**: Strict compiler warnings (`-Wall -Wextra -Wpedantic -Wconversion`), CTest test execution, and smoke testing against live `/proc`.

---

## License

MIT License. See LICENSE file for details.
