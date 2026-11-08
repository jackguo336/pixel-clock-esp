#include "ble_link/inbound_message_pool.hpp"

#include <algorithm>
#include <cstring>

namespace ble_link {

std::optional<uint8_t> InboundMessagePool::try_insert(std::span<const uint8_t> message)
{
    if (message.empty() || message.size() > kMessageBytes) {
        drop_count_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    for (std::size_t index = 0; index < kCapacity; ++index) {
        Occupancy expected = Occupancy::Free;
        const bool claimed = slots_[index].occupancy.compare_exchange_strong(
            expected, Occupancy::Held, std::memory_order_acq_rel, std::memory_order_acquire);
        if (!claimed) {
            continue;
        }

        Slot& slot = slots_[index];
        std::copy_n(message.begin(), message.size(), slot.bytes.begin());
        slot.message_length = static_cast<uint8_t>(message.size());
        if (message.size() < kMessageBytes) {
            std::memset(slot.bytes.data() + message.size(), 0, kMessageBytes - message.size());
        }
        return static_cast<uint8_t>(index);
    }

    drop_count_.fetch_add(1, std::memory_order_relaxed);
    return std::nullopt;
}

std::size_t InboundMessagePool::read_then_free_slot(uint8_t slot_index, std::span<uint8_t> out)
{
    if (slot_index >= kCapacity || slots_[slot_index].occupancy.load(std::memory_order_acquire) != Occupancy::Held) {
        return 0;
    }

    Slot& slot = slots_[slot_index];
    if (out.size() < slot.message_length) {
        free_slot(slot_index);
        return 0;
    }

    const uint8_t copied_length = slot.message_length;
    std::copy_n(slot.bytes.begin(), copied_length, out.begin());
    free_slot(slot_index);
    return copied_length;
}

void InboundMessagePool::free_slot(uint8_t slot_index)
{
    if (slot_index >= kCapacity) {
        return;
    }

    Slot& slot = slots_[slot_index];
    if (slot.occupancy.load(std::memory_order_acquire) != Occupancy::Held) {
        return;
    }

    slot.bytes.fill(0);
    slot.message_length = 0;
    slot.occupancy.store(Occupancy::Free, std::memory_order_release);
}

uint32_t InboundMessagePool::drop_count() const
{
    return drop_count_.load(std::memory_order_relaxed);
}

}  // namespace ble_link
