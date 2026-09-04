# sysmonitor_core: static library encapsulating all monitoring functionality
add_library(sysmonitor_core STATIC
    Compat.cpp
    ProcParser.cpp
    CpuMonitor.cpp
    MemoryMonitor.cpp
    DiskMonitor.cpp
    ProcessMonitor.cpp
    SystemMonitor.cpp
)

target_include_directories(sysmonitor_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(sysmonitor_core PUBLIC cxx_std_17)

# Threads support for timing and sleep operations
find_package(Threads REQUIRED)
target_link_libraries(sysmonitor_core PUBLIC Threads::Threads)

# CLI application
add_executable(sysmonitor main.cpp)
target_link_libraries(sysmonitor PRIVATE sysmonitor_core)
