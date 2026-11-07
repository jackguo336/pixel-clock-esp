#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>

#include "companion_protocol/message_header.hpp"
#include "platform/message.hpp"

namespace ble_link {

struct InboundMessagePoolEntryRef {
    uint8_t entry_index{0};
    uint8_t message_length{0};
};

static_assert(std::is_trivially_copyable_v<InboundMessagePoolEntryRef>);
static_assert(sizeof(InboundMessagePoolEntryRef) < platform::kPayloadCapacity);
static_assert(companion_protocol::kMaxMessageBytes <= std::numeric_limits<uint8_t>::max());

/**
 * Fixed set of inbound companion messages.
 * Use when a message does not fit in a platform event payload.
 */
class InboundMessagePool {
public:
    static constexpr std::size_t kCapacity = 4;
    static constexpr std::size_t kMessageBytes = companion_protocol::kMaxMessageBytes;

    [[nodiscard]] std::optional<uint8_t> try_insert(std::span<const uint8_t> message);

    /// Copies a held message into out, then frees the slot.
    /// Returns the copied length, or 0 when nothing was copied.
    [[nodiscard]] std::size_t read_then_free_slot(uint8_t slot_index, std::span<uint8_t> out);

    void free_slot(uint8_t slot_index);

    [[nodiscard]] uint32_t drop_count() const;

private:
    enum class Occupancy : uint8_t {
        Free = 0,
        Held = 1,
    };

    struct Slot {
        std::atomic<Occupancy> occupancy{Occupancy::Free};
        uint8_t message_length{0};
        std::array<uint8_t, kMessageBytes> bytes{};
    };

    std::array<Slot, kCapacity> slots_{};
    std::atomic<uint32_t> drop_count_{0};
};

static_assert(InboundMessagePool::kCapacity <= std::numeric_limits<uint8_t>::max());

[[nodiscard]] inline platform::Event companion_connected_event()
{
    platform::Event event{};
    event.type = platform::EventType::CompanionConnected;
    event.source = nullptr;
    event.generation = 0;
    event.priority = platform::Priority::Normal;
    event.coalesce_key = 0;
    event.payload_size = 0;
    return event;
}

[[nodiscard]] inline platform::Event companion_disconnected_event()
{
    platform::Event event{};
    event.type = platform::EventType::CompanionDisconnected;
    event.source = nullptr;
    event.generation = 0;
    event.priority = platform::Priority::Normal;
    event.coalesce_key = 0;
    event.payload_size = 0;
    return event;
}

[[nodiscard]] inline platform::Event companion_message_received_event(InboundMessagePoolEntryRef handoff)
{
    platform::Event event{};
    event.type = platform::EventType::CompanionMessageReceived;
    event.source = nullptr;
    event.generation = 0;
    event.priority = platform::Priority::Normal;
    event.coalesce_key = 0;
    platform::set_runtime_event_payload(event, handoff);
    return event;
}

}  // namespace ble_link
