#include "sysmonitor/Compat.hpp"

#if SYSMONITOR_HAS_STD_FILESYSTEM
#include <system_error>

namespace sysmonitor {
StorageSpace query_storage_space(const std::string& path) {
    StorageSpace s{};
    std::error_code ec;
    auto sp = fs::space(path, ec);
    if (!ec) {
        s.capacity = sp.capacity;
        s.free = sp.free;
        s.available = sp.available;
    }
    return s;
}
}

#elif defined(_WIN32)
#include <windows.h>

namespace sysmonitor {
StorageSpace query_storage_space(const std::string& path) {
    StorageSpace s{};
    ULARGE_INTEGER free_bytes_avail, total_bytes, total_free_bytes;
    std::string p = path;
    if (p == "/") p = "C:\\";

    if (GetDiskFreeSpaceExA(p.empty() ? nullptr : p.c_str(),
                           &free_bytes_avail, &total_bytes, &total_free_bytes)) {
        s.capacity = total_bytes.QuadPart;
        s.free = total_free_bytes.QuadPart;
        s.available = free_bytes_avail.QuadPart;
    }
    return s;
}
}

#elif defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <sys/statvfs.h>

namespace sysmonitor {
StorageSpace query_storage_space(const std::string& path) {
    StorageSpace s{};
    struct statvfs stat{};
    if (statvfs(path.c_str(), &stat) == 0) {
        s.capacity = static_cast<uint64_t>(stat.f_blocks) * stat.f_frsize;
        s.free = static_cast<uint64_t>(stat.f_bfree) * stat.f_frsize;
        s.available = static_cast<uint64_t>(stat.f_bavail) * stat.f_frsize;
    }
    return s;
}
}

#else
namespace sysmonitor {
StorageSpace query_storage_space(const std::string&) {
    return StorageSpace{};
}
}
#endif

#if defined(_WIN32)
#include <windows.h>
namespace sysmonitor {
void sleep_ms(uint32_t ms) {
    Sleep(static_cast<DWORD>(ms));
}
}
#else
#include <thread>
#include <chrono>
namespace sysmonitor {
void sleep_ms(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
}
#endif
