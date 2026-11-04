#include <cstdint>
#include <variant>
#include <vector>

#include "ble_link/inbound_message_router.hpp"
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

Message device_info_request()
{
    Message request{};
    request.header.request_id = 0x0007;
    request.header.message_type = companion_protocol::MessageType::GetDeviceInfo;
    request.body = companion_protocol::GetDeviceInfo{};
    return request;
}

}  // namespace

TEST(InboundMessageRouter, SecondHandlerOwnsRequestWhenFirstDeclines)
{
    std::vector<int> call_order;
    ScriptedHandler first(false, 1, call_order);
    ScriptedHandler second(true, 2, call_order);
    ble_link::InboundMessageHandler* const handlers[] = {&first, &second};
    ble_link::InboundMessageRouter router(handlers);

    const Message request = device_info_request();
    EXPECT_TRUE(router.route(request));
    EXPECT_EQ(call_order, (std::vector<int>{1, 2}));
    EXPECT_EQ(second.try_handle_count_, 1);
    EXPECT_EQ(second.last_request_.header.request_id, request.header.request_id);
    EXPECT_EQ(second.last_request_.header.message_type, companion_protocol::MessageType::GetDeviceInfo);
    ASSERT_TRUE(std::holds_alternative<companion_protocol::GetDeviceInfo>(second.last_request_.body));
}

TEST(InboundMessageRouter, FirstHandlerOwnsRequestAndStopsTheWalk)
{
    std::vector<int> call_order;
    ScriptedHandler first(true, 1, call_order);
    ScriptedHandler second(true, 2, call_order);
    ble_link::InboundMessageHandler* const handlers[] = {&first, &second};
    ble_link::InboundMessageRouter router(handlers);

    EXPECT_TRUE(router.route(device_info_request()));
    EXPECT_EQ(call_order, (std::vector<int>{1}));
    EXPECT_EQ(second.try_handle_count_, 0);
}

TEST(InboundMessageRouter, ReturnsFalseWhenEveryHandlerDeclines)
{
    std::vector<int> call_order;
    ScriptedHandler first(false, 1, call_order);
    ScriptedHandler second(false, 2, call_order);
    ble_link::InboundMessageHandler* const handlers[] = {&first, &second};
    ble_link::InboundMessageRouter router(handlers);

    EXPECT_FALSE(router.route(device_info_request()));
    EXPECT_EQ(call_order, (std::vector<int>{1, 2}));
}
