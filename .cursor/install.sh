#!/usr/bin/env bash
# Idempotent Cloud Agent bootstrap for the pixel-clock-esp firmware.
# Installs ESP-IDF v6.0.2 (RISC-V targets), the QEMU RISC-V machine used by the
# host unit tests, and the pytest-embedded test runner. Safe to re-run.
set -euo pipefail

IDF_VERSION="v6.0.2"
IDF_DIR="$HOME/esp/esp-idf"

echo "==> Installing ESP-IDF system prerequisites"
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  git wget flex bison gperf ca-certificates \
  python3 python3-pip python3-venv \
  cmake ninja-build ccache \
  libffi-dev libssl-dev dfu-util \
  libusb-1.0-0 libbsd-dev \
  libslirp0 libpixman-1-0 libgcrypt20

echo "==> Cloning ESP-IDF ${IDF_VERSION}"
mkdir -p "$HOME/esp"
if [ ! -d "$IDF_DIR/.git" ]; then
  git clone --depth 1 -b "$IDF_VERSION" --recursive \
    https://github.com/espressif/esp-idf.git "$IDF_DIR"
fi

echo "==> Installing ESP-IDF toolchains for esp32c6 and esp32c3"
"$IDF_DIR/install.sh" esp32c6,esp32c3

# Bring idf.py, the cross toolchains, and IDF_PATH into this shell.
# shellcheck disable=SC1091
. "$IDF_DIR/export.sh"

echo "==> Installing QEMU (RISC-V) for host unit tests"
# QEMU cannot emulate esp32c6, so the test suite runs on esp32c3 instead.
python "$IDF_PATH/tools/idf_tools.py" install qemu-riscv32
# Re-source so the freshly installed QEMU binary is on PATH.
# shellcheck disable=SC1091
. "$IDF_DIR/export.sh"

echo "==> Installing pytest-embedded QEMU test runner"
python -m pip install "pytest-embedded-qemu[idf]~=2.9"

echo "==> Making idf.py available in interactive shells"
if ! grep -qs "esp-idf/export.sh" "$HOME/.bashrc"; then
  {
    echo ''
    echo '# ESP-IDF environment (added by pixel-clock-esp Cloud Agent setup)'
    echo '. "$HOME/esp/esp-idf/export.sh" > /dev/null 2>&1 || true'
  } >> "$HOME/.bashrc"
fi

echo "==> ESP-IDF environment ready"
idf.py --version
