<p align="right"><strong>English</strong> · <a href="blufi-provisioning.zh_CN.md">简体中文</a></p>

# BLUFI Wi-Fi Provisioning

This page documents the behavior implemented by `main/demo_blufi.c`. BLUFI is
demo-level application behavior, not a reusable BSP hardware API.

## Scope and security boundary

- The ESP32-S3 operates as a 2.4 GHz Wi-Fi station. The demo has no SoftAP,
  cloud protocol, account service, or OTA path.
- Station credentials arrive through BLUFI and are stored by the ESP-IDF Wi-Fi
  driver in NVS. Credentials are not embedded in firmware and password contents
  are never logged.
- The transport uses the ESP-IDF 5.5.3 example's DH negotiation, MD5-derived
  AES-CFB key, and CRC. BLE SMP and bonding are disabled, and this reference
  does not enable NVS encryption or establish a production device identity.
- The local GATT MTU is capped at 255 bytes because BLUFI carries payload length
  in one byte. This avoids wrapped frame lengths from incompatible clients.

Treat this as an interoperable hardware-reference flow, not a complete
production provisioning security design.

## Lifecycle

| Event | Wi-Fi behavior | BLUFI behavior |
| --- | --- | --- |
| Boot with a saved SSID | Connect automatically. | Provisioning remains idle and does not start advertising. |
| Boot without a saved SSID | Keep station mode ready for scanning. | Advertise as `SunflowerBuddy-XXXX`. |
| Client connects | Wi-Fi state is unchanged. | Stop advertising and create a fresh security context. |
| SSID/password/connect received | Persist the station configuration and connect. | Report connecting, failure reason, or success to the connected client. |
| DHCP address received | Mark provisioning complete and reset reconnect backoff. | Stop advertising if no client is connected. An existing GATT connection is currently retained. |
| Client disconnects after success | Keep Wi-Fi connected. | Do not advertise again because provisioning is complete. |
| Wi-Fi disconnects later | Retry indefinitely after 1, 2, 4, 8, 16, then at most 30 seconds. | Do not reopen provisioning automatically. |
| Wi-Fi button held for one second | Disconnect and clear only the station configuration; keep/restart Wi-Fi for scanning. | Disconnect an active client to reset BLUFI packet sequence and security state, then advertise again. |

There is currently no post-success BLE timeout and no call to disable or
deinitialize Bluedroid or the BLE controller. If the phone keeps its GATT
connection open, that connection remains until the phone disconnects or the
Wi-Fi button is held. “Not advertising,” “no active GATT connection,” and “BLE
controller powered down” are therefore three distinct states.

## Mobile-app flow

1. Scan for and connect to `SunflowerBuddy-XXXX`.
2. Complete BLUFI negotiation.
3. Ask the device to scan and select a 2.4 GHz access point.
4. Send SSID, password, and the connect request in that order.
5. Wait for the device's Wi-Fi success report or inspect the serial disconnect
   reason.

After holding the Wi-Fi button, reconnect the app even if its UI still shows the
old peripheral. The firmware deliberately closes the old GATT connection so the
next attempt starts with packet sequence zero and performs new key negotiation.

## Expected serial log

A successful first-time provisioning attempt includes:

```text
BLUFI client connected
BLUFI received station SSID (... bytes)
BLUFI received station password (... bytes)
BLUFI credentials saved; connecting to Wi-Fi
Wi-Fi connection attempt started
Wi-Fi connected, IP=...
```

The log prints credential lengths, never password contents. Common Wi-Fi
disconnect reasons include:

| Reason | Meaning | First checks |
| --- | --- | --- |
| `201` | Access point not found | Confirm 2.4 GHz is enabled, SSID visibility, channel, and signal. |
| `202` | Authentication failed | Re-enter the password and confirm AP security mode. |
| `203` | Association failed | Check AP capacity, filtering, and compatibility. |
| `204` or `15` | Handshake timeout | Check password/security compatibility and RF quality. |
| `210` | No AP with compatible security | Use a security mode supported by ESP32-S3 station mode. |
| `212` | AP below RSSI threshold | Move closer or improve the 2.4 GHz signal. |

## Diagnostics

| Log or symptom | Interpretation and action |
| --- | --- |
| `Password length matches WPA2 standards...` | ESP-IDF automatically raised the authentication threshold after receiving a WPA2-length password; it is not a password failure. |
| `BTM_BleWriteAdvData, Partial data write into ADV` | The legacy advertising payload was truncated. It is non-fatal if the device remains discoverable and connectable. |
| `BT_HCI ... rsn 0x13` | The remote phone terminated the BLE connection normally. |
| `Invalid data length: 268, expected: 12` | Usually an old firmware/client MTU incompatibility. Confirm the firmware contains the 255-byte local MTU cap and reconnect. |
| `seq 0 is not expect N` | The app reused an old GATT session while restarting its BLUFI sequence. Hold reset, wait for disconnect, then reconnect before provisioning. |
| `ESP_ERR_WIFI_NOT_STARTED` during scan | The Wi-Fi station is not running. Current reset logic clears only station config and restarts Wi-Fi; confirm the current firmware was flashed. |
| Correct password but no connection attempt | Confirm the log reaches `credentials saved` and `connection attempt started`; older firmware could wait forever for a disconnect event while already idle. |
| Device is still shown in the app after success | The current firmware retains an already-connected GATT client and has no auto-close timer. Disconnect in the app or hold Wi-Fi to reset. |

## Device acceptance

Test with the firmware commit and mobile-app version recorded:

- provision with cleared station credentials, obtain an IP address, and confirm
  the password is not present in logs;
- reboot and confirm saved-configuration auto-connect;
- interrupt the AP and confirm exponential reconnect and backoff reset after
  DHCP succeeds;
- hold Wi-Fi while disconnected and while a BLUFI client is connected, then
  confirm credentials are cleared, the old GATT link closes, scanning works,
  and a new session can provision successfully;
- repeat reset and provisioning at least three times without frame-length,
  sequence, or `WIFI_NOT_STARTED` errors;
- separately record whether the mobile app closes the successful GATT session,
  because the firmware currently does not enforce a timeout.
