#include "sysmonitor/DiskMonitor.hpp"
#include "sysmonitor/Compat.hpp"

#include <unordered_set>
#include <array>
#include <algorithm>

namespace sysmonitor {

DiskMonitor::DiskMonitor(std::shared_ptr<ProcParser> parser)
    : parser_(std::move(parser)) {}

bool DiskMonitor::is_physical_fs(const std::string& device, const std::string& fstype) noexcept {
    // Whitelist of typical physical filesystems
    static constexpr std::array<const char*, 11> physical_types = {
        "ext4", "ext3", "ext2", "xfs", "btrfs", "zfs", "vfat", "fat", "ntfs", "fuseblk", "f2fs"
    };

    for (const auto* type : physical_types) {
        if (fstype == type) {
            return true;
        }
    }

    // Devices starting with /dev/ are physical block devices or LVM/RAID/dm-crypt volumes
    if (ProcParser::starts_with(device, "/dev/")) {
        // Exclude loop devices unless necessary
        if (ProcParser::starts_with(device, "/dev/loop")) {
            return false;
        }
        return true;
    }

    return false;
}

std::vector<std::pair<std::string, std::string>> DiskMonitor::parse_mounts(const std::vector<std::string>& lines) {
    std::vector<std::pair<std::string, std::string>> results;
    std::unordered_set<std::string> seen_mounts;

    for (const auto& line : lines) {
        auto tokens = ProcParser::tokenize(line, ' ');
        if (tokens.size() < 3) {
            continue;
        }

        const std::string& device = tokens[0];
        const std::string& mount_point = tokens[1];
        const std::string& fstype = tokens[2];

        if (is_physical_fs(device, fstype)) {
            if (seen_mounts.find(mount_point) == seen_mounts.end()) {
                seen_mounts.insert(mount_point);
                results.emplace_back(device, mount_point);
            }
        }
    }

    return results;
}

void DiskMonitor::update() {
    std::vector<DiskInfo> new_disks;
    auto lines = parser_->read_lines("mounts");
    auto mounts = parse_mounts(lines);

    // Fallback if no mounts discovered from procfs: inspect root directory
    if (mounts.empty()) {
        mounts.emplace_back("rootfs", "/");
    }

    std::unordered_set<std::string> processed_mounts;

    for (const auto& mount : mounts) {
        const std::string& device = mount.first;
        const std::string& mount_point = mount.second;

        if (processed_mounts.count(mount_point)) {
            continue;
        }

        auto space_info = query_storage_space(mount_point);
        if (space_info.capacity > 0) {
            DiskInfo info{};
            info.filesystem = device;
            info.mount_point = mount_point;
            info.total_bytes = space_info.capacity;
            info.free_bytes = space_info.free;
            info.available_bytes = space_info.available;

            new_disks.push_back(info);
            processed_mounts.insert(mount_point);
        }
    }

    disks_ = std::move(new_disks);
}

const std::vector<DiskInfo>& DiskMonitor::get_disks() const noexcept {
    return disks_;
}

} // namespace sysmonitor
