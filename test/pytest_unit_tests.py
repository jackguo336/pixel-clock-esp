import pytest
from pytest_embedded import Dut
from pytest_embedded_idf.utils import idf_parametrize


# QEMU does not support ESP32C6, so use ESP32C3 as the closest architecture.
@pytest.mark.host_test
@pytest.mark.qemu
@idf_parametrize('target', ['esp32c3'], indirect=['target'])
def test_qemu(dut: Dut) -> None:
    assert dut.app.target == 'esp32c3'
    dut.run_all_single_board_cases(timeout=120)
