#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
BUILD_TYPE="Release"

echo "====================================================="
echo " Building Linux System Monitor (C++17)               "
echo "====================================================="

ACTION="${1:-all}"

case "$ACTION" in
    clean)
        echo "Cleaning build directory: ${BUILD_DIR}..."
        rm -rf "${BUILD_DIR}"
        echo "Clean completed."
        exit 0
        ;;
    test)
        echo "Configuring and building with tests enabled..."
        cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DBUILD_TESTS=ON
        cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j"$(nproc 2>/dev/null || echo 4)"
        echo "Running CTest..."
        ctest --test-dir "${BUILD_DIR}" --output-on-failure --verbose
        ;;
    all|build)
        echo "Configuring CMake (${BUILD_TYPE})..."
        cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DBUILD_TESTS=ON
        echo "Compiling..."
        cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" -j"$(nproc 2>/dev/null || echo 4)"
        echo "Build successful! Binary located at: ${BUILD_DIR}/bin/sysmonitor"
        ;;
    *)
        echo "Usage: $0 [build|test|clean]"
        exit 1
        ;;
esac
