#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>

#include "ble_link/inbound_message_pool.hpp"
#include "companion_protocol/message_header.hpp"
#include "gtest/gtest.h"

namespace {

using ble_link::InboundMessagePool;
using ble_link::InboundMessagePoolEntryRef;

std::size_t read_entry(InboundMessagePool& pool, uint8_t entry_index, std::span<uint8_t> out)
{
    return pool.read_then_remove(entry_index, out);
}

}  // namespace

TEST(InboundMessagePool, ReusesLowestFreeEntry)
{
    InboundMessagePool pool;
    const uint8_t message[] = {0x10, 0x20, 0x30};

    const std::optional<uint8_t> first = pool.try_insert(message);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(*first, 0);

    std::array<uint8_t, InboundMessagePool::kMessageBytes> copied{};
    const std::size_t copied_length = read_entry(pool, *first, copied);
    ASSERT_EQ(copied_length, sizeof(message));
    EXPECT_TRUE(std::equal(std::begin(message), std::end(message), copied.begin()));

    EXPECT_EQ(read_entry(pool, *first, copied), 0u);

    const std::optional<uint8_t> reused = pool.try_insert(message);
    ASSERT_TRUE(reused.has_value());
    EXPECT_EQ(*reused, 0);
}

TEST(InboundMessagePool, FullPoolDropsAndKeepsFirstMessage)
{
    InboundMessagePool pool;
    const uint8_t first_message[] = {0x01, 0x02};
    const uint8_t later_message[] = {0x03};

    const std::optional<uint8_t> first = pool.try_insert(first_message);
    const std::optional<uint8_t> second = pool.try_insert(later_message);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, 0);
    EXPECT_EQ(*second, 1);

    for (std::size_t index = 2; index < InboundMessagePool::kCapacity; ++index) {
        const std::optional<uint8_t> inserted = pool.try_insert(later_message);
        ASSERT_TRUE(inserted.has_value());
        EXPECT_EQ(*inserted, index);
    }

    EXPECT_EQ(pool.drop_count(), 0u);
    EXPECT_FALSE(pool.try_insert(first_message).has_value());
    EXPECT_EQ(pool.drop_count(), 1u);

    std::array<uint8_t, InboundMessagePool::kMessageBytes> copied{};
    const std::size_t copied_length = read_entry(pool, 0, copied);
    ASSERT_EQ(copied_length, sizeof(first_message));
    EXPECT_TRUE(std::equal(std::begin(first_message), std::end(first_message), copied.begin()));
}

TEST(InboundMessagePool, RejectsEmptyAndOverlongMessages)
{
    InboundMessagePool pool;
    std::array<uint8_t, companion_protocol::kMaxMessageBytes> max_message{};
    max_message.fill(0xAB);
    std::array<uint8_t, companion_protocol::kMaxMessageBytes + 1> too_long{};
    too_long.fill(0xCD);
    const uint8_t short_message[] = {0x11};

    const std::optional<uint8_t> stored = pool.try_insert(max_message);
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(*stored, 0);

    std::array<uint8_t, InboundMessagePool::kMessageBytes> copied{};
    const std::size_t copied_length = read_entry(pool, *stored, copied);
    ASSERT_EQ(copied_length, max_message.size());
    EXPECT_TRUE(std::equal(max_message.begin(), max_message.end(), copied.begin()));

    EXPECT_FALSE(pool.try_insert(too_long).has_value());
    EXPECT_EQ(pool.drop_count(), 1u);

    const std::optional<uint8_t> after_reject = pool.try_insert(short_message);
    ASSERT_TRUE(after_reject.has_value());
    EXPECT_EQ(*after_reject, 0);

    EXPECT_FALSE(pool.try_insert(std::span<const uint8_t>{}).has_value());
    EXPECT_EQ(pool.drop_count(), 2u);

    const std::size_t short_length = read_entry(pool, *after_reject, copied);
    ASSERT_EQ(short_length, sizeof(short_message));
    EXPECT_EQ(copied[0], short_message[0]);
}

TEST(InboundMessagePool, RemoveLeavesDropCountAndShortReadDropsBytes)
{
    InboundMessagePool pool;
    pool.remove(0);
    pool.remove(static_cast<uint8_t>(InboundMessagePool::kCapacity));
    EXPECT_EQ(pool.drop_count(), 0u);

    const uint8_t message[] = {0x01, 0x02, 0x03, 0x04};
    const std::optional<uint8_t> stored = pool.try_insert(message);
    ASSERT_TRUE(stored.has_value());

    std::array<uint8_t, 2> short_out{};
    EXPECT_EQ(pool.read_then_remove(*stored, short_out), 0u);

    std::array<uint8_t, InboundMessagePool::kMessageBytes> copied{};
    EXPECT_EQ(pool.read_then_remove(*stored, copied), 0u);
    EXPECT_EQ(pool.drop_count(), 0u);
}

TEST(InboundMessagePool, CompanionEventsCarryHandoffWithoutCoalescing)
{
    const InboundMessagePoolEntryRef handoff{.entry_index = 2, .message_length = 15};
    const platform::Event message = ble_link::companion_message_received_event(handoff);
    EXPECT_EQ(message.type, platform::EventType::CompanionMessageReceived);
    EXPECT_EQ(message.coalesce_key, 0);
    EXPECT_EQ(message.priority, platform::Priority::Normal);
    EXPECT_EQ(message.generation, 0u);
    EXPECT_EQ(message.source, nullptr);
    ASSERT_EQ(message.payload_size, sizeof(handoff));

    InboundMessagePoolEntryRef copied{};
    std::memcpy(&copied, message.payload, sizeof(copied));
    EXPECT_EQ(copied.entry_index, handoff.entry_index);
    EXPECT_EQ(copied.message_length, handoff.message_length);

    const platform::Event connected = ble_link::companion_connected_event();
    EXPECT_EQ(connected.type, platform::EventType::CompanionConnected);
    EXPECT_EQ(connected.payload_size, 0);
    EXPECT_EQ(connected.coalesce_key, 0);
    EXPECT_EQ(connected.priority, platform::Priority::Normal);

    const platform::Event disconnected = ble_link::companion_disconnected_event();
    EXPECT_EQ(disconnected.type, platform::EventType::CompanionDisconnected);
    EXPECT_EQ(disconnected.payload_size, 0);
    EXPECT_EQ(disconnected.coalesce_key, 0);
    EXPECT_EQ(disconnected.priority, platform::Priority::Normal);
}
