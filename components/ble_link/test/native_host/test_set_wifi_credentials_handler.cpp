#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "ble_link/set_wifi_credentials_handler.hpp"
#include "companion_protocol/message_decoder.hpp"
#include "gtest/gtest.h"

namespace {

constexpr uint16_t kExampleRequestId = 0x1234;

const std::vector<uint8_t> kSetWifiCredentialsExample{
    0x01, 0x02, 0x00, 0x34, 0x12, 0x0E, 0x00, 0x04, 0x68, 0x6F, 0x6D, 0x65, 0x08, 0x70, 0x61, 0x73, 0x73, 0x77, 0x6F,
    0x72, 0x64,
};

const std::vector<uint8_t> kAckOkExample{0x01, 0x80, 0x00, 0x34, 0x12, 0x01, 0x00, 0x00};

const std::vector<uint8_t> kAckStorageFailed{0x01, 0x80, 0x00, 0x34, 0x12, 0x01, 0x00, 0x20};

const std::vector<uint8_t> kErrorInvalidField{0x01, 0xFF, 0x00, 0x34, 0x12, 0x01, 0x00, 0x08};

class RecordingMessageTransport : public ble_link::MessageTransport {
public:
    explicit RecordingMessageTransport(bool send_result = true)
        : send_result_(send_result)
    {
    }

    bool send(std::span<const uint8_t> message) override
    {
        ++send_count;
        sent.assign(message.begin(), message.end());
        return send_result_;
    }

    int send_count{0};
    std::vector<uint8_t> sent;

private:
    bool send_result_;
};

class ProgrammedWifiCredentialsStore : public settings::WifiCredentialsStore {
public:
    explicit ProgrammedWifiCredentialsStore(esp_err_t save_result)
        : save_result_(save_result)
    {
    }

    esp_err_t save(const settings::WifiCredentials& credentials) const override
    {
        ++save_count;
        saved = credentials;
        return save_result_;
    }

    esp_err_t load(settings::WifiCredentials& credentials) const override
    {
        ++load_count;
        (void)credentials;
        return ESP_FAIL;
    }

    esp_err_t clear() const override
    {
        ++clear_count;
        return ESP_FAIL;
    }

    mutable int save_count{0};
    mutable int load_count{0};
    mutable int clear_count{0};
    mutable settings::WifiCredentials saved{};

private:
    esp_err_t save_result_;
};

template <typename Body>
companion_protocol::Message request_with(Body body, uint16_t request_id)
{
    companion_protocol::Message request{};
    request.header.request_id = request_id;
    request.body = std::move(body);
    return request;
}

companion_protocol::SetWifiCredentials credentials_for(std::string_view ssid, std::span<const uint8_t> passphrase)
{
    companion_protocol::SetWifiCredentials credentials{};
    EXPECT_TRUE(credentials.ssid.assign(ssid));
    EXPECT_TRUE(credentials.passphrase.assign(passphrase));
    return credentials;
}

companion_protocol::Message passphrase_request(std::span<const uint8_t> passphrase)
{
    return request_with(credentials_for("home", passphrase), kExampleRequestId);
}

void expect_store_unused(const ProgrammedWifiCredentialsStore& store)
{
    EXPECT_EQ(store.save_count, 0);
    EXPECT_EQ(store.load_count, 0);
    EXPECT_EQ(store.clear_count, 0);
}

}  // namespace

TEST(SetWifiCredentialsHandler, SavesProtocolExampleAndAcksOk)
{
    RecordingMessageTransport transport;
    ProgrammedWifiCredentialsStore store(ESP_OK);
    ble_link::SetWifiCredentialsHandler handler(transport, store);

    companion_protocol::Message request{};
    ASSERT_EQ(companion_protocol::decode_message(kSetWifiCredentialsExample, request), companion_protocol::Status::Ok);

    EXPECT_TRUE(handler.try_handle(request));
    EXPECT_EQ(store.save_count, 1);
    EXPECT_EQ(store.load_count, 0);
    EXPECT_EQ(store.clear_count, 0);
    EXPECT_EQ(store.saved.ssid_length, 4);
    EXPECT_EQ(store.saved.passphrase_length, 8);
    const uint8_t ssid[] = {'h', 'o', 'm', 'e'};
    const uint8_t passphrase[] = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    EXPECT_TRUE(std::equal(std::begin(ssid), std::end(ssid), store.saved.ssid.begin()));
    EXPECT_TRUE(std::equal(std::begin(passphrase), std::end(passphrase), store.saved.passphrase.begin()));
    EXPECT_EQ(transport.send_count, 1);
    EXPECT_EQ(transport.sent, kAckOkExample);
}

TEST(SetWifiCredentialsHandler, StorageFailureAcksStorageFailed)
{
    RecordingMessageTransport transport;
    ProgrammedWifiCredentialsStore store(ESP_FAIL);
    ble_link::SetWifiCredentialsHandler handler(transport, store);

    companion_protocol::Message request{};
    ASSERT_EQ(companion_protocol::decode_message(kSetWifiCredentialsExample, request), companion_protocol::Status::Ok);

    EXPECT_TRUE(handler.try_handle(request));
    EXPECT_EQ(store.save_count, 1);
    EXPECT_EQ(store.load_count, 0);
    EXPECT_EQ(store.clear_count, 0);
    EXPECT_EQ(transport.send_count, 1);
    EXPECT_EQ(transport.sent, kAckStorageFailed);
}

