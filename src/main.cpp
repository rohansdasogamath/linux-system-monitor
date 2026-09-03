#include "sysmonitor/SystemMonitor.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <csignal>
#include <atomic>

using namespace sysmonitor;

static std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_running = false;
    }
}

// ANSI Escape Code Helpers
namespace color {
    constexpr const char* RESET   = "\033[0m";
    constexpr const char* BOLD    = "\033[1m";
    constexpr const char* DIM     = "\033[2m";
    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* BLUE    = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* WHITE   = "\033[37m";
    constexpr const char* CLEAR_SCREEN = "\033[2J\033[H";
}

std::string get_color_for_percent(double pct) {
    if (pct >= 85.0) return color::RED;
    if (pct >= 60.0) return color::YELLOW;
    return color::GREEN;
}

std::string make_progress_bar(double percent, int width = 25) {
    if (percent < 0.0) percent = 0.0;
    if (percent > 100.0) percent = 100.0;

    int filled = static_cast<int>((percent / 100.0) * static_cast<double>(width));
    std::string bar = "[";
    std::string col = get_color_for_percent(percent);

    bar += col;
    for (int i = 0; i < filled; ++i) {
        bar += "|";
    }
    bar += color::RESET;

    for (int i = filled; i < width; ++i) {
        bar += " ";
    }
    bar += "]";
    return bar;
}

std::string format_uptime(double seconds) {
    auto total_secs = static_cast<uint64_t>(seconds);
    uint64_t days = total_secs / 86400;
    uint64_t hours = (total_secs % 86400) / 3600;
    uint64_t mins = (total_secs % 3600) / 60;
    uint64_t secs = total_secs % 60;

    std::ostringstream oss;
    if (days > 0) oss << days << "d ";
    if (hours > 0 || days > 0) oss << hours << "h ";
    oss << mins << "m " << secs << "s";
    return oss.str();
}

void render_dashboard(const SystemSnapshot& snap) {
    std::cout << color::CLEAR_SCREEN;
    std::cout << color::BOLD << color::CYAN
              << "========================================================================================\n"
              << "                         LINUX SYSTEM MONITOR (C++17)                                   \n"
              << "========================================================================================\n"
              << color::RESET;

    // Header info
    std::cout << color::BOLD << " OS: " << color::RESET << snap.os_name
              << " | " << color::BOLD << "Kernel: " << color::RESET << snap.kernel_version
              << " | " << color::BOLD << "Uptime: " << color::RESET << format_uptime(snap.uptime_seconds)
              << "\n----------------------------------------------------------------------------------------\n";

    // CPU Section
    std::cout << color::BOLD << color::YELLOW << " [CPU USAGE] " << color::RESET << "\n";
    std::cout << "  Overall:  " << make_progress_bar(snap.overall_cpu_percent)
              << " " << std::fixed << std::setprecision(1) << std::setw(5)
              << get_color_for_percent(snap.overall_cpu_percent) << snap.overall_cpu_percent << "%" << color::RESET << "\n";

    if (!snap.per_core_cpu.empty()) {
        std::cout << "  Cores:    ";
        size_t count = 0;
        for (const auto& core : snap.per_core_cpu) {
            std::cout << core.name << ": "
                      << get_color_for_percent(core.usage_percent)
                      << std::fixed << std::setprecision(1) << core.usage_percent << "%"
                      << color::RESET << "  ";
            if (++count % 4 == 0 && count < snap.per_core_cpu.size()) {
                std::cout << "\n            ";
            }
        }
        std::cout << "\n";
    }

    // Memory Section
    std::cout << "\n" << color::BOLD << color::MAGENTA << " [MEMORY & SWAP] " << color::RESET << "\n";
    const auto& mem = snap.memory;
    double mem_pct = mem.get_usage_percent();
    std::cout << "  RAM:      " << make_progress_bar(mem_pct)
              << " " << std::fixed << std::setprecision(1) << std::setw(5)
              << get_color_for_percent(mem_pct) << mem_pct << "%" << color::RESET
              << " (" << format_bytes(mem.get_used_bytes()) << " / " << format_bytes(mem.total_bytes)
              << ", Avail: " << format_bytes(mem.available_bytes) << ")\n";

    if (mem.swap_total_bytes > 0) {
        double swap_pct = mem.get_swap_usage_percent();
        std::cout << "  Swap:     " << make_progress_bar(swap_pct)
                  << " " << std::fixed << std::setprecision(1) << std::setw(5)
                  << get_color_for_percent(swap_pct) << swap_pct << "%" << color::RESET
                  << " (" << format_bytes(mem.get_swap_used_bytes()) << " / " << format_bytes(mem.swap_total_bytes) << ")\n";
    }

    // Disks Section
    std::cout << "\n" << color::BOLD << color::BLUE << " [STORAGE & FILESYSTEMS] " << color::RESET << "\n";
    if (snap.disks.empty()) {
        std::cout << "  No physical filesystems detected.\n";
    } else {
        for (const auto& disk : snap.disks) {
            double disk_pct = disk.get_usage_percent();
            std::cout << "  " << std::left << std::setw(14) << disk.mount_point
                      << " " << make_progress_bar(disk_pct, 20)
                      << " " << std::right << std::fixed << std::setprecision(1) << std::setw(5)
                      << get_color_for_percent(disk_pct) << disk_pct << "%" << color::RESET
                      << " (" << format_bytes(disk.get_used_bytes()) << " / " << format_bytes(disk.total_bytes)
                      << " on " << disk.filesystem << ")\n";
        }
    }

    // Process Table Section
    std::cout << "\n" << color::BOLD << color::GREEN << " [RUNNING PROCESSES] " << color::RESET
              << " (Total listed: " << snap.processes.size() << ")\n";
    std::cout << color::BOLD << color::DIM
              << "  " << std::left
              << std::setw(8)  << "PID"
              << std::setw(12) << "USER"
              << std::setw(6)  << "STAT"
              << std::setw(9)  << "CPU%"
              << std::setw(9)  << "MEM%"
              << std::setw(12) << "RSS"
              << "COMMAND\n"
              << color::RESET;

    for (const auto& proc : snap.processes) {
        std::string cmd = proc.cmdline.substr(0, 36);
        std::cout << "  " << std::left
                  << std::setw(8)  << proc.pid
                  << std::setw(12) << proc.user.substr(0, 11)
                  << std::setw(6)  << proc.state
                  << std::right << std::fixed << std::setprecision(1)
                  << std::setw(7)  << proc.cpu_percent << "% "
                  << std::setw(7)  << proc.memory_percent << "% "
                  << std::left
                  << std::setw(12) << format_bytes(proc.memory_rss_bytes)
                  << cmd << "\n";
    }

    std::cout << color::DIM << "\n Press Ctrl+C to exit...\n" << color::RESET << std::flush;
}

