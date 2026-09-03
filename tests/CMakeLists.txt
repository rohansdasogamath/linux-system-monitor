set(TEST_DATA_PATH "${CMAKE_CURRENT_SOURCE_DIR}/data/proc")

macro(add_sysmonitor_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE sysmonitor_core)
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(${TEST_NAME} PRIVATE TEST_DATA_DIR="${TEST_DATA_PATH}")
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

add_sysmonitor_test(test_cpu test_cpu.cpp)
add_sysmonitor_test(test_memory test_memory.cpp)
add_sysmonitor_test(test_disk test_disk.cpp)
add_sysmonitor_test(test_process test_process.cpp)
