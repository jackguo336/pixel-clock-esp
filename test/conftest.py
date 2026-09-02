import os
import shutil
from pathlib import Path


def _idf_qemu_bin_dirs() -> list[Path]:
    tools_root = Path(os.environ.get('IDF_TOOLS_PATH', Path.home() / '.espressif' / 'tools'))
    return sorted(tools_root.glob('tools/qemu-riscv32/*/qemu/bin'), reverse=True)


def pytest_configure(config) -> None:  # noqa: ARG001
    if shutil.which('qemu-system-riscv32') is not None:
        return
    for bin_dir in _idf_qemu_bin_dirs():
        if (bin_dir / 'qemu-system-riscv32').exists():
            os.environ['PATH'] = f'{bin_dir}{os.pathsep}{os.environ["PATH"]}'
            return
