#include "TestHelper.hpp"
#include "sysmonitor/ProcessMonitor.hpp"
#include "sysmonitor/ProcParser.hpp"

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "tests/data/proc"
#endif

using namespace sysmonitor;

void test_parse_stat_line_standard() {
    std::cout << "[TEST] Running test_parse_stat_line_standard...\n";

    std::string line = "1337 (systemd-journal) S 1 1337 1337 0 -1 4194560 3824 1054 0 0 120 45 0 0 20 0 1 0 1234 45612800 1250 18446744073709551615 1 1 0 0 0 0 0 0 0 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0";

    int pid = 0;
    std::string comm;
    char state = '?';
    int ppid = 0;
    uint64_t utime = 0;
    uint64_t stime = 0;
    uint64_t starttime = 0;

    bool ok = ProcessMonitor::parse_stat_line(line, pid, comm, state, ppid, utime, stime, starttime);
    ASSERT_TRUE(ok);
    ASSERT_EQ(pid, 1337);
    ASSERT_STR_EQ(comm, "systemd-journal");
    ASSERT_EQ(state, 'S');
    ASSERT_EQ(ppid, 1);
    ASSERT_EQ(utime, 120ULL);
    ASSERT_EQ(stime, 45ULL);
    ASSERT_EQ(starttime, 1234ULL);
}

void test_parse_stat_line_with_spaces() {
    std::cout << "[TEST] Running test_parse_stat_line_with_spaces...\n";

    // Testing comm containing spaces: "Web Content"
    std::string line = "4242 (Web Content) R 100 4242 4242 0 -1 4194560 100 200 0 0 350 150 0 0 20 0 4 0 5000 100000 2000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";

    int pid = 0;
    std::string comm;
    char state = '?';
    int ppid = 0;
    uint64_t utime = 0;
    uint64_t stime = 0;
    uint64_t starttime = 0;

    bool ok = ProcessMonitor::parse_stat_line(line, pid, comm, state, ppid, utime, stime, starttime);
    ASSERT_TRUE(ok);
    ASSERT_EQ(pid, 4242);
    ASSERT_STR_EQ(comm, "Web Content");
    ASSERT_EQ(state, 'R');
    ASSERT_EQ(ppid, 100);
}

void test_parse_cmdline() {
    std::cout << "[TEST] Running test_parse_cmdline...\n";

    // Null-delimited arguments
    static const char sample_cmd[] = "/usr/bin/python3\0-m\0http.server\08080\0";
    std::string raw(sample_cmd, sizeof(sample_cmd) - 1);
    std::string parsed = ProcessMonitor::parse_cmdline(raw);
    ASSERT_STR_EQ(parsed, "/usr/bin/python3 -m http.server 8080");

    ASSERT_STR_EQ(ProcessMonitor::parse_cmdline(""), "");
}

void test_process_monitor_with_fixture() {
    std::cout << "[TEST] Running test_process_monitor_with_fixture...\n";

    auto parser = std::make_shared<ProcParser>(TEST_DATA_DIR);
    ProcessMonitor monitor(parser);

    uint64_t total_ram = 16ULL * 1024ULL * 1024ULL * 1024ULL; // 16 GB
    monitor.update(total_ram, 1000);

    const auto& procs = monitor.get_processes();
    ASSERT_EQ(procs.size(), 2ULL);

    // Verify top by memory (PID 2048 mysqld has 524288 kB RSS vs PID 1337 which has 5000 kB)
    auto top_mem = monitor.get_top_processes(2, ProcessSortBy::Memory);
    ASSERT_EQ(top_mem.size(), 2ULL);
    ASSERT_EQ(top_mem[0].pid, 2048);
    ASSERT_STR_EQ(top_mem[0].name, "mysqld");
    ASSERT_EQ(top_mem[0].memory_rss_bytes, 524288ULL * 1024ULL);

    ASSERT_EQ(top_mem[1].pid, 1337);
    ASSERT_STR_EQ(top_mem[1].name, "systemd-journal");
    ASSERT_EQ(top_mem[1].memory_rss_bytes, 5000ULL * 1024ULL);
}

int main() {
    test_parse_stat_line_standard();
    test_parse_stat_line_with_spaces();
    test_parse_cmdline();
    test_process_monitor_with_fixture();
    return test::summarize();
}
