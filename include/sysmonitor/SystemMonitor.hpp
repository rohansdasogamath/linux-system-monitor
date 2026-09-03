#pragma once

#include "sysmonitor/Types.hpp"
#include "sysmonitor/ProcParser.hpp"
#include "sysmonitor/CpuMonitor.hpp"
#include "sysmonitor/MemoryMonitor.hpp"
#include "sysmonitor/DiskMonitor.hpp"
#include "sysmonitor/ProcessMonitor.hpp"
#include <memory>
#include <string>

namespace sysmonitor {

/**
 * @brief High-level facade and orchestrator that coordinates all subsystem monitors
 * and captures unified system snapshots.
 */
class SystemMonitor {
public:
    explicit SystemMonitor(std::shared_ptr<ProcParser> parser = std::make_shared<ProcParser>());

    /**
     * @brief Polls all subsystem monitors and updates system state.
     */
    void update();

    [[nodiscard]] SystemSnapshot get_snapshot(size_t process_limit = 20, ProcessSortBy sort_by = ProcessSortBy::Cpu) const;

    [[nodiscard]] double get_uptime_seconds() const;
    [[nodiscard]] std::string get_os_name() const;
    [[nodiscard]] std::string get_kernel_version() const;

    // Subsystem accessors
    [[nodiscard]] CpuMonitor& cpu() noexcept { return *cpu_monitor_; }
    [[nodiscard]] MemoryMonitor& memory() noexcept { return *memory_monitor_; }
    [[nodiscard]] DiskMonitor& disk() noexcept { return *disk_monitor_; }
    [[nodiscard]] ProcessMonitor& process() noexcept { return *process_monitor_; }

    [[nodiscard]] static double parse_uptime(const std::string& uptime_line);
    [[nodiscard]] static std::string parse_os_name(const std::vector<std::string>& os_release_lines);

private:
    std::shared_ptr<ProcParser> parser_;
    std::unique_ptr<CpuMonitor> cpu_monitor_;
    std::unique_ptr<MemoryMonitor> memory_monitor_;
    std::unique_ptr<DiskMonitor> disk_monitor_;
    std::unique_ptr<ProcessMonitor> process_monitor_;

    uint64_t prev_system_total_ticks_{0};
};

} // namespace sysmonitor
