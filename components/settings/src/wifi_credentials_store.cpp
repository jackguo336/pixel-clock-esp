#include "settings/wifi_credentials_store.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_log.h"
#include "nvs.h"

namespace settings {
namespace {

constexpr char kTag[] = "settings";

static_assert(sizeof(kNvsNamespace) <= NVS_NS_NAME_MAX_SIZE);
static_assert(sizeof(kCredentialsKey) <= NVS_KEY_NAME_MAX_SIZE);

constexpr std::size_t kRecordOverheadBytes = kMaxRecordBytes - kMaxSsidBytes - kMaxPassphraseBytes;
static_assert(kRecordOverheadBytes == 3);

constexpr std::size_t kVersionOffset = 0;
constexpr std::size_t kSsidLengthOffset = 1;
constexpr std::size_t kSsidOffset = 2;

// nvs_handle_t is an integer and 0 is not a sentinel.
class OpenedNvsHandle {
public:
    OpenedNvsHandle() = default;

    OpenedNvsHandle(const OpenedNvsHandle&) = delete;
    OpenedNvsHandle& operator=(const OpenedNvsHandle&) = delete;

    ~OpenedNvsHandle()
    {
        if (open_) {
            nvs_close(handle_);
        }
    }

    [[nodiscard]] esp_err_t open(nvs_open_mode_t mode)
    {
        const esp_err_t err = nvs_open(kNvsNamespace, mode, &handle_);
        open_ = err == ESP_OK;
        return err;
    }

