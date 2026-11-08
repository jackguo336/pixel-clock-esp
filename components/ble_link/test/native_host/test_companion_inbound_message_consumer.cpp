#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "ble_link/companion_inbound_message_consumer.hpp"
#include "ble_link/set_wifi_credentials_handler.hpp"
#include "companion_protocol/message_decoder.hpp"
#include "companion_protocol/message_encoder.hpp"
#include "fake_inbound_message_handler.hpp"
#include "fake_message_transport.hpp"
#include "fake_wifi_credentials_store.hpp"
#include "gtest/gtest.h"

namespace {

constexpr uint16_t kExampleRequestId = 0x1234;

const std::vector<uint8_t> kGetDeviceInfoExample{0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00};

const std::vector<uint8_t> kSetWifiCredentialsExample{
    0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00, 0x04, 0x68, 0x6F, 0x6D, 0x65, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F,
    0x72, 0x64,
};

const std::vector<uint8_t> kAckOkExample{0x01, 0x80, 0x00, 0x34, 0x12, 0x01, 0x00, 0x00};

struct Session {
    FakeInboundMessageHandler handler{};
    FakeMessageTransport transport{};
    ble_link::InboundMessagePool pool{};
    ble_link::CompanionMessageReplySender reply_sender{transport};
    ble_link::InboundMessageHandler* const handlers[1]{&handler};
    ble_link::CompanionInboundMessageConsumer consumer{pool, handlers, reply_sender};
};

void expect_error(const FakeMessageTransport& transport, companion_protocol::Status code, uint16_t request_id)
{
    ASSERT_EQ(transport.send_count, 1);
    companion_protocol::Message reply{};
    ASSERT_EQ(companion_protocol::decode_message(transport.sent, reply), companion_protocol::Status::Ok);
    EXPECT_EQ(reply.header.version, companion_protocol::kProtocolVersion);
    EXPECT_EQ(reply.header.flags, companion_protocol::kFlagsNone);
    EXPECT_EQ(reply.header.message_type, companion_protocol::MessageType::Error);
    EXPECT_EQ(reply.header.request_id, request_id);
    const auto* error = std::get_if<companion_protocol::Error>(&reply.body);
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->code, code);
}

std::vector<uint8_t> header_with_request_id(uint8_t version, uint8_t message_type, uint8_t flags, uint16_t request_id,
                                            uint16_t body_length)
{
    return {
        version,
        message_type,
        flags,
        static_cast<uint8_t>(request_id),
        static_cast<uint8_t>(request_id >> 8),
        static_cast<uint8_t>(body_length),
        static_cast<uint8_t>(body_length >> 8),
    };
}

class ScriptedHandler : public ble_link::InboundMessageHandler {
public:
    ScriptedHandler(bool claims_request, int id, std::vector<int>& call_order)
        : claims_request_(claims_request)
        , id_(id)
        , call_order_(call_order)
    {
    }

    bool try_handle(const companion_protocol::Message& request) override
    {
        call_order_.push_back(id_);
        last_request_ = request;
        ++try_handle_count_;
        return claims_request_;
    }

    int try_handle_count_{0};
    companion_protocol::Message last_request_{};

private:
    bool claims_request_;
    int id_;
    std::vector<int>& call_order_;
};

void consume_device_info(ble_link::CompanionInboundMessageConsumer& consumer)
{
    companion_protocol::Message request{};
    request.header.request_id = 0x0007;
    request.body = companion_protocol::GetDeviceInfo{};
    std::array<uint8_t, companion_protocol::kMaxMessageBytes> encoded{};
    const std::size_t written = companion_protocol::encode_message(request, encoded);
    ASSERT_GT(written, 0u);
    consumer.consume(std::span<const uint8_t>(encoded.data(), written));
}

ble_link::InboundMessagePoolSlotRef build_message_ref_for_event(const std::optional<uint8_t>& slot_index, std::size_t message_length)
{
    ble_link::InboundMessagePoolSlotRef message_ref{};
    message_ref.slot_index = slot_index.value_or(0);
    message_ref.message_length = static_cast<uint8_t>(message_length);
    return message_ref;
}

}  // namespace

