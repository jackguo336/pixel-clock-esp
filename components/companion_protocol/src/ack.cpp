#include "companion_protocol/ack.hpp"

#include "byte_reader.hpp"
#include "byte_writer.hpp"

namespace companion_protocol {

std::optional<std::size_t> Ack::encoded_body_size() const
{
    return kAckBodyBytes;
}

bool Ack::encode(std::span<uint8_t> out) const
{
    const std::optional<std::size_t> body_size = encoded_body_size();
    if (!body_size.has_value() || out.size() < *body_size) {
        return false;
    }

    ByteWriter writer(out);
    return writer.write_u8(static_cast<uint8_t>(status)) && writer.written() == *body_size;
}

Status Ack::decode(std::span<const uint8_t> bytes)
{
    ByteReader reader(bytes);
    uint8_t status_byte = 0;
    if (!reader.read_u8(status_byte) || !reader.empty()) {
        return Status::MalformedBody;
    }
    status = static_cast<Status>(status_byte);
    return Status::Ok;
}

}  // namespace companion_protocol
