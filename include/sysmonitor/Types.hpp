#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace sysmonitor {

/**
 * @brief Raw CPU time counters parsed from /proc/stat.
 * All units are in clock ticks (jiffies).
 */
struct CpuTimes {
    uint64_t user{0};
    uint64_t nice{0};
    uint64_t system{0};
    uint64_t idle{0};
    uint64_t iowait{0};
    uint64_t irq{0};
    uint64_t softirq{0};
    uint64_t steal{0};
    uint64_t guest{0};
    uint64_t guest_nice{0};

    [[nodiscard]] constexpr uint64_t get_idle_time() const noexcept {
        return idle + iowait;
    }

    [[nodiscard]] constexpr uint64_t get_active_time() const noexcept {
        return user + nice + system + irq + softirq + steal;
    }

    [[nodiscard]] constexpr uint64_t get_total_time() const noexcept {
        return get_idle_time() + get_active_time();
    }
};

/**
 * @brief Calculated CPU usage snapshot for total or specific core.
 */
struct CpuSnapshot {
    std::string name{"cpu"};
    double usage_percent{0.0};
    CpuTimes raw_times{};
};

/**
 * @brief Memory statistics parsed from /proc/meminfo.
 * Converted to bytes for consistent internal representation.
 */
struct MemoryInfo {
    uint64_t total_bytes{0};
    uint64_t free_bytes{0};
    uint64_t available_bytes{0};
    uint64_t buffers_bytes{0};
    uint64_t cached_bytes{0};
    uint64_t swap_total_bytes{0};
    uint64_t swap_free_bytes{0};

    [[nodiscard]] uint64_t get_used_bytes() const noexcept {
        return (total_bytes > available_bytes) ? (total_bytes - available_bytes) : 0;
    }

    [[nodiscard]] double get_usage_percent() const noexcept {
        if (total_bytes == 0) return 0.0;
        return (static_cast<double>(get_used_bytes()) / static_cast<double>(total_bytes)) * 100.0;
    }

    [[nodiscard]] uint64_t get_swap_used_bytes() const noexcept {
        return (swap_total_bytes > swap_free_bytes) ? (swap_total_bytes - swap_free_bytes) : 0;
    }

    [[nodiscard]] double get_swap_usage_percent() const noexcept {
        if (swap_total_bytes == 0) return 0.0;
        return (static_cast<double>(get_swap_used_bytes()) / static_cast<double>(swap_total_bytes)) * 100.0;
    }
};

/**
 * @brief Filesystem / mount point storage metrics.
 */
struct DiskInfo {
    std::string filesystem;
    std::string mount_point;
    uint64_t total_bytes{0};
    uint64_t free_bytes{0};
    uint64_t available_bytes{0};

    [[nodiscard]] uint64_t get_used_bytes() const noexcept {
        return (total_bytes > free_bytes) ? (total_bytes - free_bytes) : 0;
    }

    [[nodiscard]] double get_usage_percent() const noexcept {
        if (total_bytes == 0) return 0.0;
        return (static_cast<double>(get_used_bytes()) / static_cast<double>(total_bytes)) * 100.0;
    }
};

/**
 * @brief Linux process status and metrics extracted from /proc/[pid]/.
 */
struct ProcessInfo {
    int pid{0};
    int ppid{0};
    std::string name;
    std::string cmdline;
    std::string user;
    uint32_t uid{0};
    char state{'?'};
    double cpu_percent{0.0};
    uint64_t memory_rss_bytes{0};
    uint64_t memory_vms_bytes{0};
    double memory_percent{0.0};
    uint64_t utime_ticks{0};
    uint64_t stime_ticks{0};
    uint64_t start_time_ticks{0};
};

/**
 * @brief Unified system telemetry snapshot.
 */
struct SystemSnapshot {
    double overall_cpu_percent{0.0};
    std::vector<CpuSnapshot> per_core_cpu;
    MemoryInfo memory;
    std::vector<DiskInfo> disks;
    std::vector<ProcessInfo> processes;
    std::chrono::system_clock::time_point timestamp;
    double uptime_seconds{0.0};
    std::string os_name;
    std::string kernel_version;
};

/**
 * @brief Helper utility to format byte counts into human-readable strings (B, KB, MB, GB, TB).
 */
inline std::string format_bytes(uint64_t bytes) {
    constexpr const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double count = static_cast<double>(bytes);

    while (count >= 1024.0 && unit_idx < 4) {
        count /= 1024.0;
        unit_idx++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << count << " " << units[unit_idx];
    return oss.str();
}

} // namespace sysmonitor
