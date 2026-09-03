#include "TestHelper.hpp"
#include "sysmonitor/DiskMonitor.hpp"

using namespace sysmonitor;

void test_filesystem_filtering() {
    std::cout << "[TEST] Running test_filesystem_filtering...\n";

    // Physical storage types
    ASSERT_TRUE(DiskMonitor::is_physical_fs("/dev/sda1", "ext4"));
    ASSERT_TRUE(DiskMonitor::is_physical_fs("/dev/nvme0n1p2", "btrfs"));
    ASSERT_TRUE(DiskMonitor::is_physical_fs("/dev/mapper/vg0-root", "xfs"));
    ASSERT_TRUE(DiskMonitor::is_physical_fs("/dev/sdb1", "ntfs"));

    // Virtual / Pseudo filesystems
    ASSERT_FALSE(DiskMonitor::is_physical_fs("proc", "proc"));
    ASSERT_FALSE(DiskMonitor::is_physical_fs("sysfs", "sysfs"));
    ASSERT_FALSE(DiskMonitor::is_physical_fs("tmpfs", "tmpfs"));
    ASSERT_FALSE(DiskMonitor::is_physical_fs("udev", "devtmpfs"));
    ASSERT_FALSE(DiskMonitor::is_physical_fs("cgroup2", "cgroup2"));
    ASSERT_FALSE(DiskMonitor::is_physical_fs("/dev/loop0", "squashfs"));
}

void test_mount_parsing() {
    std::cout << "[TEST] Running test_mount_parsing...\n";

    std::vector<std::string> mount_lines = {
        "sysfs /sys sysfs rw,nosuid,nodev,noexec,relatime 0 0",
        "proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0",
        "udev /dev devtmpfs rw,nosuid,noexec,relatime 0 0",
        "tmpfs /run tmpfs rw,nosuid,nodev,noexec,relatime 0 0",
        "/dev/nvme0n1p2 / ext4 rw,relatime,errors=remount-ro 0 0",
        "/dev/sda1 /data xfs rw,relatime 0 0",
        "tmpfs /dev/shm tmpfs rw,nosuid,nodev 0 0"
    };

    auto mounts = DiskMonitor::parse_mounts(mount_lines);

    ASSERT_EQ(mounts.size(), 2ULL);
    ASSERT_STR_EQ(mounts[0].first, "/dev/nvme0n1p2");
    ASSERT_STR_EQ(mounts[0].second, "/");

    ASSERT_STR_EQ(mounts[1].first, "/dev/sda1");
    ASSERT_STR_EQ(mounts[1].second, "/data");
}

void test_disk_math() {
    std::cout << "[TEST] Running test_disk_math...\n";

    DiskInfo info{};
    info.filesystem = "/dev/sda1";
    info.mount_point = "/";
    info.total_bytes = 1000ULL * 1024ULL * 1024ULL * 1024ULL; // 1000 GB
    info.free_bytes  = 400ULL * 1024ULL * 1024ULL * 1024ULL;  // 400 GB
    info.available_bytes = 350ULL * 1024ULL * 1024ULL * 1024ULL;

    ASSERT_EQ(info.get_used_bytes(), 600ULL * 1024ULL * 1024ULL * 1024ULL);
    ASSERT_NEAR(info.get_usage_percent(), 60.0, 0.01);
}

int main() {
    test_filesystem_filtering();
    test_mount_parsing();
    test_disk_math();
    return test::summarize();
}
