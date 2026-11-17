#include "firmware_version_reader.hpp"

#include <cstddef>

#include "companion_protocol/device_info.hpp"
#include "esp_app_desc.h"

namespace ble_link {

std::span<const uint8_t> firmware_version_bytes()
{
    const esp_app_desc_t* description = esp_app_get_description();
    const char* version_text = description->version;
    std::size_t version_length = 0;
    while (version_length < companion_protocol::kMaxFirmwareVersionBytes && version_text[version_length] != '\0') {
        ++version_length;
    }
    return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(version_text), version_length);
}

}  // namespace ble_link