    [[nodiscard]] nvs_handle_t get() const
    {
        return handle_;
    }

private:
    nvs_handle_t handle_{};
    bool open_{false};
};

void log_failure(const char* action, esp_err_t err)
{
    ESP_LOGE(kTag, "%s: %s", action, esp_err_to_name(err));
}

bool lengths_accepted(const WifiCredentials& credentials)
{
    const std::size_t ssid_length = credentials.ssid_length;
    const std::size_t passphrase_length = credentials.passphrase_length;
    return ssid_length >= kMinSsidBytes && ssid_length <= kMaxSsidBytes && passphrase_length <= kMaxPassphraseBytes;
}

std::size_t record_size(uint8_t ssid_length, uint8_t passphrase_length)
{
    return kRecordOverheadBytes + ssid_length + passphrase_length;
}

void encode(const WifiCredentials& credentials, std::array<uint8_t, kMaxRecordBytes>& record)
{
    const std::size_t ssid_length = credentials.ssid_length;
    const std::size_t passphrase_length = credentials.passphrase_length;
    uint8_t* const passphrase_length_byte = record.data() + kSsidOffset + ssid_length;

    record.fill(0);
    record[kVersionOffset] = kCredentialsVersion;
    record[kSsidLengthOffset] = credentials.ssid_length;
    std::copy_n(credentials.ssid.begin(), ssid_length, record.data() + kSsidOffset);
    *passphrase_length_byte = credentials.passphrase_length;
    std::copy_n(credentials.passphrase.begin(), passphrase_length, passphrase_length_byte + 1);
}

esp_err_t decode(const uint8_t* record, std::size_t size, WifiCredentials& credentials)
{
    if (size == 0 || size > kMaxRecordBytes) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (record[kVersionOffset] != kCredentialsVersion) {
        return ESP_ERR_INVALID_VERSION;
    }
    if (size <= kSsidLengthOffset) {
        return ESP_ERR_INVALID_SIZE;
    }

    const uint8_t ssid_length = record[kSsidLengthOffset];
    const std::size_t ssid_bytes = ssid_length;
    if (ssid_bytes < kMinSsidBytes || ssid_bytes > kMaxSsidBytes) {
        return ESP_ERR_INVALID_SIZE;
    }

    const std::size_t passphrase_length_offset = kSsidOffset + ssid_length;
    if (size <= passphrase_length_offset) {
        return ESP_ERR_INVALID_SIZE;
    }

    const uint8_t passphrase_length = record[passphrase_length_offset];
    if (static_cast<std::size_t>(passphrase_length) > kMaxPassphraseBytes) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (size != record_size(ssid_length, passphrase_length)) {
        return ESP_ERR_INVALID_SIZE;
    }

    WifiCredentials decoded{};
    decoded.ssid_length = ssid_length;
    decoded.passphrase_length = passphrase_length;
    std::copy_n(record + kSsidOffset, ssid_length, decoded.ssid.begin());
    std::copy_n(record + passphrase_length_offset + 1, passphrase_length, decoded.passphrase.begin());
    credentials = decoded;
    return ESP_OK;
}

}  // namespace

esp_err_t WifiCredentialsStore::save(const WifiCredentials& credentials) const
{
    if (!lengths_accepted(credentials)) {
        log_failure("save", ESP_ERR_INVALID_ARG);
        return ESP_ERR_INVALID_ARG;
    }

    std::array<uint8_t, kMaxRecordBytes> record{};
    encode(credentials, record);

    OpenedNvsHandle nvs;
    const esp_err_t open_err = nvs.open(NVS_READWRITE);
    if (open_err != ESP_OK) {
        log_failure("save", open_err);
        return open_err;
    }

    const std::size_t size = record_size(credentials.ssid_length, credentials.passphrase_length);
    const esp_err_t set_err = nvs_set_blob(nvs.get(), kCredentialsKey, record.data(), size);
    if (set_err != ESP_OK) {
        log_failure("save", set_err);
        return set_err;
    }

    const esp_err_t commit_err = nvs_commit(nvs.get());
    if (commit_err != ESP_OK) {
        log_failure("save", commit_err);
    }
    return commit_err;
}

esp_err_t WifiCredentialsStore::load(WifiCredentials& credentials) const
{
    OpenedNvsHandle nvs;
    const esp_err_t open_err = nvs.open(NVS_READONLY);
    if (open_err != ESP_OK) {
        if (open_err != ESP_ERR_NVS_NOT_FOUND) {
            log_failure("load", open_err);
        }
        return open_err;
    }

    std::array<uint8_t, kMaxRecordBytes> record{};
    std::size_t blob_size = record.size();
    const esp_err_t read_err = nvs_get_blob(nvs.get(), kCredentialsKey, record.data(), &blob_size);
    if (read_err == ESP_ERR_NVS_NOT_FOUND) {
        return read_err;
    }
    if (read_err == ESP_ERR_NVS_INVALID_LENGTH) {
        log_failure("load", ESP_ERR_INVALID_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }
    if (read_err != ESP_OK) {
        log_failure("load", read_err);
        return read_err;
    }
    if (blob_size == 0 || blob_size > kMaxRecordBytes) {
        log_failure("load", ESP_ERR_INVALID_SIZE);
        return ESP_ERR_INVALID_SIZE;
    }

    WifiCredentials decoded{};
    const esp_err_t decode_err = decode(record.data(), blob_size, decoded);
    if (decode_err != ESP_OK) {
        log_failure("load", decode_err);
        return decode_err;
    }

    credentials = decoded;
    return ESP_OK;
}

esp_err_t WifiCredentialsStore::clear() const
{
    OpenedNvsHandle nvs;
    const esp_err_t open_err = nvs.open(NVS_READWRITE);
    if (open_err != ESP_OK) {
        log_failure("clear", open_err);
        return open_err;
    }

    const esp_err_t erase_err = nvs_erase_key(nvs.get(), kCredentialsKey);
    if (erase_err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (erase_err != ESP_OK) {
        log_failure("clear", erase_err);
        return erase_err;
    }

    const esp_err_t commit_err = nvs_commit(nvs.get());
    if (commit_err != ESP_OK) {
        log_failure("clear", commit_err);
    }
    return commit_err;
}

}  // namespace settings
