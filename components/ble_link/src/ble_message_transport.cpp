#include "ble_link/ble_message_transport.hpp"

#include <array>
#include <cstdint>

#include "gap_advertise.hpp"
#include "companion_gatt_service_definitions.hpp"
#include "host_link_state.hpp"

#include "ble_link/companion_gatt_ids.hpp"
#include "ble_link/legacy_advertising_packets_builder.hpp"
#include "ble_link/outbound_message_splitter.hpp"
#include "companion_protocol/message_header.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "host/ble_att.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// Defined by NimBLE's store/config; no public header declares it.
extern "C" void ble_store_config_init(void);

namespace ble_link {
namespace {

constexpr char kTag[] = "ble_link";
bool start_attempted = false;

// Runs once, when Bluetooth has finished starting and is ready: the clock
// reads its address and starts advertising. A connection does not call this.
void on_sync()
{
    // Make sure the clock has a Bluetooth address. 0 uses the factory address.
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_hs_util_ensure_addr failed: rc=%d", rc);
        return;
    }

    // Remember that address for advertising. 0 shows it to phones as-is.
    rc = ble_hs_id_infer_auto(0, &host_link.own_address_type);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_hs_id_infer_auto failed: rc=%d", rc);
        return;
    }
    gap_advertise();
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

HostLinkState host_link;

BleMessageTransport::BleMessageTransport(InboundMessagePool& inbound_messages)
    : inbound_messages_(inbound_messages)
{
    host_link.inbound_messages = &inbound_messages_;
}

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

    // Standard spot where a phone reads the device name.
    ble_svc_gap_init();
    // Standard spot where a phone reads the list of services.
    ble_svc_gatt_init();

    // Reserve room for the companion service before adding it.
    int rc = ble_gatts_count_cfg(kCompanionServices);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gatts_count_cfg failed: rc=%d", rc);
        (void)nimble_port_deinit();
        return false;
    }

    // Publish the companion service. A phone reads it after connecting.
    rc = ble_gatts_add_svcs(kCompanionServices);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_gatts_add_svcs failed: rc=%d", rc);
        (void)nimble_port_deinit();
        return false;
    }

    // The name a phone shows for this clock.
    rc = ble_svc_gap_device_name_set(kDeviceName);
    if (rc != 0) {
        ESP_LOGE(kTag, "ble_svc_gap_device_name_set failed: rc=%d", rc);
        (void)nimble_port_deinit();
        return false;
    }

    // Keep pairing keys in memory until power-off.
    ble_store_config_init();
    nimble_port_freertos_init(host_task_entry);
    return true;
}

bool BleMessageTransport::send(std::span<const uint8_t> message)
{
    if (message.empty() || message.size() > companion_protocol::kMaxMessageBytes) {
        return false;
    }

    const uint16_t handle = host_link.connection_handle.load();
    if (handle == BLE_HS_CONN_HANDLE_NONE || !host_link.notifications_enabled.load()
        || host_link.outbound_value_handle == 0) {
        return false;
    }

    const uint16_t att_mtu = ble_att_mtu(handle);
    std::array<std::size_t, kMaxNotifyPieces> piece_lengths{};
    std::size_t piece_count = 0;
    if (!message_split_piece_lengths(message.size(), att_mtu, piece_lengths, piece_count)) {
        return false;
    }

    std::size_t offset = 0;
    for (std::size_t index = 0; index < piece_count; ++index) {
        const std::size_t piece_length = piece_lengths[index];
        os_mbuf* const outbound = ble_hs_mbuf_from_flat(message.data() + offset, static_cast<uint16_t>(piece_length));
        if (outbound == nullptr) {
            return false;
        }
        const int rc = ble_gatts_notify_custom(handle, host_link.outbound_value_handle, outbound);
        if (rc != 0) {
            ESP_LOGE(kTag, "ble_gatts_notify_custom failed: rc=%d", rc);
            return false;
        }
        offset += piece_length;
    }
    return true;
}

}  // namespace ble_link
