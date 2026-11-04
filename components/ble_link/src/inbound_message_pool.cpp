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
        const bool claimed = entries_[index].occupancy.compare_exchange_strong(
            expected, Occupancy::Held, std::memory_order_acq_rel, std::memory_order_acquire);
        if (!claimed) {
            continue;
        }

        Entry& entry = entries_[index];
        std::copy_n(message.begin(), message.size(), entry.bytes.begin());
        entry.message_length = static_cast<uint8_t>(message.size());
        if (message.size() < kMessageBytes) {
            std::memset(entry.bytes.data() + message.size(), 0, kMessageBytes - message.size());
        }
        return static_cast<uint8_t>(index);
    }

    drop_count_.fetch_add(1, std::memory_order_relaxed);
    return std::nullopt;
}

std::size_t InboundMessagePool::read_then_remove(uint8_t entry_index, std::span<uint8_t> out)
{
    if (entry_index >= kCapacity
        || entries_[entry_index].occupancy.load(std::memory_order_acquire) != Occupancy::Held) {
        return 0;
    }

    Entry& entry = entries_[entry_index];
    if (out.size() < entry.message_length) {
        remove(entry_index);
        return 0;
    }

    const uint8_t copied_length = entry.message_length;
    std::copy_n(entry.bytes.begin(), copied_length, out.begin());
    remove(entry_index);
    return copied_length;
}

void InboundMessagePool::remove(uint8_t entry_index)
{
    if (entry_index >= kCapacity) {
        return;
    }

    Entry& entry = entries_[entry_index];
    if (entry.occupancy.load(std::memory_order_acquire) != Occupancy::Held) {
        return;
    }

    entry.bytes.fill(0);
    entry.message_length = 0;
    entry.occupancy.store(Occupancy::Free, std::memory_order_release);
}

uint32_t InboundMessagePool::drop_count() const
{
    return drop_count_.load(std::memory_order_relaxed);
}

}  // namespace ble_link
