#pragma once

#include <cstddef>
#include <cstdint>

namespace companion_protocol {

inline constexpr uint8_t kProtocolVersion = 1;
inline constexpr uint8_t kFlagsNone = 0;
inline constexpr std::size_t kHeaderBytes = 7;
inline constexpr std::size_t kMaxMessageBytes = 128;

inline constexpr std::size_t kVersionOffset = 0;
inline constexpr std::size_t kMessageTypeOffset = 1;
inline constexpr std::size_t kFlagsOffset = 2;
inline constexpr std::size_t kRequestIdOffset = 3;
inline constexpr std::size_t kBodyLengthOffset = 5;

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
    MessageTooShort = 0x01,
    MessageTooLong = 0x02,
    UnsupportedVersion = 0x03,
    UnsupportedFlags = 0x04,
    BodyLengthMismatch = 0x05,
    UnknownMessageType = 0x06,
    MalformedBody = 0x07,
    InvalidField = 0x08,
    StorageFailed = 0x20,
};

// Keep PROTOCOL.md in sync with this message header.
struct MessageHeader {
    uint8_t version{0};
    MessageType message_type{};
    uint8_t flags{0};
    uint16_t request_id{0};
    uint16_t body_length{0};
};

}  // namespace companion_protocol
