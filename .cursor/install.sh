#!/usr/bin/env bash
# Idempotent Cloud Agent bootstrap for the pixel-clock-esp firmware.
# Installs ESP-IDF v6.0.2 via the Espressif Installation Manager (EIM), the
# QEMU RISC-V machine used by the embedded unit tests, and pytest-embedded.
# Safe to re-run. Matches the activation script VS Code tasks source:
# ~/.espressif/tools/activate_idf_v6.0.2.sh
set -euo pipefail

IDF_VERSION="v6.0.2"
ACTIVATE="${HOME}/.espressif/tools/activate_idf_${IDF_VERSION}.sh"

echo "==> Installing host packages"
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  ca-certificates curl gnupg \
  git wget flex bison gperf \
  python3 python3-pip python3-venv \
  cmake ninja-build ccache g++ \
  libffi-dev libssl-dev dfu-util \
  libusb-1.0-0 libbsd-dev pkg-config \
  libslirp0 libpixman-1-0 libgcrypt20

if ! command -v eim >/dev/null 2>&1; then
  echo "==> Adding the Espressif EIM apt repository"
  sudo install -m 0755 -d /etc/apt/keyrings
  sudo curl -fsSL https://dl.espressif.com/dl/eim/eim.gpg -o /etc/apt/keyrings/eim.gpg
  sudo chmod 0644 /etc/apt/keyrings/eim.gpg
  sudo curl -fsSL https://dl.espressif.com/dl/eim/eim.sources -o /etc/apt/sources.list.d/espressif.sources
  sudo apt-get update
  sudo apt-get install -y eim-cli
fi

if [[ ! -f "${ACTIVATE}" ]]; then
  echo "==> Installing ESP-IDF ${IDF_VERSION} for esp32c6 and esp32c3"
  # QEMU cannot emulate esp32c6; the embedded suite builds for esp32c3.
  eim --do-not-track true install \
    -i "${IDF_VERSION}" \
    -t esp32c6,esp32c3 \
    --cleanup true
fi

if [[ ! -f "${ACTIVATE}" ]]; then
  echo "EIM did not write ${ACTIVATE}" >&2
  exit 1
fi

# shellcheck disable=SC1090
. "${ACTIVATE}"

echo "==> Installing QEMU (RISC-V) for embedded unit tests"
python "${IDF_PATH}/tools/idf_tools.py" install qemu-riscv32
# Re-source so the freshly installed QEMU binary is on PATH.
# shellcheck disable=SC1090
. "${ACTIVATE}"

echo "==> Installing pytest-embedded QEMU test runner"
python -m pip install --disable-pip-version-check "pytest-embedded-qemu[idf]~=2.9"

echo "==> Making ESP-IDF available to login shells"
# Cloud Agent install/start run as non-interactive login shells. Those shells
# read /etc/profile.d and do not read ~/.bashrc, so the activation script has
# to live here for idf.py to be on PATH.
sudo tee /etc/profile.d/esp-idf.sh >/dev/null <<EOF
# Activate ESP-IDF ${IDF_VERSION} (pixel-clock-esp Cloud Agent setup).
if [ -z "\${IDF_PATH:-}" ] && [ -f "\${HOME}/.espressif/tools/activate_idf_${IDF_VERSION}.sh" ]; then
  . "\${HOME}/.espressif/tools/activate_idf_${IDF_VERSION}.sh" >/dev/null 2>&1 || true
fi
EOF
sudo chmod 644 /etc/profile.d/esp-idf.sh

echo "==> ESP-IDF environment ready"
idf.py --version
command -v eim
command -v qemu-system-riscv32 || true
