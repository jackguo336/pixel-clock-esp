#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace companion_protocol {

inline constexpr uint8_t kProtocolVersion = 1;
inline constexpr std::size_t kHeaderBytes = 7;
inline constexpr std::size_t kMaxFrameBytes = 128;
inline constexpr std::size_t kMaxSsidBytes = 32;
inline constexpr std::size_t kMaxPassphraseBytes = 64;
inline constexpr std::size_t kMaxFirmwareVersionBytes = 32;

enum class MessageType : uint8_t {
    GetDeviceInfo = 0x01,
    SetWifiCredentials = 0x02,
    Ack = 0x80,
    DeviceInfo = 0x81,
    Error = 0xFF,
};

// Shared by decoder results, Ack.status, and Error.code.
enum class Status : uint8_t {
    Ok = 0x00,
    FrameTooShort = 0x01,
    FrameTooLong = 0x02,
    UnsupportedVersion = 0x03,
    UnsupportedFlags = 0x04,
    BodyLengthMismatch = 0x05,
    UnknownMessageType = 0x06,
    MalformedBody = 0x07,
    InvalidField = 0x08,
    StorageFailed = 0x20,
};

struct FrameHeader {
    uint8_t version{0};
    MessageType message_type{};
    uint8_t flags{0};
    uint16_t request_id{0};
    uint16_t body_length{0};
};

struct Frame {
    FrameHeader header{};
    // Views the caller's buffer. Valid only while that buffer lives.
    std::span<const uint8_t> body{};
};

// Reads the 7-byte header only. A later reassembler can call this on a partial frame.
[[nodiscard]] Status decode_header(std::span<const uint8_t> bytes, FrameHeader& header);

// Validates the header, total length, and known message type.
// On every status except FrameTooShort, frame.header.request_id and message_type
// are filled so the caller can build an Error reply.
[[nodiscard]] Status decode_frame(std::span<const uint8_t> bytes, Frame& frame);

}  // namespace companion_protocol
