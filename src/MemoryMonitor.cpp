#include "sysmonitor/MemoryMonitor.hpp"

namespace sysmonitor {

MemoryMonitor::MemoryMonitor(std::shared_ptr<ProcParser> parser)
    : parser_(std::move(parser)) {}

MemoryInfo MemoryMonitor::parse_meminfo(const std::vector<std::string>& lines) {
    MemoryInfo mem{};
    bool found_available = false;

    for (const auto& line : lines) {
        if (ProcParser::starts_with(line, "MemTotal:")) {
            if (auto bytes = ProcParser::parse_kb_value(line)) mem.total_bytes = *bytes;
        } else if (ProcParser::starts_with(line, "MemFree:")) {
            if (auto bytes = ProcParser::parse_kb_value(line)) mem.free_bytes = *bytes;
        } else if (ProcParser::starts_with(line, "MemAvailable:")) {
            if (auto bytes = ProcParser::parse_kb_value(line)) {
                mem.available_bytes = *bytes;
                found_available = true;
            }
        } else if (ProcParser::starts_with(line, "Buffers:")) {
            if (auto bytes = ProcParser::parse_kb_value(line)) mem.buffers_bytes = *bytes;
        } else if (ProcParser::starts_with(line, "Cached:")) {
            if (auto bytes = ProcParser::parse_kb_value(line)) mem.cached_bytes = *bytes;
        } else if (ProcParser::starts_with(line, "SwapTotal:")) {
            if (auto bytes = ProcParser::parse_kb_value(line)) mem.swap_total_bytes = *bytes;
        } else if (ProcParser::starts_with(line, "SwapFree:")) {
            if (auto bytes = ProcParser::parse_kb_value(line)) mem.swap_free_bytes = *bytes;
        }
    }

    // Fallback for older kernels lacking MemAvailable: MemFree + Buffers + Cached
    if (!found_available && mem.total_bytes > 0) {
        mem.available_bytes = mem.free_bytes + mem.buffers_bytes + mem.cached_bytes;
        if (mem.available_bytes > mem.total_bytes) {
            mem.available_bytes = mem.total_bytes;
        }
    }

    return mem;
}

void MemoryMonitor::update() {
    auto lines = parser_->read_lines("meminfo");
    if (!lines.empty()) {
        memory_info_ = parse_meminfo(lines);
    }
}

const MemoryInfo& MemoryMonitor::get_memory_info() const noexcept {
    return memory_info_;
}

} // namespace sysmonitor
