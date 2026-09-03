#include "sysmonitor/SystemMonitor.hpp"

#include <fstream>
#include <sstream>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <sys/utsname.h>
#endif

namespace sysmonitor {

SystemMonitor::SystemMonitor(std::shared_ptr<ProcParser> parser)
    : parser_(std::move(parser)),
      cpu_monitor_(std::make_unique<CpuMonitor>(parser_)),
      memory_monitor_(std::make_unique<MemoryMonitor>(parser_)),
      disk_monitor_(std::make_unique<DiskMonitor>(parser_)),
      process_monitor_(std::make_unique<ProcessMonitor>(parser_)) {}

double SystemMonitor::parse_uptime(const std::string& uptime_line) {
    auto tokens = ProcParser::tokenize(uptime_line, ' ');
    if (tokens.empty()) {
        return 0.0;
    }
    try {
        return std::stod(tokens[0]);
    } catch (...) {
        return 0.0;
    }
}

std::string SystemMonitor::parse_os_name(const std::vector<std::string>& os_release_lines) {
    for (const auto& line : os_release_lines) {
        if (ProcParser::starts_with(line, "PRETTY_NAME=")) {
            std::string val = line.substr(12);
            if (!val.empty() && val.front() == '"') val.erase(0, 1);
            if (!val.empty() && val.back() == '"') val.pop_back();
            return val;
        }
    }
    return "Linux";
}

double SystemMonitor::get_uptime_seconds() const {
    auto content = parser_->read_file("uptime");
    if (!content) {
        return 0.0;
    }
    return parse_uptime(*content);
}

std::string SystemMonitor::get_os_name() const {
    // Try reading /etc/os-release or root-relative etc/os-release
    std::ifstream file("/etc/os-release");
    if (file.is_open()) {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        return parse_os_name(lines);
    }
    return "Linux Ubuntu";
}

std::string SystemMonitor::get_kernel_version() const {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    struct utsname buffer{};
    if (uname(&buffer) == 0) {
        return std::string(buffer.sysname) + " " + std::string(buffer.release);
    }
#endif
    auto version_str = parser_->read_file("version");
    if (version_str) {
        auto tokens = ProcParser::tokenize(*version_str, ' ');
        if (tokens.size() >= 3) {
            return tokens[0] + " " + tokens[2];
        }
    }
    return "Linux Kernel";
}

void SystemMonitor::update() {
    cpu_monitor_->update();
    memory_monitor_->update();
    disk_monitor_->update();

    const auto agg_cpu = cpu_monitor_->get_aggregate_cpu();
    const uint64_t curr_system_total = agg_cpu.raw_times.get_total_time();
    const uint64_t delta_ticks = (curr_system_total >= prev_system_total_ticks_)
                                 ? (curr_system_total - prev_system_total_ticks_)
                                 : 0;
    prev_system_total_ticks_ = curr_system_total;

    process_monitor_->update(memory_monitor_->get_memory_info().total_bytes, delta_ticks);
}

SystemSnapshot SystemMonitor::get_snapshot(size_t process_limit, ProcessSortBy sort_by) const {
    SystemSnapshot snapshot{};
    snapshot.overall_cpu_percent = cpu_monitor_->get_aggregate_cpu().usage_percent;
    snapshot.per_core_cpu = cpu_monitor_->get_per_core_cpu();
    snapshot.memory = memory_monitor_->get_memory_info();
    snapshot.disks = disk_monitor_->get_disks();
    snapshot.processes = process_monitor_->get_top_processes(process_limit, sort_by);
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.uptime_seconds = get_uptime_seconds();
    snapshot.os_name = get_os_name();
    snapshot.kernel_version = get_kernel_version();
    return snapshot;
}

} // namespace sysmonitor