TEST(CompanionInboundMessageConsumer, ConsumeRoutesAValidGetDeviceInfo)
{
    Session session;
    EXPECT_STREQ(session.consumer.name(), "companion_inbound_message_consumer");

    session.consumer.consume(kGetDeviceInfoExample);
    EXPECT_EQ(session.handler.try_handle_count, 1);
    EXPECT_EQ(session.handler.last_request.header.request_id, 0x0001);
    EXPECT_EQ(session.handler.last_request.header.message_type, companion_protocol::MessageType::GetDeviceInfo);
    EXPECT_TRUE(std::holds_alternative<companion_protocol::GetDeviceInfo>(session.handler.last_request.body));
    EXPECT_EQ(session.transport.send_count, 0);
}

TEST(CompanionInboundMessageConsumer, FailedDecodeSendsErrorWithoutCallingAHandler)
{
    const std::vector<uint8_t> too_short{0x01, 0x34, 0x12};
    std::vector<uint8_t> too_long = header_with_request_id(companion_protocol::kProtocolVersion,
                                                           static_cast<uint8_t>(companion_protocol::MessageType::GetDeviceInfo),
                                                           companion_protocol::kFlagsNone, kExampleRequestId, 0);
    too_long.resize(companion_protocol::kMaxMessageBytes + 1);

    auto body_length_mismatch = header_with_request_id(
        companion_protocol::kProtocolVersion, static_cast<uint8_t>(companion_protocol::MessageType::GetDeviceInfo),
        companion_protocol::kFlagsNone, kExampleRequestId, 5);

    auto unknown_type = header_with_request_id(companion_protocol::kProtocolVersion, 0x42,
                                               companion_protocol::kFlagsNone, kExampleRequestId, 0);

    auto unsupported_version = header_with_request_id(2, static_cast<uint8_t>(companion_protocol::MessageType::GetDeviceInfo),
                                                      companion_protocol::kFlagsNone, kExampleRequestId, 0);

    auto unsupported_flags = header_with_request_id(
        companion_protocol::kProtocolVersion, static_cast<uint8_t>(companion_protocol::MessageType::GetDeviceInfo), 0x01,
        kExampleRequestId, 0);

    auto invalid_ssid = header_with_request_id(
        companion_protocol::kProtocolVersion, static_cast<uint8_t>(companion_protocol::MessageType::SetWifiCredentials),
        companion_protocol::kFlagsNone, kExampleRequestId, 1);
    invalid_ssid.push_back(0x00);

    auto malformed_get_device_info = header_with_request_id(
        companion_protocol::kProtocolVersion, static_cast<uint8_t>(companion_protocol::MessageType::GetDeviceInfo),
        companion_protocol::kFlagsNone, kExampleRequestId, 1);
    malformed_get_device_info.push_back(0xFF);

    struct Failure {
        std::string_view name;
        std::span<const uint8_t> bytes;
        companion_protocol::Status status;
        uint16_t request_id;
    };
    const Failure failures[] = {
        {"too short", too_short, companion_protocol::Status::MessageTooShort, 0},
        {"too long", too_long, companion_protocol::Status::MessageTooLong, kExampleRequestId},
        {"body length", body_length_mismatch, companion_protocol::Status::BodyLengthMismatch, kExampleRequestId},
        {"unknown type", unknown_type, companion_protocol::Status::UnknownMessageType, kExampleRequestId},
        {"version", unsupported_version, companion_protocol::Status::UnsupportedVersion, kExampleRequestId},
        {"flags", unsupported_flags, companion_protocol::Status::UnsupportedFlags, kExampleRequestId},
        {"ssid length", invalid_ssid, companion_protocol::Status::InvalidField, kExampleRequestId},
        {"get device info body", malformed_get_device_info, companion_protocol::Status::MalformedBody, kExampleRequestId},
    };

    for (const Failure& failure : failures) {
        SCOPED_TRACE(failure.name);
        Session session;
        session.consumer.consume(failure.bytes);
        EXPECT_EQ(session.handler.try_handle_count, 0);
        expect_error(session.transport, failure.status, failure.request_id);
    }
}

