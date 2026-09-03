#pragma once

#include "sysmonitor/Compat.hpp"
#include <string>
#include <vector>

namespace sysmonitor {

/**
 * @brief Thread-safe file reader and parsing utility for Linux /proc and /sys filesystems.
 * Supports configurable root directory for deterministic testing with mock procfs fixtures.
 */
class ProcParser {
public:
    explicit ProcParser(std::string procfs_root = "/proc");
    virtual ~ProcParser() = default;

    // Procfs root accessors
    void set_procfs_root(std::string procfs_root);
    [[nodiscard]] const std::string& get_procfs_root() const noexcept;

    // File reading utilities
    [[nodiscard]] optional<std::string> read_file(const std::string& relative_path) const;
    [[nodiscard]] std::vector<std::string> read_lines(const std::string& relative_path) const;
    [[nodiscard]] bool file_exists(const std::string& relative_path) const;

    // Directory inspection
    [[nodiscard]] std::vector<std::string> list_subdirectories(const std::string& relative_path = "") const;

    // String manipulation helpers
    [[nodiscard]] static std::vector<std::string> tokenize(const std::string& line, char delimiter = ' ');
    [[nodiscard]] static std::string trim(const std::string& str);
    [[nodiscard]] static bool starts_with(const std::string& str, const std::string& prefix);
    [[nodiscard]] static optional<uint64_t> parse_kb_value(const std::string& line);

protected:
    [[nodiscard]] std::string build_path(const std::string& relative_path) const;

private:
    std::string procfs_root_;
};

} // namespace sysmonitor
