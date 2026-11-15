#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

#include "ble_link/outbound_message_splitter.hpp"
#include "gtest/gtest.h"

namespace {

using ble_link::kMaxNotifyPieces;
using ble_link::message_split_piece_lengths;

constexpr std::size_t kUnchangedLength = 99;
constexpr uint16_t kMinimumMtu = 23;
constexpr uint16_t kLargerMtu = 185;

struct Split {
    bool accepted{false};
    std::size_t piece_count{0};
    std::array<std::size_t, kMaxNotifyPieces> piece_lengths{};
};

Split split(std::size_t message_length, uint16_t att_mtu)
{
    Split result{};
    result.piece_lengths.fill(kUnchangedLength);
    result.piece_count = kUnchangedLength;
    result.accepted = message_split_piece_lengths(message_length, att_mtu, result.piece_lengths, result.piece_count);
    return result;
}

void expect_unchanged(const Split& result)
{
    EXPECT_FALSE(result.accepted);
    EXPECT_EQ(result.piece_count, 0u);
    EXPECT_TRUE(std::all_of(result.piece_lengths.begin(), result.piece_lengths.end(),
                            [](std::size_t length) { return length == kUnchangedLength; }));
}

void expect_pieces(const Split& result, std::initializer_list<std::size_t> expected)
{
    ASSERT_TRUE(result.accepted);
    ASSERT_EQ(result.piece_count, expected.size());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), result.piece_lengths.begin()));
    EXPECT_TRUE(std::all_of(result.piece_lengths.begin() + static_cast<std::ptrdiff_t>(expected.size()),
                            result.piece_lengths.end(),
                            [](std::size_t length) { return length == kUnchangedLength; }));
}

}  // namespace

TEST(OutboundMessageSplitter, Mtu23MessageOf7IsOnePiece)
{
    expect_pieces(split(7, kMinimumMtu), {7});
}

TEST(OutboundMessageSplitter, Mtu23MessageOf21Is20Then1)
{
    expect_pieces(split(21, kMinimumMtu), {20, 1});
}

TEST(OutboundMessageSplitter, Mtu23MessageOf128IsSixTwentiesAndAnEight)
{
    expect_pieces(split(128, kMinimumMtu), {20, 20, 20, 20, 20, 20, 8});
}

TEST(OutboundMessageSplitter, Mtu185MessageOf128IsOnePiece)
{
    expect_pieces(split(128, kLargerMtu), {128});
}

TEST(OutboundMessageSplitter, RejectsShortMtuEmptyAndOverlong)
{
    expect_unchanged(split(7, 22));
    expect_unchanged(split(7, 0));
    expect_unchanged(split(0, kMinimumMtu));
    expect_unchanged(split(129, kMinimumMtu));
}