TEST(CompanionInboundMessageConsumer, DeclinedRequestSendsNothing)
{
    Session session;
    session.handler.claims_request = false;
    session.consumer.consume(kGetDeviceInfoExample);
    EXPECT_EQ(session.handler.try_handle_count, 1);
    EXPECT_EQ(session.transport.send_count, 0);
}

TEST(CompanionInboundMessageConsumer, MessageBeforeConnectRemovesEntry)
{
    Session session;
    const std::optional<uint8_t> slot_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(slot_index.has_value());

    session.consumer.on_event(ble_link::companion_message_received_event(
        build_message_ref_for_event(slot_index, kGetDeviceInfoExample.size())));

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    EXPECT_EQ(session.pool.read_then_free_slot(*slot_index, copied), 0u);
}

TEST(CompanionInboundMessageConsumer, MessageAfterDisconnectRemovesEntry)
{
    Session session;
    session.consumer.on_event(ble_link::companion_connected_event());
    session.consumer.on_event(ble_link::companion_disconnected_event());

    const std::optional<uint8_t> slot_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(slot_index.has_value());
    session.consumer.on_event(ble_link::companion_message_received_event(
        build_message_ref_for_event(slot_index, kGetDeviceInfoExample.size())));

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    EXPECT_EQ(session.pool.read_then_free_slot(*slot_index, copied), 0u);
}

TEST(CompanionInboundMessageConsumer, DisconnectLeavesAnUndeliveredEntryHeld)
{
    Session session;
    const std::optional<uint8_t> first = session.pool.try_insert(kGetDeviceInfoExample);
    const std::optional<uint8_t> second = session.pool.try_insert(kSetWifiCredentialsExample);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, 0);
    EXPECT_EQ(*second, 1);

    session.consumer.on_event(ble_link::companion_connected_event());
    session.consumer.on_event(ble_link::companion_disconnected_event());

    const uint8_t filler[] = {0x01};
    EXPECT_TRUE(session.pool.try_insert(filler).has_value());
    EXPECT_TRUE(session.pool.try_insert(filler).has_value());
    const uint32_t drops_before = session.pool.drop_count();
    EXPECT_FALSE(session.pool.try_insert(filler).has_value());
    EXPECT_EQ(session.pool.drop_count(), drops_before + 1);

    session.consumer.on_event(ble_link::companion_message_received_event(
        build_message_ref_for_event(second, kSetWifiCredentialsExample.size())));
    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);

    const std::optional<uint8_t> reused = session.pool.try_insert(filler);
    ASSERT_TRUE(reused.has_value());
    EXPECT_EQ(*reused, *second);
}

TEST(CompanionInboundMessageConsumer, ZeroPayloadSizeDoesNotRemoveEntryZero)
{
    Session session;
    session.consumer.on_event(ble_link::companion_connected_event());
    const std::optional<uint8_t> slot_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(slot_index.has_value());
    ASSERT_EQ(*slot_index, 0);

    platform::Event event{};
    event.type = platform::EventType::CompanionMessageReceived;
    event.payload_size = 0;
    session.consumer.on_event(event);

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    const std::size_t copied_length = session.pool.read_then_free_slot(0, copied);
    ASSERT_EQ(copied_length, kGetDeviceInfoExample.size());
    EXPECT_TRUE(std::equal(kGetDeviceInfoExample.begin(), kGetDeviceInfoExample.end(), copied.begin()));
}

TEST(CompanionInboundMessageConsumer, MismatchedLengthRemovesEntry)
{
    Session session;
    session.consumer.on_event(ble_link::companion_connected_event());
    const std::optional<uint8_t> slot_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(slot_index.has_value());

    ble_link::InboundMessagePoolSlotRef message_ref = build_message_ref_for_event(slot_index, kGetDeviceInfoExample.size());
    message_ref.message_length = 1;
    session.consumer.on_event(ble_link::companion_message_received_event(message_ref));

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    EXPECT_EQ(session.pool.read_then_free_slot(*slot_index, copied), 0u);
}

TEST(CompanionInboundMessageConsumer, TickDoesNotCallTheHandler)
{
    Session session;
    session.consumer.on_event(ble_link::companion_connected_event());
    platform::Event tick{};
    tick.type = platform::EventType::Tick;
    session.consumer.on_event(tick);
    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
}

