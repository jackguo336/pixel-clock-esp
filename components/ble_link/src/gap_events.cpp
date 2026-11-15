#include "gap_events.hpp"

#include "gap_advertise.hpp"
#include "host_link_state.hpp"

#include "ble_link/inbound_message_pool.hpp"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "platform/runtime.hpp"

namespace ble_link {
namespace {

constexpr char kTag[] = "ble_link";

void post_link_event(const platform::Event& event)
{
    if (!platform::Runtime::instance().post_event(event)) {
        ESP_LOGW(kTag, "mailbox full; dropped link event type=%u", static_cast<unsigned>(event.type));
    }
}

void end_link(uint16_t connection_handle, uint8_t hci_reason)
{
    // End the phone's connection to the pixel clock.
    const int rc = ble_gap_terminate(connection_handle, hci_reason);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gap_terminate failed on connection %u: rc=%d",
                 static_cast<unsigned>(connection_handle), rc);
    }
}

void handle_connect(int status, uint16_t connection_handle)
{
    if (status != 0) {
        ESP_LOGW(kTag, "connect failed: status=%d", status);
        gap_advertise();
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

    // Ask the phone to pair. It shows the system dialog, with no PIN.
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
    gap_advertise();
}

void handle_encryption_change(int status, uint16_t connection_handle)
{
    if (status == 0) {
        ESP_LOGI(kTag, "connection %u is encrypted", static_cast<unsigned>(connection_handle));
        return;
    }
    ESP_LOGW(kTag, "encryption failed on connection %u: status=%d", static_cast<unsigned>(connection_handle), status);
    end_link(connection_handle, BLE_ERR_REM_USER_CONN_TERM);
}

int handle_repeat_pairing(uint16_t connection_handle)
{
    ble_gap_conn_desc connection{};
    // Look up which phone is on this connection.
    const int rc = ble_gap_conn_find(connection_handle, &connection);
    if (rc == 0) {
        ESP_LOGI(kTag, "companion forgot its bond; deleting the clock's copy");
        // Delete the pixel clock's saved pairing keys for that phone.
        (void)ble_store_util_delete_peer(&connection.peer_id_addr);
    } else {
        ESP_LOGE(kTag, "ble_gap_conn_find failed on repeat pairing: rc=%d", rc);
    }
    return BLE_GAP_REPEAT_PAIRING_RETRY;
}

}  // namespace

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
        gap_advertise();
        return 0;
    default:
        return 0;
    }
}

}  // namespace ble_link
