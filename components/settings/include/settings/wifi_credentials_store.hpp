#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"

namespace settings {

inline constexpr std::size_t kMinSsidBytes = 1;
inline constexpr std::size_t kMaxSsidBytes = 32;
inline constexpr std::size_t kMaxPassphraseBytes = 64;

struct WifiCredentials {
    std::array<uint8_t, kMaxSsidBytes> ssid{};
    uint8_t ssid_length{0};
    std::array<uint8_t, kMaxPassphraseBytes> passphrase{};
    uint8_t passphrase_length{0};
};

/**
 * One Wi-Fi SSID and passphrase.
 * Use when a caller saves, loads, or clears credentials without choosing the storage.
 */
class WifiCredentialsStore {
public:
    virtual ~WifiCredentialsStore() = default;

    WifiCredentialsStore(const WifiCredentialsStore&) = delete;
    WifiCredentialsStore& operator=(const WifiCredentialsStore&) = delete;

    /// Returns ESP_ERR_INVALID_ARG and does not write when ssid_length is outside 1..32
    /// or passphrase_length is above 64.
    [[nodiscard]] virtual esp_err_t save(const WifiCredentials& credentials) const = 0;

    [[nodiscard]] virtual esp_err_t load(WifiCredentials& credentials) const = 0;

    /// Returns ESP_OK when the key is already absent.
    [[nodiscard]] virtual esp_err_t clear() const = 0;

protected:
    WifiCredentialsStore() = default;
};

}  // namespace settings
