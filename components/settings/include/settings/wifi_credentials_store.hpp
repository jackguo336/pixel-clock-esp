#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"

namespace settings {

inline constexpr char kNvsNamespace[] = "wifi";
inline constexpr char kCredentialsKey[] = "credentials";
inline constexpr uint8_t kCredentialsVersion = 1;
inline constexpr std::size_t kMinSsidBytes = 1;
inline constexpr std::size_t kMaxSsidBytes = 32;
inline constexpr std::size_t kMaxPassphraseBytes = 64;
inline constexpr std::size_t kMaxRecordBytes = 3 + kMaxSsidBytes + kMaxPassphraseBytes;

struct WifiCredentials {
    std::array<uint8_t, kMaxSsidBytes> ssid{};
    uint8_t ssid_length{0};
    std::array<uint8_t, kMaxPassphraseBytes> passphrase{};
    uint8_t passphrase_length{0};
};

/**
 * One Wi-Fi SSID and passphrase in the default NVS partition.
 * Use after initialize_nvs. Does not connect, and does not judge passphrase shape.
 */
class WifiCredentialsStore {
public:
    /// Returns ESP_ERR_INVALID_ARG and does not write when a length is outside 1..32 or 0..64.
    [[nodiscard]] esp_err_t save(const WifiCredentials& credentials) const;

    /// Leaves credentials unchanged on failure.
    [[nodiscard]] esp_err_t load(WifiCredentials& credentials) const;

    /// Returns ESP_OK when the key is already absent.
    [[nodiscard]] esp_err_t clear() const;
};

}  // namespace settings
