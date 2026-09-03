#include "sysmonitor/ProcessMonitor.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#endif

namespace sysmonitor {

ProcessMonitor::ProcessMonitor(std::shared_ptr<ProcParser> parser)
    : parser_(std::move(parser)) {}

bool ProcessMonitor::parse_stat_line(
    const std::string& line,
    int& out_pid,
    std::string& out_comm,
    char& out_state,
    int& out_ppid,
    uint64_t& out_utime,
    uint64_t& out_stime,
    uint64_t& out_starttime
) {
    const auto open_paren = line.find('(');
    const auto close_paren = line.rfind(')');

    if (open_paren == std::string::npos || close_paren == std::string::npos || close_paren <= open_paren) {
        return false;
    }

    try {
        out_pid = std::stoi(ProcParser::trim(line.substr(0, open_paren)));
        out_comm = line.substr(open_paren + 1, close_paren - open_paren - 1);

        const std::string rest = line.substr(close_paren + 1);
        const auto tokens = ProcParser::tokenize(rest, ' ');

        // Expected tokens after ')' start from index 0 (which is state) to at least starttime (index 19)
        if (tokens.size() < 20) {
            return false;
        }

        out_state = tokens[0].empty() ? '?' : tokens[0][0];
        out_ppid = std::stoi(tokens[1]);
        out_utime = std::stoull(tokens[11]);
        out_stime = std::stoull(tokens[12]);
        out_starttime = std::stoull(tokens[19]);

        return true;
    } catch (...) {
        return false;
    }
}

std::string ProcessMonitor::parse_cmdline(const std::string& raw_cmdline) {
    if (raw_cmdline.empty()) {
        return "";
    }

    std::string result;
    result.reserve(raw_cmdline.size());

    for (char ch : raw_cmdline) {
        if (ch == '\0') {
            result += ' ';
        } else {
            result += ch;
        }
    }

    return ProcParser::trim(result);
}

std::string ProcessMonitor::uid_to_username(uint32_t uid) {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    struct passwd pwd{};
    struct passwd* result = nullptr;
    char buffer[1024];

    int status = getpwuid_r(static_cast<uid_t>(uid), &pwd, buffer, sizeof(buffer), &result);
    if (status == 0 && result != nullptr && result->pw_name != nullptr) {
        return std::string(result->pw_name);
    }
#endif
    return std::to_string(uid);
}

void ProcessMonitor::update(uint64_t total_memory_bytes, uint64_t system_cpu_delta_ticks) {
    std::vector<ProcessInfo> new_processes;
    const auto now = std::chrono::steady_clock::now();

    // Discover numeric PID directories in /proc
    const auto dirs = parser_->list_subdirectories("");

    for (const auto& dir_path_str : dirs) {
        const size_t last_sep = dir_path_str.find_last_of("/\\");
        const std::string dir_name = (last_sep == std::string::npos)
                                     ? dir_path_str
                                     : dir_path_str.substr(last_sep + 1);

        // Check if directory name is purely numeric (indicating a PID)
        if (dir_name.empty() || !std::all_of(dir_name.begin(), dir_name.end(), ::isdigit)) {
            continue;
        }

        const std::string pid_str = dir_name;
        const std::string stat_path = pid_str + "/stat";
        const auto stat_lines = parser_->read_lines(stat_path);
        if (stat_lines.empty()) {
            continue;
        }

        ProcessInfo proc{};
        if (!parse_stat_line(
                stat_lines[0],
                proc.pid,
                proc.name,
                proc.state,
                proc.ppid,
                proc.utime_ticks,
                proc.stime_ticks,
                proc.start_time_ticks)) {
            continue;
        }

        // Read cmdline
        const auto raw_cmd = parser_->read_file(pid_str + "/cmdline");
        proc.cmdline = raw_cmd ? parse_cmdline(*raw_cmd) : "";
        if (proc.cmdline.empty()) {
            proc.cmdline = "[" + proc.name + "]";
        }

        // Read status for UID and Memory (VmRSS, VmSize)
        const auto status_lines = parser_->read_lines(pid_str + "/status");
        for (const auto& status_line : status_lines) {
            if (ProcParser::starts_with(status_line, "Uid:")) {
                const auto tokens = ProcParser::tokenize(status_line, '\t');
                if (tokens.size() > 1) {
                    try {
                        proc.uid = static_cast<uint32_t>(std::stoul(ProcParser::trim(tokens[1])));
                    } catch (...) {}
                }
            } else if (ProcParser::starts_with(status_line, "VmRSS:")) {
                if (auto bytes = ProcParser::parse_kb_value(status_line)) {
                    proc.memory_rss_bytes = *bytes;
                }
            } else if (ProcParser::starts_with(status_line, "VmSize:")) {
                if (auto bytes = ProcParser::parse_kb_value(status_line)) {
                    proc.memory_vms_bytes = *bytes;
                }
            }
        }

        proc.user = uid_to_username(proc.uid);

        // Memory percentage calculation
        if (total_memory_bytes > 0) {
            proc.memory_percent = (static_cast<double>(proc.memory_rss_bytes) /
                                   static_cast<double>(total_memory_bytes)) * 100.0;
        }

        // CPU percentage calculation over time delta
        const uint64_t current_ticks = proc.utime_ticks + proc.stime_ticks;
        auto prev_it = prev_process_ticks_.find(proc.pid);

        if (prev_it != prev_process_ticks_.end() && system_cpu_delta_ticks > 0) {
            const uint64_t prev_ticks = prev_it->second.total_ticks;
            if (current_ticks >= prev_ticks) {
                const uint64_t delta_ticks = current_ticks - prev_ticks;
                proc.cpu_percent = (static_cast<double>(delta_ticks) /
                                    static_cast<double>(system_cpu_delta_ticks)) * 100.0;
            }
        }

        prev_process_ticks_[proc.pid] = ProcessPrevState{current_ticks, now};
        new_processes.push_back(proc);
    }

    // Clean up stale PIDs from tracking history
    if (prev_process_ticks_.size() > new_processes.size() * 2) {
        std::unordered_map<int, ProcessPrevState> pruned;
        for (const auto& proc : new_processes) {
            auto it = prev_process_ticks_.find(proc.pid);
            if (it != prev_process_ticks_.end()) {
                pruned[proc.pid] = it->second;
            }
        }
        prev_process_ticks_ = std::move(pruned);
    }

    processes_ = std::move(new_processes);
}

const std::vector<ProcessInfo>& ProcessMonitor::get_processes() const noexcept {
    return processes_;
}

std::vector<ProcessInfo> ProcessMonitor::get_top_processes(size_t limit, ProcessSortBy sort_by) const {
    auto sorted = processes_;

    switch (sort_by) {
        case ProcessSortBy::Cpu:
            std::sort(sorted.begin(), sorted.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                return a.cpu_percent > b.cpu_percent;
            });
            break;
        case ProcessSortBy::Memory:
            std::sort(sorted.begin(), sorted.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                return a.memory_rss_bytes > b.memory_rss_bytes;
            });
            break;
        case ProcessSortBy::Pid:
            std::sort(sorted.begin(), sorted.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
                return a.pid < b.pid;
            });
            break;
    }

    if (sorted.size() > limit) {
        sorted.resize(limit);
    }
    return sorted;
}

} // namespace sysmonitor
