#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>

#include "ble_link/buffered_message_assembler.hpp"
#include "companion_protocol/message_header.hpp"
#include "gtest/gtest.h"

namespace {

using ble_link::AppendResult;
using ble_link::BufferedMessageAssembler;

constexpr uint8_t kUnwritten = 0xEE;
constexpr uint8_t kGetDeviceInfo[] = {0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00};
constexpr uint8_t kGetDeviceInfoRequestTwo[] = {0x01, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00};
constexpr uint8_t kSetWifiCredentials[] = {
    0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00, 0x04, 0x68, 0x6F, 0x6D, 0x65, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F,
    0x72, 0x64,
};
constexpr std::size_t kPieceBytes = 20;

struct Appended {
    AppendResult result{AppendResult::NeedMore};
    std::size_t message_length{0};
    std::array<uint8_t, BufferedMessageAssembler::kMessageBytes> message{};
};

Appended append_piece(BufferedMessageAssembler& assembler, std::span<const uint8_t> message_piece)
{
    Appended appended{};
    appended.message.fill(kUnwritten);
    appended.message_length = std::numeric_limits<std::size_t>::max();
    appended.result = assembler.append(message_piece, appended.message, appended.message_length);
    return appended;
}

void expect_unchanged(const Appended& appended)
{
    EXPECT_EQ(appended.message_length, std::numeric_limits<std::size_t>::max());
    EXPECT_TRUE(std::all_of(appended.message.begin(), appended.message.end(),
                            [](uint8_t byte) { return byte == kUnwritten; }));
}

void expect_message(const Appended& appended, std::span<const uint8_t> expected)
{
    ASSERT_EQ(appended.message_length, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), appended.message.begin()));
    EXPECT_TRUE(std::all_of(appended.message.begin() + static_cast<std::ptrdiff_t>(expected.size()),
                            appended.message.end(), [](uint8_t byte) { return byte == kUnwritten; }));
}

}  // namespace

TEST(BufferedMessageAssembler, GetDeviceInfoFinishesInOneAppend)
{
    BufferedMessageAssembler assembler;
    const Appended appended = append_piece(assembler, kGetDeviceInfo);

    EXPECT_EQ(appended.result, AppendResult::Finished);
    expect_message(appended, kGetDeviceInfo);
    EXPECT_EQ(assembler.drop_count(), 0u);
}

TEST(BufferedMessageAssembler, GetDeviceInfoSplitAcrossTwoAppends)
{
    BufferedMessageAssembler assembler;
    const std::span<const uint8_t> message(kGetDeviceInfo);

    const Appended first = append_piece(assembler, message.first(3));
    EXPECT_EQ(first.result, AppendResult::NeedMore);
    expect_unchanged(first);
    EXPECT_EQ(assembler.drop_count(), 0u);

    const Appended second = append_piece(assembler, message.subspan(3));
    EXPECT_EQ(second.result, AppendResult::Finished);
    expect_message(second, kGetDeviceInfo);
    EXPECT_EQ(assembler.drop_count(), 0u);
}

TEST(BufferedMessageAssembler, SetWifiCredentialsCompletesWhenAllBytesArrive)
{
    BufferedMessageAssembler assembler;
    const std::span<const uint8_t> message(kSetWifiCredentials);
    ASSERT_GT(message.size(), kPieceBytes);

    const Appended first = append_piece(assembler, message.first(kPieceBytes));
    EXPECT_EQ(first.result, AppendResult::NeedMore);
    expect_unchanged(first);

    const Appended second = append_piece(assembler, message.subspan(kPieceBytes));
    EXPECT_EQ(second.result, AppendResult::Finished);
    expect_message(second, kSetWifiCredentials);
    EXPECT_EQ(assembler.drop_count(), 0u);
}

TEST(BufferedMessageAssembler, HeaderDeclaresMessagePastMaxLength)
{
    BufferedMessageAssembler assembler;
    // body_length 122 makes 7 + 122 = 129 bytes. The stored header is thrown away.
    const uint8_t declares_129_bytes[] = {0x01, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x00};

    const Appended rejected = append_piece(assembler, declares_129_bytes);
    EXPECT_EQ(rejected.result, AppendResult::Rejected);
    expect_unchanged(rejected);
    EXPECT_EQ(assembler.drop_count(), 1u);

    const Appended following = append_piece(assembler, kGetDeviceInfo);
    EXPECT_EQ(following.result, AppendResult::Finished);
    expect_message(following, kGetDeviceInfo);
    EXPECT_EQ(assembler.drop_count(), 1u);
}

