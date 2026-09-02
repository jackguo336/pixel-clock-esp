#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/message.hpp"

namespace platform {

class Mailbox {
public:
    static constexpr std::size_t kCapacity = 32;
    static constexpr std::size_t kCriticalReserved = 4;
    static constexpr std::size_t kNormalCapacity = kCapacity - kCriticalReserved;

    Mailbox();
    ~Mailbox();

    Mailbox(const Mailbox&) = delete;
    Mailbox& operator=(const Mailbox&) = delete;

    bool post(const Message& message, uint32_t timeout_ms);
    bool post_from_isr(const Message& message, bool* yielded);
    bool receive(Message& out, uint32_t timeout_ms);

    std::size_t size() const;
    uint32_t drop_count() const;
    uint32_t overflow_count() const;

private:
    enum class InsertResult : uint8_t {
        InsertedNormal,
        InsertedHighPriority,
        Coalesced,
        Dropped,
        Overflow,
    };

    InsertResult try_insert_locked(const Message& message);
    bool try_coalesce_locked(const Message& message);
    static bool uses_reserved_slots(const Message& message);

    Message buffer_[kCapacity]{};
    /** Next slot to receive */
    std::size_t head_{0};
    /** Next slot to insert */
    std::size_t tail_{0};
    std::size_t count_{0};
    uint32_t drop_count_{0};
    uint32_t overflow_count_{0};
    /** Counting semaphore: messages waiting to be received. */
    void* items_available_{nullptr};
    /** Counting semaphore of free slots usable by non-critical events. */
    void* normal_slots_available_{nullptr};
    /** Counting semaphore of free slots reserved for commands and critical events. */
    void* high_priority_slots_available_{nullptr};
    void* lock_{nullptr};
};

}  // namespace platform
