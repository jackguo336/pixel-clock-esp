#pragma once

#include "settings/wifi_credentials_store.hpp"

namespace settings {

inline constexpr char kNvsNamespace[] = "wifi";
inline constexpr char kCredentialsKey[] = "credentials";
inline constexpr uint8_t kCredentialsVersion = 1;
inline constexpr std::size_t kVersionFieldBytes = 1;
inline constexpr std::size_t kSsidLengthFieldBytes = 1;
inline constexpr std::size_t kPassphraseLengthFieldBytes = 1;
inline constexpr std::size_t kMaxRecordBytes = kVersionFieldBytes + kSsidLengthFieldBytes + kMaxSsidBytes
    + kPassphraseLengthFieldBytes + kMaxPassphraseBytes;

/**
 * One Wi-Fi SSID and passphrase in the NVS partition labeled nvs.
 * Use after that partition has been initialized. Does not connect, and does not judge passphrase shape.
 */
class NvsWifiCredentialsStore : public WifiCredentialsStore {
public:
    [[nodiscard]] esp_err_t save(const WifiCredentials& credentials) const override;
    [[nodiscard]] esp_err_t load(WifiCredentials& credentials) const override;
    [[nodiscard]] esp_err_t clear() const override;
};

}  // namespace settings
