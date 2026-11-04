#pragma once

#include <cstdint>
#include <span>

namespace ble_link {

/**
 * Sends one encoded companion message.
 * Use when a reply leaves the session without the session knowing which link carries it.
 */
class MessageTransport {
public:
    virtual ~MessageTransport() = default;

    MessageTransport(const MessageTransport&) = delete;
    MessageTransport& operator=(const MessageTransport&) = delete;

    /// Bytes are valid only for this call. Copy or transmit them before returning.
    [[nodiscard]] virtual bool send(std::span<const uint8_t> message) = 0;

protected:
    MessageTransport() = default;
};

}  // namespace ble_link
