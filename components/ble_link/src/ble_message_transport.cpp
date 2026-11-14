#include "ble_link/ble_message_transport.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "companion_gatt_service_definitions.hpp"
#include "ble_link/companion_gatt_ids.hpp"
#include "ble_link/inbound_message_pool.hpp"
#include "ble_link/legacy_advertising_packets_builder.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "platform/runtime.hpp"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// Defined by NimBLE's store/config; no public header declares it.
extern "C" void ble_store_config_init(void);

namespace ble_link {
namespace {

constexpr char kTag[] = "ble_link";

// The callbacks below run one at a time on the NimBLE host task.
struct HostLinkState {
    LegacyAdvertisingPackets packets{};
    uint8_t own_address_type{0};
    // Stage 5's send reads this from the control task.
    std::atomic<uint16_t> connection_handle{BLE_HS_CONN_HANDLE_NONE};
};

HostLinkState host_link;
bool start_attempted = false;

void post_link_event(const platform::Event& event)
{
    if (!platform::Runtime::instance().post_event(event)) {
        ESP_LOGW(kTag, "mailbox full; dropped link event type=%u", static_cast<unsigned>(event.type));
    }
}

void end_link(uint16_t connection_handle, uint8_t hci_reason)
{
    const int rc = ble_gap_terminate(connection_handle, hci_reason);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_terminate failed on connection %u: rc=%d",
                 static_cast<unsigned>(connection_handle), rc);
    }
}

int on_gap_event(ble_gap_event* event, void* unused);

void advertise()
{
    int rc = ble_gap_adv_set_data(host_link.packets.advertising.data(),
                                  static_cast<int>(host_link.packets.advertising_length));
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_adv_set_data failed: rc=%d", rc);
        return;
    }

    rc = ble_gap_adv_rsp_set_data(host_link.packets.scan_response.data(),
                                  static_cast<int>(host_link.packets.scan_response_length));
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_adv_rsp_set_data failed: rc=%d", rc);
        return;
    }

    ble_gap_adv_params advertising_parameters{};
    advertising_parameters.conn_mode = BLE_GAP_CONN_MODE_UND;
    advertising_parameters.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(host_link.own_address_type, nullptr, BLE_HS_FOREVER, &advertising_parameters,
                           on_gap_event, nullptr);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_adv_start failed: rc=%d", rc);
        return;
    }
    ESP_LOGI(kTag, "advertising as \"%s\"", kDeviceName);
}

void handle_connect(int status, uint16_t connection_handle)
{
    if (status != 0) {
        ESP_LOGW(kTag, "connect failed: status=%d", status);
        advertise();
        return;
    }

    if (host_link.connection_handle.load() != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGW(kTag, "a companion is already connected; ending connection %u",
                 static_cast<unsigned>(connection_handle));
        end_link(connection_handle, BLE_ERR_CONN_LIMIT);
        return;
    }

    host_link.connection_handle.store(connection_handle);
    ESP_LOGI(kTag, "companion connected on connection %u", static_cast<unsigned>(connection_handle));
    post_link_event(companion_connected_event());

    const int rc = ble_gap_security_initiate(connection_handle);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_security_initiate failed: rc=%d", rc);
        end_link(connection_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
}

void handle_disconnect(int reason, uint16_t connection_handle)
{
    uint16_t expected_handle = connection_handle;
    if (host_link.connection_handle.compare_exchange_strong(expected_handle, BLE_HS_CONN_HANDLE_NONE)) {
        ESP_LOGI(kTag, "companion disconnected: reason=0x%x", static_cast<unsigned>(reason));
        post_link_event(companion_disconnected_event());
    }
    advertise();
}

void handle_encryption_change(int status, uint16_t connection_handle)
{
    if (status == 0) {
        ESP_LOGI(kTag, "connection %u is encrypted", static_cast<unsigned>(connection_handle));
        return;
    }
    ESP_LOGW(kTag, "encryption failed on connection %u: status=%d", static_cast<unsigned>(connection_handle),
             status);
    end_link(connection_handle, BLE_ERR_REM_USER_CONN_TERM);
}

int handle_repeat_pairing(uint16_t connection_handle)
{
    ble_gap_conn_desc connection{};
    const int rc = ble_gap_conn_find(connection_handle, &connection);
    if (rc == 0) {
        ESP_LOGI(kTag, "companion forgot its bond; deleting the clock's copy");
        (void)ble_store_util_delete_peer(&connection.peer_id_addr);
    } else {
        ESP_LOGE(kTag, "ble_gap_conn_find failed on repeat pairing: rc=%d", rc);
    }
    return BLE_GAP_REPEAT_PAIRING_RETRY;
}

int on_gap_event(ble_gap_event* event, void* unused)
{
    (void)unused;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        handle_connect(event->connect.status, event->connect.conn_handle);
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        handle_disconnect(event->disconnect.reason, event->disconnect.conn.conn_handle);
        return 0;
    case BLE_GAP_EVENT_ENC_CHANGE:
        handle_encryption_change(event->enc_change.status, event->enc_change.conn_handle);
        return 0;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        return handle_repeat_pairing(event->repeat_pairing.conn_handle);
    case BLE_GAP_EVENT_ADV_COMPLETE:
        advertise();
        return 0;
    default:
        return 0;
    }
}

