#include "TestHelper.hpp"
#include "sysmonitor/CpuMonitor.hpp"
#include "sysmonitor/ProcParser.hpp"

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "tests/data/proc"
#endif

using namespace sysmonitor;

void test_pure_calculation() {
    std::cout << "[TEST] Running test_pure_calculation...\n";

    CpuTimes prev{};
    prev.user = 1000;
    prev.idle = 1000;

    // 50% CPU scenario: active +500, idle +500 -> total +1000
    CpuTimes curr_50 = prev;
    curr_50.user += 500;
    curr_50.idle += 500;
    ASSERT_NEAR(CpuMonitor::calculate_usage(prev, curr_50), 50.0, 0.01);

    // 100% CPU scenario: active +1000, idle +0 -> total +1000
    CpuTimes curr_100 = prev;
    curr_100.user += 1000;
    ASSERT_NEAR(CpuMonitor::calculate_usage(prev, curr_100), 100.0, 0.01);

    // 0% CPU scenario: active +0, idle +1000 -> total +1000
    CpuTimes curr_0 = prev;
    curr_0.idle += 1000;
    ASSERT_NEAR(CpuMonitor::calculate_usage(prev, curr_0), 0.0, 0.01);

    // Zero delta scenario (no time passed)
    ASSERT_NEAR(CpuMonitor::calculate_usage(prev, prev), 0.0, 0.01);
}

void test_parse_cpu_line() {
    std::cout << "[TEST] Running test_parse_cpu_line...\n";

    std::string agg_line = "cpu  101234 5678 45678 890123 1234 567 890 0 0 0";
    std::string name;
    CpuTimes times{};

    ASSERT_TRUE(CpuMonitor::parse_cpu_line(agg_line, name, times));
    ASSERT_STR_EQ(name, "cpu");
    ASSERT_EQ(times.user, 101234ULL);
    ASSERT_EQ(times.nice, 5678ULL);
    ASSERT_EQ(times.system, 45678ULL);
    ASSERT_EQ(times.idle, 890123ULL);
    ASSERT_EQ(times.iowait, 1234ULL);
    ASSERT_EQ(times.irq, 567ULL);
    ASSERT_EQ(times.softirq, 890ULL);

    std::string core_line = "cpu0 25308 1419 11419 222530 308 141 222 0 0 0";
    ASSERT_TRUE(CpuMonitor::parse_cpu_line(core_line, name, times));
    ASSERT_STR_EQ(name, "cpu0");
    ASSERT_EQ(times.user, 25308ULL);

    std::string invalid_line = "intr 12345678 1 2 3";
    ASSERT_FALSE(CpuMonitor::parse_cpu_line(invalid_line, name, times));
}

void test_cpu_monitor_with_fixture() {
    std::cout << "[TEST] Running test_cpu_monitor_with_fixture...\n";

    auto parser = std::make_shared<ProcParser>(TEST_DATA_DIR);
    CpuMonitor monitor(parser);

    monitor.update();

    ASSERT_STR_EQ(monitor.get_aggregate_cpu().name, "cpu");
    ASSERT_EQ(monitor.get_core_count(), 4ULL);

    const auto& cores = monitor.get_per_core_cpu();
    ASSERT_STR_EQ(cores[0].name, "cpu0");
    ASSERT_STR_EQ(cores[1].name, "cpu1");
    ASSERT_STR_EQ(cores[2].name, "cpu2");
    ASSERT_STR_EQ(cores[3].name, "cpu3");
}

int main() {
    test_pure_calculation();
    test_parse_cpu_line();
    test_cpu_monitor_with_fixture();
    return test::summarize();
}
