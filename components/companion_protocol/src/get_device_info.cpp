#include "companion_protocol/get_device_info.hpp"

namespace companion_protocol {

std::optional<std::size_t> GetDeviceInfo::encoded_body_size() const
{
    return 0;
}

bool GetDeviceInfo::encode(std::span<uint8_t> out) const
{
    const std::optional<std::size_t> body_size = encoded_body_size();
    return body_size.has_value() && out.size() >= *body_size;
}

Status GetDeviceInfo::decode(std::span<const uint8_t> bytes)
{
    if (!bytes.empty()) {
        return Status::MalformedBody;
    }
    return Status::Ok;
}

}  // namespace companion_protocol
