#include "TestHelper.hpp"
#include "sysmonitor/MemoryMonitor.hpp"
#include "sysmonitor/ProcParser.hpp"

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "tests/data/proc"
#endif

using namespace sysmonitor;

void test_parse_meminfo_direct() {
    std::cout << "[TEST] Running test_parse_meminfo_direct...\n";

    std::vector<std::string> sample_lines = {
        "MemTotal:       16304576 kB",
        "MemFree:         4123568 kB",
        "MemAvailable:   10582912 kB",
        "Buffers:          456128 kB",
        "Cached:          6501232 kB",
        "SwapTotal:       2097148 kB",
        "SwapFree:        1048574 kB"
    };

    auto mem = MemoryMonitor::parse_meminfo(sample_lines);

    ASSERT_EQ(mem.total_bytes, 16304576ULL * 1024ULL);
    ASSERT_EQ(mem.free_bytes, 4123568ULL * 1024ULL);
    ASSERT_EQ(mem.available_bytes, 10582912ULL * 1024ULL);
    ASSERT_EQ(mem.buffers_bytes, 456128ULL * 1024ULL);
    ASSERT_EQ(mem.cached_bytes, 6501232ULL * 1024ULL);

    uint64_t expected_used = (16304576ULL - 10582912ULL) * 1024ULL;
    ASSERT_EQ(mem.get_used_bytes(), expected_used);

    double expected_pct = (static_cast<double>(expected_used) / static_cast<double>(mem.total_bytes)) * 100.0;
    ASSERT_NEAR(mem.get_usage_percent(), expected_pct, 0.05);

    // Swap verification
    uint64_t expected_swap_used = (2097148ULL - 1048574ULL) * 1024ULL;
    ASSERT_EQ(mem.get_swap_used_bytes(), expected_swap_used);
    ASSERT_NEAR(mem.get_swap_usage_percent(), 50.0, 0.01);
}

void test_meminfo_fallback_calculation() {
    std::cout << "[TEST] Running test_meminfo_fallback_calculation...\n";

    // Scenario: old Linux kernel without MemAvailable
    std::vector<std::string> legacy_lines = {
        "MemTotal:       10000000 kB",
        "MemFree:         2000000 kB",
        "Buffers:         1000000 kB",
        "Cached:          3000000 kB"
    };

    auto mem = MemoryMonitor::parse_meminfo(legacy_lines);

    // MemAvailable should fallback to Free + Buffers + Cached = 6000000 kB
    uint64_t expected_avail = 6000000ULL * 1024ULL;
    ASSERT_EQ(mem.available_bytes, expected_avail);

    uint64_t expected_used = 4000000ULL * 1024ULL;
    ASSERT_EQ(mem.get_used_bytes(), expected_used);
    ASSERT_NEAR(mem.get_usage_percent(), 40.0, 0.01);
}

void test_memory_monitor_with_fixture() {
    std::cout << "[TEST] Running test_memory_monitor_with_fixture...\n";

    auto parser = std::make_shared<ProcParser>(TEST_DATA_DIR);
    MemoryMonitor monitor(parser);

    monitor.update();
    const auto& mem = monitor.get_memory_info();

    ASSERT_EQ(mem.total_bytes, 16304576ULL * 1024ULL);
    ASSERT_EQ(mem.available_bytes, 10582912ULL * 1024ULL);
    ASSERT_TRUE(mem.get_used_bytes() > 0);
    ASSERT_TRUE(mem.get_usage_percent() > 30.0 && mem.get_usage_percent() < 40.0);
}

int main() {
    test_parse_meminfo_direct();
    test_meminfo_fallback_calculation();
    test_memory_monitor_with_fixture();
    return test::summarize();
}
