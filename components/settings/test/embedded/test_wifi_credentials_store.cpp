#include "unity.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "nvs.h"
#include "nvs_flash.h"
#include "settings/nvs_init.hpp"
#include "settings/wifi_credentials_store.hpp"

namespace {

settings::WifiCredentials fake_credentials()
{
    settings::WifiCredentials credentials{};
    credentials.ssid.fill(0xA5);
    credentials.passphrase.fill(0x5A);
    credentials.ssid_length = 0x11;
    credentials.passphrase_length = 0x22;
    return credentials;
}

settings::WifiCredentials make_credentials(uint8_t ssid_length, uint8_t passphrase_length, uint8_t ssid_seed,
                                           uint8_t passphrase_seed)
{
    settings::WifiCredentials credentials{};
    credentials.ssid_length = ssid_length;
    credentials.passphrase_length = passphrase_length;
    for (uint8_t i = 0; i < ssid_length && i < settings::kMaxSsidBytes; ++i) {
        credentials.ssid[i] = static_cast<uint8_t>(ssid_seed + i);
    }
    for (uint8_t i = 0; i < passphrase_length && i < settings::kMaxPassphraseBytes; ++i) {
        credentials.passphrase[i] = static_cast<uint8_t>(passphrase_seed + i);
    }
    return credentials;
}

void assert_load_error(settings::WifiCredentialsStore& store, esp_err_t expected)
{
    settings::WifiCredentials output = fake_credentials();
    const settings::WifiCredentials unchanged = output;
    TEST_ASSERT_EQUAL(expected, store.load(output));
    TEST_ASSERT_EQUAL_MEMORY(&unchanged, &output, sizeof(output));
}

esp_err_t store_credentials_blob(const uint8_t* bytes, std::size_t byte_count)
{
    nvs_handle_t handle = 0;
    const esp_err_t open_err = nvs_open(settings::kNvsNamespace, NVS_READWRITE, &handle);
    if (open_err != ESP_OK) {
        return open_err;
    }

    const esp_err_t set_err = nvs_set_blob(handle, settings::kCredentialsKey, bytes, byte_count);
    const esp_err_t commit_err = set_err == ESP_OK ? nvs_commit(handle) : set_err;
    nvs_close(handle);
    return commit_err;
}

}  // namespace

// The shared Unity runner has one setUp. platform's suite already defines it,
// so this wrapper is that call: init NVS, clear the key, then platform's setUp.
extern "C" void __real_setUp(void);

extern "C" void __wrap_setUp(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, settings::initialize_nvs());
    settings::WifiCredentialsStore store;
    TEST_ASSERT_EQUAL(ESP_OK, store.clear());
    __real_setUp();
}

TEST_CASE("save and load return the same credential bytes", "[settings]")
{
    settings::WifiCredentialsStore store;

    settings::WifiCredentials full =
        make_credentials(settings::kMaxSsidBytes, settings::kMaxPassphraseBytes, 0x5A, 0xA5);
    full.ssid[0] = 0;
    full.ssid[16] = 0;
    TEST_ASSERT_EQUAL(ESP_OK, store.save(full));

    settings::WifiCredentials output = fake_credentials();
    TEST_ASSERT_EQUAL(ESP_OK, store.load(output));
    TEST_ASSERT_EQUAL_MEMORY(&full, &output, sizeof(full));

    settings::WifiCredentials with_zero = make_credentials(4, 8, 'a', 'p');
    with_zero.ssid[1] = 0;
    TEST_ASSERT_EQUAL(ESP_OK, store.save(with_zero));

    output = fake_credentials();
    TEST_ASSERT_EQUAL(ESP_OK, store.load(output));
    TEST_ASSERT_EQUAL_MEMORY(&with_zero, &output, sizeof(with_zero));
}

TEST_CASE("save and load keep an empty passphrase", "[settings]")
{
    settings::WifiCredentials saved = make_credentials(6, 0, 'n', 0);
    saved.passphrase.fill(0xEE);
    saved.passphrase_length = 0;
    const settings::WifiCredentials expected = make_credentials(6, 0, 'n', 0);

    settings::WifiCredentialsStore store;
    TEST_ASSERT_EQUAL(ESP_OK, store.save(saved));

    settings::WifiCredentials output = fake_credentials();
    TEST_ASSERT_EQUAL(ESP_OK, store.load(output));
    TEST_ASSERT_EQUAL_UINT8(0, output.passphrase_length);
    TEST_ASSERT_EQUAL_MEMORY(&expected, &output, sizeof(expected));
}

TEST_CASE("clear removes saved credentials", "[settings]")
{
    const settings::WifiCredentials saved = make_credentials(4, 4, 3, 4);
    settings::WifiCredentialsStore store;
    TEST_ASSERT_EQUAL(ESP_OK, store.save(saved));
    TEST_ASSERT_EQUAL(ESP_OK, store.clear());

    settings::WifiCredentials output = fake_credentials();
    const settings::WifiCredentials unchanged = output;
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, store.load(output));
    TEST_ASSERT_EQUAL_MEMORY(&unchanged, &output, sizeof(output));
}

TEST_CASE("load when nothing has been saved returns not found", "[settings]")
{
    // setUp's clear opens read-write and creates the namespace. Erase so this
    // load is a namespace that was never written.
    TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
    TEST_ASSERT_EQUAL(ESP_OK, settings::initialize_nvs());

    settings::WifiCredentialsStore store;
    assert_load_error(store, ESP_ERR_NVS_NOT_FOUND);
}

