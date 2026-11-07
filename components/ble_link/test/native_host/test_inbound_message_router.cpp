#include <array>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>

#include "ble_link/companion_inbound_message_consumer.hpp"
#include "companion_protocol/message_encoder.hpp"
#include "gtest/gtest.h"

namespace {

using companion_protocol::Message;

class ScriptedHandler : public ble_link::InboundMessageHandler {
public:
    ScriptedHandler(bool claims_request, int id, std::vector<int>& call_order)
        : claims_request_(claims_request)
        , id_(id)
        , call_order_(call_order)
    {
    }

    bool try_handle(const Message& request) override
    {
        call_order_.push_back(id_);
        last_request_ = request;
        ++try_handle_count_;
        return claims_request_;
    }

    int try_handle_count_{0};
    Message last_request_{};

private:
    bool claims_request_;
    int id_;
    std::vector<int>& call_order_;
};

class RecordingMessageTransport : public ble_link::MessageTransport {
public:
    bool send(std::span<const uint8_t> message) override
    {
        (void)message;
        ++send_count;
        return true;
    }

    int send_count{0};
};

void consume_device_info(ble_link::CompanionInboundMessageConsumer& consumer)
{
    Message request{};
    request.header.request_id = 0x0007;
    request.body = companion_protocol::GetDeviceInfo{};
    std::array<uint8_t, companion_protocol::kMaxMessageBytes> encoded{};
    const std::size_t written = companion_protocol::encode_message(request, encoded);
    ASSERT_GT(written, 0u);
    consumer.consume(std::span<const uint8_t>(encoded.data(), written));
}

}  // namespace

TEST(CompanionInboundMessageConsumer, SecondHandlerOwnsRequestWhenFirstDeclines)
{
    std::vector<int> call_order;
    ScriptedHandler first(false, 1, call_order);
    ScriptedHandler second(true, 2, call_order);
    RecordingMessageTransport transport;
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
    RecordingMessageTransport transport;
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
    RecordingMessageTransport transport;
    ble_link::CompanionMessageReplySender reply_sender(transport);
    ble_link::InboundMessagePool pool;
    ble_link::InboundMessageHandler* const handlers[] = {&first, &second};
    ble_link::CompanionInboundMessageConsumer consumer(pool, handlers, reply_sender);

    consume_device_info(consumer);

    EXPECT_EQ(call_order, (std::vector<int>{1, 2}));
    EXPECT_EQ(transport.send_count, 0);
}