TEST(CompanionInboundMessageConsumer, ConnectedSetWifiCredentialsSavesAndAcks)
{
    FakeMessageTransport transport;
    ble_link::CompanionMessageReplySender reply_sender(transport);
    FakeWifiCredentialsStore store;
    ble_link::SetWifiCredentialsHandler handler(reply_sender, store);
    ble_link::InboundMessagePool pool;
    ble_link::InboundMessageHandler* const handlers[] = {&handler};
    ble_link::CompanionInboundMessageConsumer consumer(pool, handlers, reply_sender);

    consumer.on_event(ble_link::companion_connected_event());
    const std::optional<uint8_t> slot_index = pool.try_insert(kSetWifiCredentialsExample);
    ASSERT_TRUE(slot_index.has_value());
    consumer.on_event(
        ble_link::companion_message_received_event(build_message_ref_for_event(slot_index, kSetWifiCredentialsExample.size())));

    EXPECT_EQ(store.save_count, 1);
    EXPECT_EQ(store.saved.ssid_length, 4);
    EXPECT_EQ(store.saved.passphrase_length, 8);
    const uint8_t ssid[] = {'h', 'o', 'm', 'e'};
    const uint8_t passphrase[] = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    EXPECT_TRUE(std::equal(std::begin(ssid), std::end(ssid), store.saved.ssid.begin()));
    EXPECT_TRUE(std::equal(std::begin(passphrase), std::end(passphrase), store.saved.passphrase.begin()));
    EXPECT_EQ(transport.send_count, 1);
    EXPECT_EQ(transport.sent, kAckOkExample);
}

TEST(CompanionInboundMessageConsumer, SecondHandlerOwnsRequestWhenFirstDeclines)
{
    std::vector<int> call_order;
    ScriptedHandler first(false, 1, call_order);
    ScriptedHandler second(true, 2, call_order);
    FakeMessageTransport transport;
    ble_link::CompanionMessageReplySender reply_sender(transport);
    ble_link::InboundMessagePool pool;
    ble_link::InboundMessageHandler* const handlers[] = {&first, &second};
    ble_link::CompanionInboundMessageConsumer consumer(pool, handlers, reply_sender);

    consume_device_info(consumer);

    EXPECT_EQ(call_order, (std::vector<int>{1, 2}));
    EXPECT_EQ(second.try_handle_count_, 1);
    EXPECT_EQ(second.last_request_.header.request_id, 0x0007);
    EXPECT_EQ(second.last_request_.header.message_type, companion_protocol::MessageType::GetDeviceInfo);
    ASSERT_TRUE(std::holds_alternative<companion_protocol::GetDeviceInfo>(second.last_request_.body));
    EXPECT_EQ(transport.send_count, 0);
}

TEST(CompanionInboundMessageConsumer, FirstHandlerOwnsRequestAndStopsTheWalk)
{
    std::vector<int> call_order;
    ScriptedHandler first(true, 1, call_order);
    ScriptedHandler second(true, 2, call_order);
    FakeMessageTransport transport;
    ble_link::CompanionMessageReplySender reply_sender(transport);
    ble_link::InboundMessagePool pool;
    ble_link::InboundMessageHandler* const handlers[] = {&first, &second};
    ble_link::CompanionInboundMessageConsumer consumer(pool, handlers, reply_sender);

    consume_device_info(consumer);

    EXPECT_EQ(call_order, (std::vector<int>{1}));
    EXPECT_EQ(second.try_handle_count_, 0);
    EXPECT_EQ(transport.send_count, 0);
}

TEST(CompanionInboundMessageConsumer, EveryHandlerDecliningSendsNothing)
{
    std::vector<int> call_order;
    ScriptedHandler first(false, 1, call_order);
    ScriptedHandler second(false, 2, call_order);
    FakeMessageTransport transport;
    ble_link::CompanionMessageReplySender reply_sender(transport);
    ble_link::InboundMessagePool pool;
    ble_link::InboundMessageHandler* const handlers[] = {&first, &second};
    ble_link::CompanionInboundMessageConsumer consumer(pool, handlers, reply_sender);

    consume_device_info(consumer);

    EXPECT_EQ(call_order, (std::vector<int>{1, 2}));
    EXPECT_EQ(transport.send_count, 0);
}
