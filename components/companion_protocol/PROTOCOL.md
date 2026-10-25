# Companion protocol

Wire contract between the pixel clock and the iOS companion app. Version 1.

The device is the Bluetooth peripheral. The companion is the central. One connection is active at a time. This document describes the bytes on that connection. The C++ codec in this directory encodes and decodes those bytes. It does not open the radio, store credentials, or decide when to reply.

## Transport

| Item | Value |
| --- | --- |
| GATT service UUID | **TBD (stage 4)** |
| Inbound characteristic (companion writes a request) | **TBD (stage 4)** |
| Outbound characteristic (device notifies a reply) | **TBD (stage 4)** |
| MTU chunking and reassembly | **TBD (stage 5)** |

A message may be larger than a single ATT payload. How messages are split and reassembled is **TBD (stage 5)**. The layout rules below describe a whole message, after reassembly.

## Message layout

Multi-byte integers are little-endian. A message is a 7-byte header followed by the body. The largest message is 128 bytes (`kMaxMessageBytes`).

| Offset | Type | Field | Rule |
| --- | --- | --- | --- |
| 0 | `u8` | `version` | `1` in this revision |
| 1 | `u8` | `message_type` | see Messages |
| 2 | `u8` | `flags` | `0`. No flags are defined. Any other value is rejected |
| 3 | `u16` | `request_id` | chosen by the companion, echoed on the reply. `0` is reserved |
| 5 | `u16` | `body_length` | number of bytes that follow the header |
| 7 | bytes | `body` | exactly `body_length` bytes |

`body_length` must equal the number of bytes received after the header. A message shorter than 7 bytes, or longer than 128 bytes, is rejected.

## Request ids

The companion picks a non-zero `request_id` for each request. The device copies that id into the reply header. `0` is reserved: the device uses it only when it could not read the request id (the header itself was too short to parse).

A reply header always carries:

- `version` = the protocol version the device speaks (`1`)
- `flags` = `0`
- `request_id` = the request's id, or `0` when that id could not be read

## Messages

The high bit separates requests (`0x01`–`0x7F`) from responses (`0x80`–`0xFF`).

### GetDeviceInfo (`0x01`)

Request. The body is empty.

Reply: `DeviceInfo`.

### SetWifiCredentials (`0x02`)

Request. The body is:

| Offset in body | Type | Field | Rule |
| --- | --- | --- | --- |
| 0 | `u8` | `ssid_length` | `1`..`32` |
| 1 | bytes | `ssid` | `ssid_length` bytes. Raw bytes, not necessarily UTF-8 |
| 1 + `ssid_length` | `u8` | `passphrase_length` | `0`..`64` |
| 2 + `ssid_length` | bytes | `passphrase` | `passphrase_length` bytes |

The body length is exactly `2 + ssid_length + passphrase_length`. The largest body is 98 bytes, so the largest message is 7 + 98 = 105 bytes.

Reply: `Ack`.

The codec checks only those lengths. It does not check whether a passphrase is valid for WPA. A passphrase must be one of:

- empty, for an open network
- 8 to 63 printable ASCII characters
- 64 hexadecimal digits

A passphrase outside those rules is answered with `Error` / `InvalidField`. That check is made by the session handler, not by this codec.

### Ack (`0x80`)

Response. The body is one byte:

| Offset in body | Type | Field |
| --- | --- | --- |
| 0 | `u8` | `status` |

`status` uses the status table below. `Ok` (`0x00`) means the request was applied. `StorageFailed` (`0x20`) is a handler outcome, not a layout error.

### DeviceInfo (`0x81`)

Response to `GetDeviceInfo`. The body is:

| Offset in body | Type | Field | Rule |
| --- | --- | --- | --- |
| 0 | `u8` | `protocol_version` | version the device speaks (`1`) |
| 1 | `u8` | `firmware_version_length` | `0`..`32` |
| 2 | bytes | `firmware_version` | `firmware_version_length` UTF-8 bytes |

The 32-byte cap matches `esp_app_desc_t::version`. The body length is exactly `2 + firmware_version_length`.

### Error (`0xFF`)