TEST(BufferedMessageAssembler, PieceCrossesDeclaredMessageEnd)
{
    BufferedMessageAssembler assembler;
    // body_length 121 needs 128 bytes. 120 are stored, then 9 more would pass 128.
    std::array<uint8_t, 120> stored{};
    stored[0] = 0x01;
    stored[1] = 0x01;
    stored[2] = 0x00;
    stored[3] = 0x01;
    stored[4] = 0x00;
    stored[5] = 0x79;
    stored[6] = 0x00;

    const Appended waiting = append_piece(assembler, stored);
    EXPECT_EQ(waiting.result, AppendResult::NeedMore);
    expect_unchanged(waiting);

    std::array<uint8_t, 9> overflow{};
    overflow.fill(0x11);
    const Appended rejected = append_piece(assembler, overflow);
    EXPECT_EQ(rejected.result, AppendResult::Rejected);
    expect_unchanged(rejected);
    EXPECT_EQ(assembler.drop_count(), 1u);

    const Appended following = append_piece(assembler, kGetDeviceInfo);
    EXPECT_EQ(following.result, AppendResult::Finished);
    expect_message(following, kGetDeviceInfo);
    EXPECT_EQ(assembler.drop_count(), 1u);
}

TEST(BufferedMessageAssembler, PiecePastDeclaredLengthIsRejected)
{
    BufferedMessageAssembler assembler;
    const uint8_t header_prefix[] = {0x01, 0x01, 0x00, 0x01, 0x00, 0x00};
    const uint8_t crosses_end[] = {0x00, 0xFF};

    const Appended waiting = append_piece(assembler, header_prefix);
    EXPECT_EQ(waiting.result, AppendResult::NeedMore);

    const Appended rejected = append_piece(assembler, crosses_end);
    EXPECT_EQ(rejected.result, AppendResult::Rejected);
    expect_unchanged(rejected);
    EXPECT_EQ(assembler.drop_count(), 1u);
}

TEST(BufferedMessageAssembler, EmptyPieceLeavesPartialInPlace)
{
    BufferedMessageAssembler assembler;
    const std::span<const uint8_t> message(kGetDeviceInfo);

    const Appended partial = append_piece(assembler, message.first(3));
    EXPECT_EQ(partial.result, AppendResult::NeedMore);
    const uint32_t drops = assembler.drop_count();

    const Appended empty = append_piece(assembler, std::span<const uint8_t>{});
    EXPECT_EQ(empty.result, AppendResult::NeedMore);
    expect_unchanged(empty);
    EXPECT_EQ(assembler.drop_count(), drops);

    const Appended finished = append_piece(assembler, message.subspan(3));
    EXPECT_EQ(finished.result, AppendResult::Finished);
    expect_message(finished, kGetDeviceInfo);
    EXPECT_EQ(assembler.drop_count(), drops);
}

TEST(BufferedMessageAssembler, ResetClearsPartialAndLeavesDropCount)
{
    BufferedMessageAssembler assembler;
    const uint8_t declares_129_bytes[] = {0x01, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x00};
    ASSERT_EQ(append_piece(assembler, declares_129_bytes).result, AppendResult::Rejected);
    ASSERT_EQ(assembler.drop_count(), 1u);

    const Appended partial = append_piece(assembler, std::span<const uint8_t>(kGetDeviceInfo).first(3));
    EXPECT_EQ(partial.result, AppendResult::NeedMore);

    assembler.reset();
    EXPECT_EQ(assembler.drop_count(), 1u);

    const Appended restarted = append_piece(assembler, kGetDeviceInfo);
    EXPECT_EQ(restarted.result, AppendResult::Finished);
    expect_message(restarted, kGetDeviceInfo);
    EXPECT_EQ(assembler.drop_count(), 1u);
}

TEST(BufferedMessageAssembler, WriteLongerThanMessageIsNotAppended)
{
    BufferedMessageAssembler assembler;
    const std::span<const uint8_t> message(kGetDeviceInfo);
    const Appended partial = append_piece(assembler, message.first(3));
    EXPECT_EQ(partial.result, AppendResult::NeedMore);
    const uint32_t drops = assembler.drop_count();

    // The write callback refuses this length before append, so the partial stays waiting.
    std::array<uint8_t, companion_protocol::kMaxMessageBytes + 1> long_write{};
    long_write.fill(0x5A);
    ASSERT_GT(long_write.size(), companion_protocol::kMaxMessageBytes);
    EXPECT_EQ(assembler.drop_count(), drops);

    const Appended finished = append_piece(assembler, message.subspan(3));
    EXPECT_EQ(finished.result, AppendResult::Finished);
    expect_message(finished, kGetDeviceInfo);
    EXPECT_EQ(assembler.drop_count(), drops);
}

TEST(BufferedMessageAssembler, NextAppendStartsANewMessage)
{
    BufferedMessageAssembler assembler;

    const Appended first = append_piece(assembler, kGetDeviceInfo);
    EXPECT_EQ(first.result, AppendResult::Finished);
    expect_message(first, kGetDeviceInfo);

    const Appended second = append_piece(assembler, kGetDeviceInfoRequestTwo);
    EXPECT_EQ(second.result, AppendResult::Finished);
    expect_message(second, kGetDeviceInfoRequestTwo);
    EXPECT_EQ(assembler.drop_count(), 0u);
}
