cmake_minimum_required(VERSION 3.16)
project(LinuxSystemMonitor VERSION 1.0.0 LANGUAGES CXX)

# Enforce C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers (clangd, ccls)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output binaries and libraries to unified directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(BUILD_TESTS "Build the unit test suite" ON)
option(ENABLE_WERROR "Treat warnings as errors" OFF)

# Common compiler warning flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(ENABLE_WERROR)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor -Wno-attributes)
    if(ENABLE_WERROR)
        add_compile_options(-Werror)
    endif()
endif()

# Common include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Subdirectories
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