void on_sync()
{
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_hs_util_ensure_addr failed: rc=%d", rc);
        return;
    }

    rc = ble_hs_id_infer_auto(0, &host_link.own_address_type);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_hs_id_infer_auto failed: rc=%d", rc);
        return;
    }
    advertise();
}

void on_reset(int reason)
{
    ESP_LOGW(kTag, "NimBLE host reset: reason=%d", reason);
}

// nimble_port_run returns only after nimble_port_stop, which this firmware never calls.
void host_task_entry(void* unused)
{
    (void)unused;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

}  // namespace

int refuse_characteristic_access(uint16_t connection_handle,
                                 uint16_t attribute_handle,
                                 ble_gatt_access_ctxt* context,
                                 void* unused)
{
    (void)attribute_handle;
    (void)unused;
    if (context->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        ESP_LOGW(kTag, "refused a %u-byte companion write on connection %u",
                 static_cast<unsigned>(OS_MBUF_PKTLEN(context->om)), static_cast<unsigned>(connection_handle));
    }
    return BLE_ATT_ERR_UNLIKELY;
}

bool BleMessageTransport::start()
{
    if (start_attempted) {
        ESP_LOGW(kTag, "start already called");
        return false;
    }
    start_attempted = true;

    if (!build_legacy_advertising_packets(kCompanionServiceUuid, kDeviceName, host_link.packets)) {
        ESP_LOGE(kTag, "advertising packets do not fit");
        return false;
    }

    const esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "nimble_port_init failed: %s", esp_err_to_name(err));
        return false;
    }

    // Just Works: no keyboard or display, so no man-in-the-middle check.
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_sc_only = 0;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    // The first sync can arrive as soon as the host task starts.
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    // store_status_cb stays unset so a full bond store refuses a new bond
    // instead of deleting the existing one.

    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(kCompanionServices);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gatts_count_cfg failed: rc=%d", rc);
        (void)nimble_port_deinit();
        return false;
    }

    rc = ble_gatts_add_svcs(kCompanionServices);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gatts_add_svcs failed: rc=%d", rc);
        (void)nimble_port_deinit();
        return false;
    }

    rc = ble_svc_gap_device_name_set(kDeviceName);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_svc_gap_device_name_set failed: rc=%d", rc);
        (void)nimble_port_deinit();
        return false;
    }

    ble_store_config_init();
    nimble_port_freertos_init(host_task_entry);
    return true;
}

bool BleMessageTransport::send(std::span<const uint8_t> message)
{
    (void)message;
    return false;
}

}  // namespace ble_link
