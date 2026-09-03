#include "sysmonitor/CpuMonitor.hpp"

#include <algorithm>
#include <sstream>

namespace sysmonitor {

CpuMonitor::CpuMonitor(std::shared_ptr<ProcParser> parser)
    : parser_(std::move(parser)) {}

double CpuMonitor::calculate_usage(const CpuTimes& prev, const CpuTimes& curr) noexcept {
    const uint64_t prev_active = prev.get_active_time();
    const uint64_t curr_active = curr.get_active_time();

    const uint64_t prev_idle = prev.get_idle_time();
    const uint64_t curr_idle = curr.get_idle_time();

    const uint64_t delta_active = (curr_active >= prev_active) ? (curr_active - prev_active) : 0;
    const uint64_t delta_idle = (curr_idle >= prev_idle) ? (curr_idle - prev_idle) : 0;
    const uint64_t delta_total = delta_active + delta_idle;

    if (delta_total == 0) {
        return 0.0;
    }

    double percent = (static_cast<double>(delta_active) / static_cast<double>(delta_total)) * 100.0;
    if (percent < 0.0) return 0.0;
    if (percent > 100.0) return 100.0;
    return percent;
}

bool CpuMonitor::parse_cpu_line(const std::string& line, std::string& out_name, CpuTimes& out_times) {
    auto tokens = ProcParser::tokenize(line, ' ');
    if (tokens.size() < 5 || !ProcParser::starts_with(tokens[0], "cpu")) {
        return false;
    }

    out_name = tokens[0];
    out_times = CpuTimes{};

    try {
        if (tokens.size() > 1) out_times.user = std::stoull(tokens[1]);
        if (tokens.size() > 2) out_times.nice = std::stoull(tokens[2]);
        if (tokens.size() > 3) out_times.system = std::stoull(tokens[3]);
        if (tokens.size() > 4) out_times.idle = std::stoull(tokens[4]);
        if (tokens.size() > 5) out_times.iowait = std::stoull(tokens[5]);
        if (tokens.size() > 6) out_times.irq = std::stoull(tokens[6]);
        if (tokens.size() > 7) out_times.softirq = std::stoull(tokens[7]);
        if (tokens.size() > 8) out_times.steal = std::stoull(tokens[8]);
        if (tokens.size() > 9) out_times.guest = std::stoull(tokens[9]);
        if (tokens.size() > 10) out_times.guest_nice = std::stoull(tokens[10]);
        return true;
    } catch (...) {
        return false;
    }
}

void CpuMonitor::update() {
    auto lines = parser_->read_lines("stat");
    if (lines.empty()) {
        return;
    }

    std::vector<CpuSnapshot> new_per_core;

    for (const auto& line : lines) {
        if (!ProcParser::starts_with(line, "cpu")) {
            continue;
        }

        std::string cpu_name;
        CpuTimes current_times;
        if (!parse_cpu_line(line, cpu_name, current_times)) {
            continue;
        }

        double usage = 0.0;
        auto it = prev_cpu_times_.find(cpu_name);
        if (it != prev_cpu_times_.end()) {
            usage = calculate_usage(it->second, current_times);
        }

        CpuSnapshot snapshot{cpu_name, usage, current_times};

        if (cpu_name == "cpu") {
            aggregate_cpu_ = snapshot;
        } else {
            new_per_core.push_back(snapshot);
        }

        prev_cpu_times_[cpu_name] = current_times;
    }

    per_core_cpu_ = std::move(new_per_core);
    has_previous_sample_ = true;
    last_sample_time_ = std::chrono::steady_clock::now();
}

CpuSnapshot CpuMonitor::get_aggregate_cpu() const {
    return aggregate_cpu_;
}

const std::vector<CpuSnapshot>& CpuMonitor::get_per_core_cpu() const noexcept {
    return per_core_cpu_;
}

size_t CpuMonitor::get_core_count() const noexcept {
    return per_core_cpu_.size();
}

} // namespace sysmonitor
