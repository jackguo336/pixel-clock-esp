#include "companion_gatt_service_definitions.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include "ble_link/companion_gatt_ids.hpp"
#include "host/ble_hs.h"

namespace ble_link {
namespace {

constexpr ble_uuid128_t make_ble_uuid128(const std::array<uint8_t, 16>& little_endian_bytes)
{
    ble_uuid128_t uuid{};
    uuid.u.type = BLE_UUID_TYPE_128;
    for (std::size_t i = 0; i < little_endian_bytes.size(); ++i) {
        uuid.value[i] = little_endian_bytes[i];
    }
    return uuid;
}

constexpr ble_uuid128_t kCompanionServiceBleUuid = make_ble_uuid128(kCompanionServiceUuid);
constexpr ble_uuid128_t kInboundCharacteristicBleUuid = make_ble_uuid128(kInboundCharacteristicUuid);
constexpr ble_uuid128_t kOutboundCharacteristicBleUuid = make_ble_uuid128(kOutboundCharacteristicUuid);

const ble_gatt_chr_def kCompanionCharacteristics[] = {
    {
        .uuid = &kInboundCharacteristicBleUuid.u,
        .access_cb = refuse_characteristic_access,
        .arg = nullptr,
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
        .min_key_size = 0,
        .val_handle = nullptr,
        .cpfd = nullptr,
    },
    {
        .uuid = &kOutboundCharacteristicBleUuid.u,
        .access_cb = refuse_characteristic_access,
        .arg = nullptr,
        .descriptors = nullptr,
        .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_NOTIFY_INDICATE_ENC,
        .min_key_size = 0,
        .val_handle = nullptr,
        .cpfd = nullptr,
    },
    {},
};

}  // namespace

const ble_gatt_svc_def kCompanionServices[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &kCompanionServiceBleUuid.u,
        .includes = nullptr,
        .characteristics = kCompanionCharacteristics,
    },
    {},
};

}  // namespace ble_link
