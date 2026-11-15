#include "gap_advertise.hpp"

#include "gap_events.hpp"
#include "host_link_state.hpp"

#include "ble_link/companion_gatt_ids.hpp"
#include "esp_log.h"
#include "host/ble_hs.h"

namespace ble_link {
namespace {

constexpr char kTag[] = "ble_link";

}  // namespace

void gap_advertise()
{
    // The packet phones see while scanning: flags and the service UUID.
    int rc = ble_gap_adv_set_data(host_link.packets.advertising.data(),
                                  static_cast<int>(host_link.packets.advertising_length));
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_adv_set_data failed: rc=%d", rc);
        return;
    }

    // The extra packet a phone requests while scanning: the clock's name.
    rc = ble_gap_adv_rsp_set_data(host_link.packets.scan_response.data(),
                                  static_cast<int>(host_link.packets.scan_response_length));
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_adv_rsp_set_data failed: rc=%d", rc);
        return;
    }

    ble_gap_adv_params advertising_parameters{};
    advertising_parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    advertising_parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;
    // Broadcast so a phone can find the clock and connect. Keeps going until a phone connects.
    rc = ble_gap_adv_start(host_link.own_address_type, nullptr, BLE_HS_FOREVER, &advertising_parameters, on_gap_event,
                           nullptr);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_adv_start failed: rc=%d", rc);
        return;
    }
    ESP_LOGI(kTag, "advertising as \"%s\"", kDeviceName);
}

}  // namespace ble_link