Response when the device rejects a request before the handler runs, and when a handler rejects a field. The body is:

| Offset in body | Type | Field |
| --- | --- | --- |
| 0 | `u8` | `code` |

`code` uses the status table. The reply header echoes `request_id`. When the header could not be read, `request_id` is `0`.

## Status codes

One table covers decoder results, `Ack.status`, and `Error.code`.

| Code | Name | Meaning |
| --- | --- | --- |
| `0x00` | `Ok` | Success. On `Ack`, the request was applied |
| `0x01` | `MessageTooShort` | Fewer than 7 header bytes |
| `0x02` | `MessageTooLong` | More than 128 bytes |
| `0x03` | `UnsupportedVersion` | `version` is not `1` |
| `0x04` | `UnsupportedFlags` | `flags` is not `0` |
| `0x05` | `BodyLengthMismatch` | `body_length` is not the number of bytes that follow the header |
| `0x06` | `UnknownMessageType` | `message_type` is not one of the types in this document |
| `0x07` | `MalformedBody` | The body size or structure does not match its message type |
| `0x08` | `InvalidField` | A field is outside its range, or a passphrase fails the WPA rule above |
| `0x20` | `StorageFailed` | The handler could not persist the request. Carried on `Ack` |

Layout and validation failures use `Error`. A `SetWifiCredentials` request that was understood uses `Ack`, including `StorageFailed`.

## Version handling

The device speaks protocol version `1` and writes that value in every header it sends. `DeviceInfo.protocol_version` is the same number.

If a request's `version` is not `1`, the device replies with `Error` / `UnsupportedVersion`. That reply's header `version` is the version the device speaks (`1`), not the version from the request. The companion uses that byte to see which protocol the device implements.

`flags` must be `0`. A non-zero flags byte is `Error` / `UnsupportedFlags`. No flag bits are assigned in version 1.

## Examples

Bytes are hexadecimal. Multi-byte fields are little-endian.

### GetDeviceInfo, `request_id` `0x0001`

```text
01 01 00 01 00 00 00
```

| Bytes | Field |
| --- | --- |
| `01` | version 1 |
| `01` | GetDeviceInfo |
| `00` | flags |
| `01 00` | request_id `0x0001` |
| `00 00` | body_length 0 |

### SetWifiCredentials, `request_id` `0x1234`, SSID `home`, passphrase `password`

```text
01 02 00 34 12 0E 00 04 68 6F 6D 65 08 70 61 73 73 77 6F 72 64
```

| Bytes | Field |
| --- | --- |
| `01` | version 1 |
| `02` | SetWifiCredentials |
| `00` | flags |
| `34 12` | request_id `0x1234` |
| `0E 00` | body_length 14 |
| `04` | ssid_length 4 |
| `68 6F 6D 65` | ssid `home` |
| `08` | passphrase_length 8 |
| `70 61 73 73 77 6F 72 64` | passphrase `password` |

### Ack `Ok`, `request_id` `0x1234`

```text
01 80 00 34 12 01 00 00
```

| Bytes | Field |
| --- | --- |
| `01` | version 1 |
| `80` | Ack |
| `00` | flags |
| `34 12` | request_id `0x1234` |
| `01 00` | body_length 1 |
| `00` | status `Ok` |

### DeviceInfo, `request_id` `0x0001`, firmware version `1.0.0`

```text
01 81 00 01 00 07 00 01 05 31 2E 30 2E 30
```

| Bytes | Field |
| --- | --- |
| `01` | version 1 |
| `81` | DeviceInfo |
| `00` | flags |
| `01 00` | request_id `0x0001` |
| `07 00` | body_length 7 |
| `01` | protocol_version 1 |
| `05` | firmware_version_length 5 |
| `31 2E 30 2E 30` | firmware_version `1.0.0` |

### Error `InvalidField`, `request_id` `0`

```text
01 FF 00 00 00 01 00 08
```

| Bytes | Field |
| --- | --- |
| `01` | version 1 (the version the device speaks) |
| `FF` | Error |
| `00` | flags |
| `00 00` | request_id `0` (the request id was not available) |
| `01 00` | body_length 1 |
| `08` | code `InvalidField` |
