#include "companion_protocol/error.hpp"

#include "byte_reader.hpp"
#include "byte_writer.hpp"

namespace companion_protocol {

std::optional<std::size_t> Error::encoded_body_size() const
{
    return kErrorBodyBytes;
}

bool Error::encode(std::span<uint8_t> out) const
{
    const std::optional<std::size_t> body_size = encoded_body_size();
    if (!body_size.has_value() || out.size() < *body_size) {
        return false;
    }

    ByteWriter writer(out);
    return writer.write_u8(static_cast<uint8_t>(code)) && writer.written() == *body_size;
}

Status Error::decode(std::span<const uint8_t> bytes)
{
    ByteReader reader(bytes);
    uint8_t code_byte = 0;
    if (!reader.read_u8(code_byte) || !reader.empty()) {
        return Status::MalformedBody;
    }
    code = static_cast<Status>(code_byte);
    return Status::Ok;
}

}  // namespace companion_protocol
