#pragma once

#include "sysmonitor/Types.hpp"
#include "sysmonitor/ProcParser.hpp"
#include <memory>
#include <unordered_map>
#include <chrono>

namespace sysmonitor {

/**
 * @brief Reads /proc/stat to monitor total and per-core CPU utilization.
 * Calculates usage as a time-delta ratio between consecutive sampling intervals.
 */
class CpuMonitor {
public:
    explicit CpuMonitor(std::shared_ptr<ProcParser> parser = std::make_shared<ProcParser>());

    /**
     * @brief Reads /proc/stat, calculates the delta since the previous sample,
     * and updates internal CPU snapshots.
     */
    void update();

    [[nodiscard]] CpuSnapshot get_aggregate_cpu() const;
    [[nodiscard]] const std::vector<CpuSnapshot>& get_per_core_cpu() const noexcept;
    [[nodiscard]] size_t get_core_count() const noexcept;

    /**
     * @brief Pure calculation helper that computes CPU usage percentage given two time samples.
     * Guaranteed no side effects; ideal for unit testing.
     */
    [[nodiscard]] static double calculate_usage(const CpuTimes& prev, const CpuTimes& curr) noexcept;

    /**
     * @brief Parse a single line from /proc/stat into a CpuTimes struct.
     */
    [[nodiscard]] static bool parse_cpu_line(const std::string& line, std::string& out_name, CpuTimes& out_times);

private:
    std::shared_ptr<ProcParser> parser_;
    CpuSnapshot aggregate_cpu_{};
    std::vector<CpuSnapshot> per_core_cpu_{};

    // History map for delta calculations (keyed by CPU name e.g. "cpu", "cpu0")
    std::unordered_map<std::string, CpuTimes> prev_cpu_times_{};
    bool has_previous_sample_{false};
    std::chrono::steady_clock::time_point last_sample_time_{};
};

} // namespace sysmonitor
