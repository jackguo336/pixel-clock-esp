#include "companion_inbound_message_write.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include <inttypes.h>

#include "host_link_state.hpp"

#include "ble_link/buffered_message_assembler.hpp"
#include "companion_protocol/message_header.hpp"
#include "esp_log.h"
#include "host/ble_att.h"
#include "host/ble_hs_mbuf.h"
#include "platform/runtime.hpp"

namespace ble_link {
namespace {

constexpr char kTag[] = "companion_inbound_write";

/// Returns a Bluetooth error when this write must be refused, or 0 when it
/// can continue.
[[nodiscard]] int validate_inbound_write(uint16_t connection_handle,
                                         const ble_gatt_access_ctxt* context)
{
    if (context->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (connection_handle != host_link.connection_handle.load()) {
        ESP_LOGW(kTag, "refused a companion write on connection %u", static_cast<unsigned>(connection_handle));
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (context->offset != 0) {
        ESP_LOGW(kTag, "refused a companion write at offset %u", static_cast<unsigned>(context->offset));
        return BLE_ATT_ERR_INVALID_OFFSET;
    }
    if (context->om == nullptr) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    const uint16_t length = OS_MBUF_PKTLEN(context->om);
    if (length > companion_protocol::kMaxMessageBytes) {
        ESP_LOGW(kTag, "refused a %u-byte companion write on connection %u", static_cast<unsigned>(length),
                 static_cast<unsigned>(connection_handle));
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    return 0;
}

}  // namespace

int on_inbound_write(uint16_t connection_handle,
                     uint16_t attribute_handle,
                     ble_gatt_access_ctxt* context,
                     void* unused)
{
    (void)attribute_handle;
    (void)unused;
    const int error = validate_inbound_write(connection_handle, context);
    if (error != 0) {
        return error;
    }

    const uint16_t length = OS_MBUF_PKTLEN(context->om);

    InboundMessagePool* const inbound_messages = host_link.inbound_messages;
    if (inbound_messages == nullptr) {
        ESP_LOGE(kTag, "inbound pool is missing");
        return BLE_ATT_ERR_UNLIKELY;
    }

    std::array<uint8_t, BufferedMessageAssembler::kMessageBytes> copied{};
    if (length > 0) {
        uint16_t copied_length = 0;
        // Copy the phone's write into a buffer the pixel clock can keep.
        const int copy_rc = ble_hs_mbuf_to_flat(context->om, copied.data(), static_cast<uint16_t>(copied.size()),
                                                &copied_length);
        if (copy_rc != 0 || copied_length != length) {
            ESP_LOGE(kTag, "inbound write copy failed: rc=%d", copy_rc);
            return BLE_ATT_ERR_UNLIKELY;
        }
    }

    std::array<uint8_t, BufferedMessageAssembler::kMessageBytes> message{};
    std::size_t message_length = 0;
    const AppendResult result = host_link.assembler.append(std::span<const uint8_t>(copied.data(), length), message,
                                                           message_length);
    if (result == AppendResult::Rejected) {
        ESP_LOGW(kTag, "discarded a stored message after a %u-byte write; drop_count=%" PRIu32,
                 static_cast<unsigned>(length), host_link.assembler.drop_count());
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    if (result == AppendResult::NeedMore) {
        return 0;
    }

    const std::optional<uint8_t> slot =
        inbound_messages->try_insert(std::span<const uint8_t>(message.data(), message_length));
    if (!slot.has_value()) {
        ESP_LOGW(kTag, "inbound pool dropped a message of %u bytes; drop_count=%" PRIu32,
                 static_cast<unsigned>(message_length), inbound_messages->drop_count());
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    const InboundMessagePoolSlotRef message_ref{
        .slot_index = *slot,
        .message_length = static_cast<uint8_t>(message_length),
    };
    if (!platform::Runtime::instance().post_event(companion_message_received_event(message_ref))) {
        inbound_messages->free_slot(*slot);
        ESP_LOGW(kTag, "mailbox full; freed slot %u, message length %u", static_cast<unsigned>(*slot),
                 static_cast<unsigned>(message_length));
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    ESP_LOGI(kTag, "companion message length %u in slot %u", static_cast<unsigned>(message_length),
             static_cast<unsigned>(*slot));
    return 0;
}

}  // namespace ble_link