TEST_CASE("save rejects lengths outside the store limits", "[settings]")
{
    const settings::WifiCredentials saved = make_credentials(8, 12, 9, 10);
    settings::WifiCredentialsStore store;
    TEST_ASSERT_EQUAL(ESP_OK, store.save(saved));

    settings::WifiCredentials rejected = saved;
    rejected.ssid_length = 0;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, store.save(rejected));

    rejected = saved;
    rejected.ssid_length = static_cast<uint8_t>(settings::kMaxSsidBytes + 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, store.save(rejected));

    rejected = saved;
    rejected.passphrase_length = static_cast<uint8_t>(settings::kMaxPassphraseBytes + 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, store.save(rejected));

    settings::WifiCredentials output = fake_credentials();
    TEST_ASSERT_EQUAL(ESP_OK, store.load(output));
    TEST_ASSERT_EQUAL_MEMORY(&saved, &output, sizeof(saved));
}

TEST_CASE("a second save replaces the first", "[settings]")
{
    const settings::WifiCredentials first =
        make_credentials(settings::kMaxSsidBytes, settings::kMaxPassphraseBytes, 0x21, 0x41);
    const settings::WifiCredentials second = make_credentials(3, 1, 'a', 'Z');

    settings::WifiCredentialsStore store;
    TEST_ASSERT_EQUAL(ESP_OK, store.save(first));
    TEST_ASSERT_EQUAL(ESP_OK, store.save(second));

    settings::WifiCredentials output = fake_credentials();
    TEST_ASSERT_EQUAL(ESP_OK, store.load(output));
    TEST_ASSERT_EQUAL_MEMORY(&second, &output, sizeof(second));
}

TEST_CASE("clear on an empty store returns ok", "[settings]")
{
    settings::WifiCredentialsStore store;
    TEST_ASSERT_EQUAL(ESP_OK, store.clear());
    assert_load_error(store, ESP_ERR_NVS_NOT_FOUND);
}

TEST_CASE("load rejects a blob with a bad version or length", "[settings]")
{
    settings::WifiCredentialsStore store;

    const uint8_t version_two[] = {2, 1, 'z', 0};
    TEST_ASSERT_EQUAL(ESP_OK, store_credentials_blob(version_two, sizeof(version_two)));
    assert_load_error(store, ESP_ERR_INVALID_VERSION);

    const uint8_t length_mismatch[] = {settings::kCredentialsVersion, 1, 'z', 0, 0xEE};
    TEST_ASSERT_EQUAL(ESP_OK, store_credentials_blob(length_mismatch, sizeof(length_mismatch)));
    assert_load_error(store, ESP_ERR_INVALID_SIZE);

    const uint8_t empty_ssid[] = {settings::kCredentialsVersion, 0, 0};
    TEST_ASSERT_EQUAL(ESP_OK, store_credentials_blob(empty_ssid, sizeof(empty_ssid)));
    assert_load_error(store, ESP_ERR_INVALID_SIZE);

    constexpr std::size_t kSsidBytesPastLimit = settings::kMaxSsidBytes + 1;
    std::array<uint8_t, settings::kVersionFieldBytes + settings::kSsidLengthFieldBytes + kSsidBytesPastLimit
                            + settings::kPassphraseLengthFieldBytes>
        ssid_too_long{};
    ssid_too_long[0] = settings::kCredentialsVersion;
    ssid_too_long[1] = static_cast<uint8_t>(kSsidBytesPastLimit);
    TEST_ASSERT_EQUAL(ESP_OK, store_credentials_blob(ssid_too_long.data(), ssid_too_long.size()));
    assert_load_error(store, ESP_ERR_INVALID_SIZE);

    constexpr std::size_t kPassphraseBytesPastLimit = settings::kMaxPassphraseBytes + 1;
    std::array<uint8_t, settings::kVersionFieldBytes + settings::kSsidLengthFieldBytes + settings::kMinSsidBytes
                            + settings::kPassphraseLengthFieldBytes + kPassphraseBytesPastLimit>
        passphrase_too_long{};
    passphrase_too_long[0] = settings::kCredentialsVersion;
    passphrase_too_long[1] = static_cast<uint8_t>(settings::kMinSsidBytes);
    passphrase_too_long[2] = 'a';
    passphrase_too_long[settings::kVersionFieldBytes + settings::kSsidLengthFieldBytes + settings::kMinSsidBytes] =
        static_cast<uint8_t>(kPassphraseBytesPastLimit);
    TEST_ASSERT_EQUAL(ESP_OK, store_credentials_blob(passphrase_too_long.data(), passphrase_too_long.size()));
    assert_load_error(store, ESP_ERR_INVALID_SIZE);
}

TEST_CASE("initialize_nvs again does not erase saved credentials", "[settings]")
{
    TEST_ASSERT_EQUAL(ESP_OK, settings::initialize_nvs());

    const settings::WifiCredentials saved = make_credentials(8, 10, 1, 2);
    settings::WifiCredentialsStore store;
    TEST_ASSERT_EQUAL(ESP_OK, store.save(saved));
    TEST_ASSERT_EQUAL(ESP_OK, settings::initialize_nvs());

    settings::WifiCredentials output = fake_credentials();
    TEST_ASSERT_EQUAL(ESP_OK, store.load(output));
    TEST_ASSERT_EQUAL_MEMORY(&saved, &output, sizeof(saved));
}
