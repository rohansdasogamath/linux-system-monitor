#include "sysmonitor/ProcParser.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#if !SYSMONITOR_HAS_STD_FILESYSTEM
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace sysmonitor {

ProcParser::ProcParser(std::string procfs_root)
    : procfs_root_(std::move(procfs_root)) {}

void ProcParser::set_procfs_root(std::string procfs_root) {
    procfs_root_ = std::move(procfs_root);
}

const std::string& ProcParser::get_procfs_root() const noexcept {
    return procfs_root_;
}

std::string ProcParser::build_path(const std::string& relative_path) const {
    if (relative_path.empty()) {
        return procfs_root_;
    }

    char sep = '/';
#if defined(_WIN32)
    sep = '\\';
#endif

    std::string result = procfs_root_;
    if (!result.empty() && result.back() != '/' && result.back() != '\\') {
        result += sep;
    }

    if (!relative_path.empty() && (relative_path.front() == '/' || relative_path.front() == '\\')) {
        result += relative_path.substr(1);
    } else {
        result += relative_path;
    }
    return result;
}

optional<std::string> ProcParser::read_file(const std::string& relative_path) const {
    std::string full_path = build_path(relative_path);
    std::ifstream file(full_path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return nullopt;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::vector<std::string> ProcParser::read_lines(const std::string& relative_path) const {
    std::vector<std::string> lines;
    std::string full_path = build_path(relative_path);
    std::ifstream file(full_path);
    if (!file.is_open()) {
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

bool ProcParser::file_exists(const std::string& relative_path) const {
    std::ifstream f(build_path(relative_path));
    return f.good();
}

std::vector<std::string> ProcParser::list_subdirectories(const std::string& relative_path) const {
    std::vector<std::string> dirs;
    std::string target_path = build_path(relative_path);

#if SYSMONITOR_HAS_STD_FILESYSTEM
    std::error_code ec;
    if (!fs::exists(target_path, ec) || !fs::is_directory(target_path, ec)) {
        return dirs;
    }

    for (const auto& entry : fs::directory_iterator(target_path, ec)) {
        if (ec) break;
        if (entry.is_directory(ec)) {
            dirs.push_back(entry.path().string());
        }
    }
#else
    DIR* dir = opendir(target_path.c_str());
    if (!dir) {
        return dirs;
    }

    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }

        std::string full_child = target_path;
        if (full_child.back() != '/' && full_child.back() != '\\') {
            full_child += '/';
        }
        full_child += name;

        struct stat st{};
        if (stat(full_child.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            dirs.push_back(full_child);
        }
    }
    closedir(dir);
#endif

    return dirs;
}

std::vector<std::string> ProcParser::tokenize(const std::string& line, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;

    for (char ch : line) {
        if (delimiter == ' ') {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            } else {
                current += ch;
            }
        } else {
            if (ch == delimiter) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            } else {
                current += ch;
            }
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string ProcParser::trim(const std::string& str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

bool ProcParser::starts_with(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size()) {
        return false;
    }
    return str.compare(0, prefix.size(), prefix) == 0;
}

optional<uint64_t> ProcParser::parse_kb_value(const std::string& line) {
    auto colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        return nullopt;
    }

    std::string rest = trim(line.substr(colon_pos + 1));
    std::istringstream iss(rest);
    uint64_t kb_val = 0;
    if (!(iss >> kb_val)) {
        return nullopt;
    }

    // Convert kB to bytes (1 kB in /proc/meminfo = 1024 bytes)
    return kb_val * 1024ULL;
}

} // namespace sysmonitor
