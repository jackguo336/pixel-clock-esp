#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ble_link {

inline constexpr std::size_t kLegacyAdvertisingPacketBytes = 31;

struct LegacyAdvertisingPackets {
    std::array<uint8_t, kLegacyAdvertisingPacketBytes> advertising{};
    std::size_t advertising_length{0};
    std::array<uint8_t, kLegacyAdvertisingPacketBytes> scan_response{};
    std::size_t scan_response_length{0};
};

namespace legacy_advertising_detail {

// Each advertising field is a length byte, a type byte, then the data. The
// length byte counts the type byte and the data.
inline constexpr std::size_t kFieldHeaderBytes = 2;
inline constexpr uint8_t kFieldTypeFlags = 0x01;
inline constexpr uint8_t kFieldTypeComplete128BitServiceUuids = 0x07;
inline constexpr uint8_t kFieldTypeCompleteLocalName = 0x09;
inline constexpr uint8_t kFlagGeneralDiscoverable = 0x02;
inline constexpr uint8_t kFlagClassicBluetoothNotSupported = 0x04;
inline constexpr std::size_t kServiceUuidBytes = 16;

inline std::size_t append_field(std::span<uint8_t> packet,
                                std::size_t offset,
                                uint8_t field_type,
                                std::span<const uint8_t> field_data)
{
    packet[offset] = static_cast<uint8_t>(field_data.size() + 1);
    packet[offset + 1] = field_type;
    std::copy(field_data.begin(), field_data.end(), packet.begin() + offset + kFieldHeaderBytes);
    return offset + kFieldHeaderBytes + field_data.size();
}

}  // namespace legacy_advertising_detail

/// The advertising packet carries the flags and the service UUID. The scan
/// response carries the name.
/// Returns false when the name is empty or either packet would pass 31 bytes.
/// On false, out is left unchanged.
[[nodiscard]] inline bool build_legacy_advertising_packets(std::span<const uint8_t, 16> service_uuid,
                                                           std::string_view device_name,
                                                           LegacyAdvertisingPackets& out)
{
    namespace detail = legacy_advertising_detail;

    constexpr std::size_t kFlagsBytes = 1;
    static_assert(detail::kFieldHeaderBytes + kFlagsBytes + detail::kFieldHeaderBytes + detail::kServiceUuidBytes <=
                  kLegacyAdvertisingPacketBytes);

    if (device_name.empty() ||
        detail::kFieldHeaderBytes + device_name.size() > kLegacyAdvertisingPacketBytes) {
        return false;
    }

    LegacyAdvertisingPackets packets{};

    const uint8_t flags[kFlagsBytes] = {detail::kFlagGeneralDiscoverable | detail::kFlagClassicBluetoothNotSupported};
    packets.advertising_length = detail::append_field(packets.advertising, 0, detail::kFieldTypeFlags, flags);
    packets.advertising_length = detail::append_field(packets.advertising,
                                                      packets.advertising_length,
                                                      detail::kFieldTypeComplete128BitServiceUuids,
                                                      service_uuid);

    const std::span<const uint8_t> name_bytes{reinterpret_cast<const uint8_t*>(device_name.data()),
                                              device_name.size()};
    packets.scan_response_length =
        detail::append_field(packets.scan_response, 0, detail::kFieldTypeCompleteLocalName, name_bytes);

    out = packets;
    return true;
}

}  // namespace ble_link
