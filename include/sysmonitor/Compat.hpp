#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Standard optional handling with GCC 6+ compatibility
#if defined(__has_include) && __has_include(<optional>)
  #include <optional>
  namespace sysmonitor {
      template <typename T>
      using optional = std::optional<T>;
      using std::nullopt;
  }
#elif defined(__has_include) && __has_include(<experimental/optional>)
  #include <experimental/optional>
  namespace sysmonitor {
      template <typename T>
      using optional = std::experimental::optional<T>;
      using std::experimental::nullopt;
  }
#else
  #error "A compiler supporting std::optional or std::experimental::optional is required."
#endif

// Standard filesystem handling
#if defined(__has_include) && __has_include(<filesystem>)
  #include <filesystem>
  namespace sysmonitor {
      namespace fs = std::filesystem;
  }
  #define SYSMONITOR_HAS_STD_FILESYSTEM 1
#elif defined(__has_include) && __has_include(<experimental/filesystem>)
  #include <experimental/filesystem>
  namespace sysmonitor {
      namespace fs = std::experimental::filesystem;
  }
  #define SYSMONITOR_HAS_STD_FILESYSTEM 1
#else
  #define SYSMONITOR_HAS_STD_FILESYSTEM 0
#endif

namespace sysmonitor {

struct StorageSpace {
    uint64_t capacity{0};
    uint64_t free{0};
    uint64_t available{0};
};

/**
 * @brief Portable space query that uses std::filesystem::space when available,
 * with POSIX statvfs and Windows API fallbacks.
 */
StorageSpace query_storage_space(const std::string& path);

/**
 * @brief Portable thread sleep helper.
 */
void sleep_ms(uint32_t ms);

} // namespace sysmonitor
