#pragma once

#include "sysmonitor/Types.hpp"
#include "sysmonitor/ProcParser.hpp"
#include <memory>
#include <vector>
#include <string>

namespace sysmonitor {

/**
 * @brief Reads /proc/meminfo to track physical RAM and swap space utilization.
 */
class MemoryMonitor {
public:
    explicit MemoryMonitor(std::shared_ptr<ProcParser> parser = std::make_shared<ProcParser>());

    /**
     * @brief Reads /proc/meminfo and updates internal memory statistics.
     */
    void update();

    [[nodiscard]] const MemoryInfo& get_memory_info() const noexcept;

    /**
     * @brief Pure parsing helper for /proc/meminfo lines.
     * Guaranteed no side effects; ideal for unit testing.
     */
    [[nodiscard]] static MemoryInfo parse_meminfo(const std::vector<std::string>& lines);

private:
    std::shared_ptr<ProcParser> parser_;
    MemoryInfo memory_info_{};
};

} // namespace sysmonitor
