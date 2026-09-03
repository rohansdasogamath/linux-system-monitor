#pragma once

#include "sysmonitor/Types.hpp"
#include "sysmonitor/ProcParser.hpp"
#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace sysmonitor {

/**
 * @brief Reads /proc/mounts and queries filesystem space via std::filesystem::space.
 */
class DiskMonitor {
public:
    explicit DiskMonitor(std::shared_ptr<ProcParser> parser = std::make_shared<ProcParser>());

    /**
     * @brief Refreshes mount table and queries capacity and usage for each disk.
     */
    void update();

    [[nodiscard]] const std::vector<DiskInfo>& get_disks() const noexcept;

    /**
     * @brief Helper to filter out virtual pseudofs (proc, sysfs, tmpfs) from real block storage.
     */
    [[nodiscard]] static bool is_physical_fs(const std::string& device, const std::string& fstype) noexcept;

    /**
     * @brief Pure parsing helper for /proc/mounts lines.
     * Returns a list of (device, mount_point) pairs.
     */
    [[nodiscard]] static std::vector<std::pair<std::string, std::string>> parse_mounts(const std::vector<std::string>& lines);

private:
    std::shared_ptr<ProcParser> parser_;
    std::vector<DiskInfo> disks_{};
};

} // namespace sysmonitor