void print_help(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  -s, --snapshot        Print a single snapshot and exit\n"
              << "  -i, --interval <sec>  Refresh interval in seconds (default: 1.0)\n"
              << "  -c, --count <n>       Exit after N snapshots (default: infinite)\n"
              << "  -t, --top <n>         Number of processes to display (default: 10)\n"
              << "  --sort <cpu|mem|pid>  Sort processes by metric (default: cpu)\n"
              << "  --procfs <path>       Custom procfs path (default: /proc)\n"
              << "  -h, --help            Show this help message\n";
}

int main(int argc, char* argv[]) {
    bool snapshot_mode = false;
    double interval_sec = 1.0;
    int max_count = -1;
    size_t top_n = 10;
    ProcessSortBy sort_by = ProcessSortBy::Cpu;
    std::string procfs_path = "/proc";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s" || arg == "--snapshot") {
            snapshot_mode = true;
        } else if ((arg == "-i" || arg == "--interval") && i + 1 < argc) {
            interval_sec = std::stod(argv[++i]);
        } else if ((arg == "-c" || arg == "--count") && i + 1 < argc) {
            max_count = std::stoi(argv[++i]);
        } else if ((arg == "-t" || arg == "--top") && i + 1 < argc) {
            top_n = static_cast<size_t>(std::stoul(argv[++i]));
        } else if (arg == "--sort" && i + 1 < argc) {
            std::string s = argv[++i];
            if (s == "mem" || s == "memory") sort_by = ProcessSortBy::Memory;
            else if (s == "pid") sort_by = ProcessSortBy::Pid;
            else sort_by = ProcessSortBy::Cpu;
        } else if (arg == "--procfs" && i + 1 < argc) {
            procfs_path = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto parser = std::make_shared<ProcParser>(procfs_path);
    SystemMonitor monitor(parser);

    // Initial baseline sample to establish tick baseline for delta calculation
    monitor.update();

    if (snapshot_mode) {
        // Sleep briefly so a non-zero CPU delta can be measured
        sleep_ms(250);
        monitor.update();
        render_dashboard(monitor.get_snapshot(top_n, sort_by));
        return 0;
    }

    int iterations = 0;
    while (g_running) {
        sleep_ms(static_cast<uint32_t>(interval_sec * 1000.0));
        if (!g_running) break;

        monitor.update();
        render_dashboard(monitor.get_snapshot(top_n, sort_by));

        iterations++;
        if (max_count > 0 && iterations >= max_count) {
            break;
        }
    }

    std::cout << "\nExiting System Monitor. Goodbye!\n";
    return 0;
}