TEST(SetWifiCredentialsHandler, SavesSsidBytesThatContainZero)
{
    RecordingMessageTransport transport;
    ProgrammedWifiCredentialsStore store(ESP_OK);
    ble_link::SetWifiCredentialsHandler handler(transport, store);

    const uint8_t ssid[] = {'h', 0x00, 'm', 'e'};
    const uint8_t passphrase[] = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    companion_protocol::SetWifiCredentials credentials{};
    ASSERT_TRUE(credentials.ssid.assign(std::span<const uint8_t>(ssid, sizeof(ssid))));
    ASSERT_TRUE(credentials.passphrase.assign(std::span<const uint8_t>(passphrase, sizeof(passphrase))));

    EXPECT_TRUE(handler.try_handle(request_with(credentials, kExampleRequestId)));
    EXPECT_EQ(store.save_count, 1);
    EXPECT_EQ(store.saved.ssid_length, sizeof(ssid));
    EXPECT_TRUE(std::equal(std::begin(ssid), std::end(ssid), store.saved.ssid.begin()));
    EXPECT_EQ(transport.sent, kAckOkExample);
}

TEST(SetWifiCredentialsHandler, RejectsPassphrasesOutsideWpaRules)
{
    const uint8_t length_seven[] = {'1', '2', '3', '4', '5', '6', '7'};
    const uint8_t interior_zero[] = {'p', 'a', 's', 's', 0x00, 'o', 'r', 'd'};
    const uint8_t unit_separator[] = {0x1F, 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    const uint8_t delete_byte[] = {0x7F, 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    std::array<uint8_t, 64> sixty_four_g{};
    sixty_four_g.fill('g');

    const std::span<const uint8_t> rejected[] = {length_seven, interior_zero, unit_separator, delete_byte,
                                                 sixty_four_g};

    for (const std::span<const uint8_t> passphrase : rejected) {
        RecordingMessageTransport transport;
        ProgrammedWifiCredentialsStore store(ESP_OK);
        ble_link::SetWifiCredentialsHandler handler(transport, store);

        EXPECT_TRUE(handler.try_handle(passphrase_request(passphrase)));
        expect_store_unused(store);
        EXPECT_EQ(transport.send_count, 1);
        EXPECT_EQ(transport.sent, kErrorInvalidField);
    }
}

TEST(SetWifiCredentialsHandler, AcceptsOpenPrintableAndHexPassphrases)
{
    const uint8_t eight_spaces[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    std::array<uint8_t, 63> sixty_three_g{};
    sixty_three_g.fill('g');
    const char hex_digits[] = "0123456789abcdefABCDEF0123456789abcdefABCDEF0123456789abcdefABCD";
    static_assert(sizeof(hex_digits) - 1 == 64);
    const auto hex = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(hex_digits), 64);

    const std::span<const uint8_t> allowed[] = {
        {},
        eight_spaces,
        sixty_three_g,
        hex,
    };

    for (const std::span<const uint8_t> passphrase : allowed) {
        RecordingMessageTransport transport;
        ProgrammedWifiCredentialsStore store(ESP_OK);
        ble_link::SetWifiCredentialsHandler handler(transport, store);

        EXPECT_TRUE(handler.try_handle(passphrase_request(passphrase)));
        EXPECT_EQ(store.save_count, 1);
        EXPECT_EQ(store.load_count, 0);
        EXPECT_EQ(store.clear_count, 0);
        EXPECT_EQ(store.saved.passphrase_length, passphrase.size());
        EXPECT_TRUE(std::equal(passphrase.begin(), passphrase.end(), store.saved.passphrase.begin()));
        EXPECT_EQ(transport.send_count, 1);
        EXPECT_EQ(transport.sent, kAckOkExample);
    }
}

TEST(SetWifiCredentialsHandler, IgnoresOtherBodies)
{
    RecordingMessageTransport transport;
    ProgrammedWifiCredentialsStore store(ESP_OK);
    ble_link::SetWifiCredentialsHandler handler(transport, store);

    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::GetDeviceInfo{}, kExampleRequestId)));
    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::Ack{}, kExampleRequestId)));
    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::DeviceInfo{}, kExampleRequestId)));
    EXPECT_FALSE(handler.try_handle(request_with(companion_protocol::Error{}, kExampleRequestId)));
    expect_store_unused(store);
    EXPECT_EQ(transport.send_count, 0);
}

TEST(SetWifiCredentialsHandler, FailedSendStillSavedOnce)
{
    RecordingMessageTransport transport(false);
    ProgrammedWifiCredentialsStore store(ESP_OK);
    ble_link::SetWifiCredentialsHandler handler(transport, store);

    companion_protocol::Message request{};
    ASSERT_EQ(companion_protocol::decode_message(kSetWifiCredentialsExample, request), companion_protocol::Status::Ok);

    EXPECT_TRUE(handler.try_handle(request));
    EXPECT_EQ(store.save_count, 1);
    EXPECT_EQ(transport.send_count, 1);
    EXPECT_EQ(transport.sent, kAckOkExample);
}
