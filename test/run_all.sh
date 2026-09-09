#!/usr/bin/env bash

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
native_build_dir="${project_dir}/test/native_host/build"
embedded_dir="${project_dir}/test/embedded"
embedded_command="idf.py -B build_esp32c3_qemu -D SDKCONFIG=build_esp32c3_qemu/sdkconfig -D IDF_TARGET=esp32c3 build && python -m pytest pytest_embedded_tests.py"

cmake -S "${project_dir}/test/native_host" -B "${native_build_dir}"
cmake --build "${native_build_dir}"
ctest --test-dir "${native_build_dir}" --output-on-failure

# Run directly in an active ESP-IDF environment; otherwise let EIM activate it.
if [[ -n "${IDF_PATH:-}" ]] && command -v idf.py >/dev/null 2>&1; then
    (cd "${embedded_dir}" && bash -c "${embedded_command}")
elif command -v eim >/dev/null 2>&1; then
    (cd "${embedded_dir}" && eim run "${embedded_command}")
else
    echo "ESP-IDF is not active and eim is not installed." >&2
    exit 1
fi
