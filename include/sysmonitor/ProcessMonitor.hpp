#pragma once

#include "sysmonitor/Types.hpp"
#include "sysmonitor/ProcParser.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace sysmonitor {

enum class ProcessSortBy {
    Cpu,
    Memory,
    Pid
};

/**
 * @brief Discovers and tracks metrics for running processes in /proc/[pid]/.
 */
class ProcessMonitor {
public:
    explicit ProcessMonitor(std::shared_ptr<ProcParser> parser = std::make_shared<ProcParser>());

    /**
     * @brief Iterates over all active PIDs, parses process stats, and calculates CPU/RAM usage.
     * @param total_memory_bytes Total host memory (used to calculate memory percentage).
     * @param system_cpu_delta_ticks Total system CPU ticks delta across all cores.
     */
    void update(uint64_t total_memory_bytes = 0, uint64_t system_cpu_delta_ticks = 0);

    [[nodiscard]] const std::vector<ProcessInfo>& get_processes() const noexcept;
    [[nodiscard]] std::vector<ProcessInfo> get_top_processes(size_t limit, ProcessSortBy sort_by = ProcessSortBy::Cpu) const;

    /**
     * @brief Parse a single /proc/[pid]/stat line.
     * Safely handles process names containing spaces and parentheses.
     */
    [[nodiscard]] static bool parse_stat_line(
        const std::string& line,
        int& out_pid,
        std::string& out_comm,
        char& out_state,
        int& out_ppid,
        uint64_t& out_utime,
        uint64_t& out_stime,
        uint64_t& out_starttime
    );

    /**
     * @brief Parse /proc/[pid]/cmdline (null-byte delimited arguments).
     */
    [[nodiscard]] static std::string parse_cmdline(const std::string& raw_cmdline);

    /**
     * @brief Convert UID to username (uses getpwuid_r on POSIX, fallback to string UID).
     */
    [[nodiscard]] static std::string uid_to_username(uint32_t uid);

private:
    std::shared_ptr<ProcParser> parser_;
    std::vector<ProcessInfo> processes_{};

    // Tracking ticks between samples for CPU calculation (keyed by PID)
    struct ProcessPrevState {
        uint64_t total_ticks{0};
        std::chrono::steady_clock::time_point sample_time;
    };
    std::unordered_map<int, ProcessPrevState> prev_process_ticks_{};
};

} // namespace sysmonitor
