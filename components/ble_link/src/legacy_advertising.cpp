#include "ble_link/legacy_advertising.hpp"

#include <algorithm>

namespace ble_link {
namespace {

// Each advertising field is a length byte, a type byte, then the data. The
// length byte counts the type byte and the data.
constexpr std::size_t kFieldHeaderBytes = 2;
constexpr uint8_t kFieldTypeFlags = 0x01;
constexpr uint8_t kFieldTypeComplete128BitServiceUuids = 0x07;
constexpr uint8_t kFieldTypeCompleteLocalName = 0x09;
constexpr uint8_t kFlagGeneralDiscoverable = 0x02;
constexpr uint8_t kFlagClassicBluetoothNotSupported = 0x04;

std::size_t append_field(std::span<uint8_t> packet,
                         std::size_t offset,
                         uint8_t field_type,
                         std::span<const uint8_t> field_data)
{
    packet[offset] = static_cast<uint8_t>(field_data.size() + 1);
    packet[offset + 1] = field_type;
    std::copy(field_data.begin(), field_data.end(), packet.begin() + offset + kFieldHeaderBytes);
    return offset + kFieldHeaderBytes + field_data.size();
}

}  // namespace

bool build_legacy_advertising_packets(std::span<const uint8_t, 16> service_uuid,
                                      std::string_view device_name,
                                      LegacyAdvertisingPackets& out)
{
    constexpr std::size_t kFlagsBytes = 1;
    static_assert(kFieldHeaderBytes + kFlagsBytes + kFieldHeaderBytes + service_uuid.size() <=
                  kLegacyAdvertisingPacketBytes);

    if (device_name.empty() || kFieldHeaderBytes + device_name.size() > kLegacyAdvertisingPacketBytes) {
        return false;
    }

    LegacyAdvertisingPackets packets{};

    const uint8_t flags[kFlagsBytes] = {kFlagGeneralDiscoverable | kFlagClassicBluetoothNotSupported};
    packets.advertising_length = append_field(packets.advertising, 0, kFieldTypeFlags, flags);
    packets.advertising_length =
        append_field(packets.advertising, packets.advertising_length, kFieldTypeComplete128BitServiceUuids,
                     service_uuid);

    const std::span<const uint8_t> name_bytes{reinterpret_cast<const uint8_t*>(device_name.data()), device_name.size()};
    packets.scan_response_length = append_field(packets.scan_response, 0, kFieldTypeCompleteLocalName, name_bytes);

    out = packets;
    return true;
}

}  // namespace ble_link
