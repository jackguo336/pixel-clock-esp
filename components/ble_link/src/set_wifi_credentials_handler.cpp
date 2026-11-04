#include "ble_link/set_wifi_credentials_handler.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <variant>

#include "companion_protocol/ack.hpp"
#include "companion_protocol/error.hpp"
#include "companion_protocol/set_wifi_credentials.hpp"
#include "esp_err.h"
#include "esp_log.h"

namespace ble_link {
namespace {

constexpr char kTag[] = "set_wifi_credentials_handler";

constexpr std::size_t kShortestPrintablePassphraseBytes = 8;
constexpr std::size_t kLongestPrintablePassphraseBytes = 63;
constexpr std::size_t kHexPassphraseBytes = 64;
constexpr uint8_t kLowestPrintableAscii = 0x20;
constexpr uint8_t kHighestPrintableAscii = 0x7E;

static_assert(settings::kMinSsidBytes == companion_protocol::kMinSsidBytes);
static_assert(settings::kMaxSsidBytes == companion_protocol::kMaxSsidBytes);
static_assert(settings::kMaxPassphraseBytes == companion_protocol::kMaxPassphraseBytes);

[[nodiscard]] bool is_printable_ascii(uint8_t byte)
{
    return byte >= kLowestPrintableAscii && byte <= kHighestPrintableAscii;
}

[[nodiscard]] bool is_hex_digit(uint8_t byte)
{
    const bool decimal = byte >= '0' && byte <= '9';
    const bool lower = byte >= 'a' && byte <= 'f';
    const bool upper = byte >= 'A' && byte <= 'F';
    return decimal || lower || upper;
}

[[nodiscard]] bool passphrase_allowed(std::span<const uint8_t> passphrase)
{
    if (passphrase.empty()) {
        return true;
    }
    if (passphrase.size() == kHexPassphraseBytes) {
        return std::all_of(passphrase.begin(), passphrase.end(), is_hex_digit);
    }
    if (passphrase.size() >= kShortestPrintablePassphraseBytes
        && passphrase.size() <= kLongestPrintablePassphraseBytes) {
        return std::all_of(passphrase.begin(), passphrase.end(), is_printable_ascii);
    }
    return false;
}

void copy_field(std::span<const uint8_t> bytes, std::span<uint8_t> destination, uint8_t& length)
{
    std::copy_n(bytes.begin(), bytes.size(), destination.begin());
    length = static_cast<uint8_t>(bytes.size());
}

}  // namespace

SetWifiCredentialsHandler::SetWifiCredentialsHandler(MessageTransport& transport,
                                                     settings::WifiCredentialsStore& wifi_credentials)
    : transport_(transport)
    , wifi_credentials_(wifi_credentials)
{
}

bool SetWifiCredentialsHandler::try_handle(const companion_protocol::Message& request)
{
    const auto* credentials = std::get_if<companion_protocol::SetWifiCredentials>(&request.body);
    if (credentials == nullptr) {
        return false;
    }

    const std::span<const uint8_t> passphrase = credentials->passphrase.view();
    if (!passphrase_allowed(passphrase)) {
        send_reply(transport_, request.header.request_id,
                   companion_protocol::MessageBody{
                       companion_protocol::Error{.code = companion_protocol::Status::InvalidField}});
        return true;
    }

    settings::WifiCredentials stored{};
    copy_field(credentials->ssid.view(), stored.ssid, stored.ssid_length);
    copy_field(passphrase, stored.passphrase, stored.passphrase_length);

    const esp_err_t save_result = wifi_credentials_.save(stored);
    if (save_result != ESP_OK) {
        ESP_LOGE(kTag, "save: %s", esp_err_to_name(save_result));
        send_reply(transport_, request.header.request_id,
                   companion_protocol::MessageBody{
                       companion_protocol::Ack{.status = companion_protocol::Status::StorageFailed}});
        return true;
    }

    send_reply(transport_, request.header.request_id,
               companion_protocol::MessageBody{companion_protocol::Ack{.status = companion_protocol::Status::Ok}});
    return true;
}

}  // namespace ble_link
