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
#include "gtest/gtest.h"

namespace {

constexpr uint16_t kExampleRequestId = 0x1234;

const std::vector<uint8_t> kGetDeviceInfoExample{0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00};

const std::vector<uint8_t> kSetWifiCredentialsExample{
    0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00, 0x04, 0x68, 0x6F, 0x6D, 0x65, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F,
    0x72, 0x64,
};

const std::vector<uint8_t> kAckOkExample{0x01, 0x80, 0x00, 0x34, 0x12, 0x01, 0x00, 0x00};

class RecordingMessageTransport : public ble_link::MessageTransport {
public:
    bool send(std::span<const uint8_t> message) override
    {
        ++send_count;
        sent.assign(message.begin(), message.end());
        return true;
    }

    int send_count{0};
    std::vector<uint8_t> sent;
};

class RecordingInboundMessageHandler : public ble_link::InboundMessageHandler {
public:
    bool try_handle(const companion_protocol::Message& request) override
    {
        ++try_handle_count;
        last_request = request;
        return claims_request;
    }

    bool claims_request{true};
    int try_handle_count{0};
    companion_protocol::Message last_request{};
};

class ProgrammedWifiCredentialsStore : public settings::WifiCredentialsStore {
public:
    esp_err_t save(const settings::WifiCredentials& credentials) const override
    {
        ++save_count;
        saved = credentials;
        return ESP_OK;
    }

    esp_err_t load(settings::WifiCredentials&) const override
    {
        return ESP_FAIL;
    }

    esp_err_t clear() const override
    {
        return ESP_FAIL;
    }

    mutable int save_count{0};
    mutable settings::WifiCredentials saved{};
};

struct Session {
    RecordingInboundMessageHandler handler{};
    RecordingMessageTransport transport{};
    ble_link::InboundMessagePool pool{};
    ble_link::InboundMessageHandler* const handlers[1]{&handler};
    ble_link::InboundMessageRouter router{handlers};
    ble_link::CompanionInboundMessageConsumer consumer{pool, router, transport};
};

void expect_error(const RecordingMessageTransport& transport, companion_protocol::Status code, uint16_t request_id)
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

ble_link::InboundMessagePoolEntryRef inserted_ref(const std::optional<uint8_t>& entry_index, std::size_t message_length)
{
    ble_link::InboundMessagePoolEntryRef handoff{};
    handoff.entry_index = entry_index.value_or(0);
    handoff.message_length = static_cast<uint8_t>(message_length);
    return handoff;
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

TEST(CompanionInboundMessageConsumer, FailedDecodeSendsErrorAndSkipsTheRouter)
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
    const std::optional<uint8_t> entry_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(entry_index.has_value());

    session.consumer.on_event(ble_link::companion_message_received_event(
        inserted_ref(entry_index, kGetDeviceInfoExample.size())));

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    EXPECT_EQ(session.pool.read_then_remove(*entry_index, copied), 0u);
}

TEST(CompanionInboundMessageConsumer, MessageAfterDisconnectRemovesEntry)
{
    Session session;
    session.consumer.on_event(ble_link::companion_connected_event());
    session.consumer.on_event(ble_link::companion_disconnected_event());

    const std::optional<uint8_t> entry_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(entry_index.has_value());
    session.consumer.on_event(ble_link::companion_message_received_event(
        inserted_ref(entry_index, kGetDeviceInfoExample.size())));

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    EXPECT_EQ(session.pool.read_then_remove(*entry_index, copied), 0u);
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
        inserted_ref(second, kSetWifiCredentialsExample.size())));
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
    const std::optional<uint8_t> entry_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(entry_index.has_value());
    ASSERT_EQ(*entry_index, 0);

    platform::Event event{};
    event.type = platform::EventType::CompanionMessageReceived;
    event.payload_size = 0;
    session.consumer.on_event(event);

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    const std::size_t copied_length = session.pool.read_then_remove(0, copied);
    ASSERT_EQ(copied_length, kGetDeviceInfoExample.size());
    EXPECT_TRUE(std::equal(kGetDeviceInfoExample.begin(), kGetDeviceInfoExample.end(), copied.begin()));
}

TEST(CompanionInboundMessageConsumer, MismatchedLengthRemovesEntry)
{
    Session session;
    session.consumer.on_event(ble_link::companion_connected_event());
    const std::optional<uint8_t> entry_index = session.pool.try_insert(kGetDeviceInfoExample);
    ASSERT_TRUE(entry_index.has_value());

    ble_link::InboundMessagePoolEntryRef handoff = inserted_ref(entry_index, kGetDeviceInfoExample.size());
    handoff.message_length = 1;
    session.consumer.on_event(ble_link::companion_message_received_event(handoff));

    EXPECT_EQ(session.handler.try_handle_count, 0);
    EXPECT_EQ(session.transport.send_count, 0);
    std::array<uint8_t, ble_link::InboundMessagePool::kMessageBytes> copied{};
    EXPECT_EQ(session.pool.read_then_remove(*entry_index, copied), 0u);
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
    RecordingMessageTransport transport;
    ProgrammedWifiCredentialsStore store;
    ble_link::SetWifiCredentialsHandler handler(transport, store);
    ble_link::InboundMessagePool pool;
    ble_link::InboundMessageHandler* const handlers[] = {&handler};
    ble_link::InboundMessageRouter router(handlers);
    ble_link::CompanionInboundMessageConsumer consumer(pool, router, transport);

    consumer.on_event(ble_link::companion_connected_event());
    const std::optional<uint8_t> entry_index = pool.try_insert(kSetWifiCredentialsExample);
    ASSERT_TRUE(entry_index.has_value());
    consumer.on_event(
        ble_link::companion_message_received_event(inserted_ref(entry_index, kSetWifiCredentialsExample.size())));

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
