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

# Login shells source ~/.profile, which sources ~/.bashrc after /etc/profile.d.
# An older bootstrap appended the legacy export.sh hook there, and that hook
# overrides the EIM activation. Drop it when it is still present.
if [[ -f "${HOME}/.bashrc" ]] && grep -q 'esp/esp-idf/export.sh' "${HOME}/.bashrc"; then
  sed -i '/pixel-clock-esp Cloud Agent setup/d;/esp\/esp-idf\/export.sh/d' "${HOME}/.bashrc"
fi

echo "==> Making ESP-IDF available to login shells"
# Cloud Agent install/start run as non-interactive login shells. Those shells
# read /etc/profile.d and do not read ~/.bashrc. Source the activation script
# from a login shell: EIM's script refuses to run when sourced from another
# script (it checks $0), and it reads ZSH_VERSION under nounset.
sudo tee /etc/profile.d/esp-idf.sh >/dev/null <<EOF
# Activate ESP-IDF ${IDF_VERSION} (pixel-clock-esp Cloud Agent setup).
# Replace an inherited legacy IDF_PATH; login shells source this file before
# ~/.bashrc, but a parent process may already have exported the old clone.
eim_idf="\${HOME}/.espressif/${IDF_VERSION}/esp-idf"
activate="\${HOME}/.espressif/tools/activate_idf_${IDF_VERSION}.sh"
if [ "\${IDF_PATH:-}" != "\${eim_idf}" ] && [ -f "\${activate}" ]; then
  unset IDF_PATH
  . "\${activate}" >/dev/null 2>&1 || true
fi
if ! command -v qemu-system-riscv32 >/dev/null 2>&1; then
  for qemu_bin in "\${HOME}"/.espressif/tools/qemu-riscv32/*/qemu/bin; do
    if [ -x "\${qemu_bin}/qemu-system-riscv32" ]; then
      PATH="\${qemu_bin}:\${PATH}"
      export PATH
      break
    fi
  done
fi
EOF
sudo chmod 644 /etc/profile.d/esp-idf.sh

echo "==> Installing QEMU (RISC-V) and pytest-embedded"
# QEMU cannot emulate esp32c6, so the embedded suite runs on esp32c3.
bash -lc "python \"\$IDF_PATH/tools/idf_tools.py\" install qemu-riscv32"
bash -lc "python -m pip install --disable-pip-version-check 'pytest-embedded-qemu[idf]~=2.9'"

echo "==> ESP-IDF environment ready"
bash -lc 'idf.py --version && command -v eim && command -v qemu-system-riscv32'
