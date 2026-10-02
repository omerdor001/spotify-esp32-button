# Pixel 9 / Android 17: BLE HID-over-GATT peripheral is "connected" but delivers no input events

Draft for Google Issue Tracker (component: Android Public Tracker > Connectivity > Bluetooth).

## Summary

On a Pixel 9, a standards-compliant BLE HID-over-GATT (HOGP) peripheral pairs, bonds,
encrypts, and is shown as a connected input device, and the peripheral sends HID input
report notifications to a subscribed, secured link. The notifications never become input
events: `getevent` shows zero events from the device's evdev node. The same peripheral
works on iPhone. A second, completely independent peripheral implementation fails
identically on the Pixel. The same peripheral worked on this Pixel around late June 2026
with no firmware changes since, which points to a regression in the phone's Bluetooth stack
or an update to it.

## Environment

- Device: Pixel 9 (also reproduces on a Pixel 9a, different Google account)
- Android 17, security patch 2026-09-05
- Peripheral: ESP32 (WROOM), BLE only
  - Firmware A: NimBLE-Arduino + HijelHID_BLEKeyboard (keyboard report ID 1 + consumer control report ID 2)
  - Firmware B (control): arduino-esp32 core BLEHIDDevice (Bluedroid), consumer-control-only
    descriptor, single input report, Just Works pairing with bonding
- Control host: iPhone, same peripherals, works with both firmwares

## Steps to reproduce

1. Flash Firmware B (a ~70-line sketch based on the arduino-esp32 `Server_Gamepad` example with a
   consumer-control descriptor; sends usage 0x00CD, Play/Pause, as a notification every 6 s).
2. Pair from Pixel Settings > Connected devices. Device appears as a connected Input device.
3. Play media. Observe that Play/Pause is never delivered.
4. `adb shell getevent -lt` while the peripheral sends: no events from the device.

## Expected

Each notification on the HID Input Report characteristic produces an input event
(a media key press), as it does on iOS.

## Actual

- Android shows the device connected (`HOGP connection state=2`, `BTA_HH_CONN_ST`, `le_encrypted`, 16-byte key).
- An evdev device is created (`/dev/input/eventN`, name "<device> Keyboard") whose capabilities are
  listed, but no events ever arrive.
- Peripheral side: notifications are sent to a subscribed, secured connection (NimBLE reports a
  subscriber on the input report characteristic with `secured=1`).
- Not related to the Report ID 1/2 split or consumer reports: a keyboard-only, single-Report-ID
  descriptor also produced no events.
- Bond clearing on both sides and re-pairing on two phones and two Google accounts did not help.

## Evidence

- `adb shell getevent -lt` during repeated notifications: no output from the device's node.
- `adb shell dumpsys bluetooth_manager`: link encrypted, HID host connected, HOGP connection state=2.
- Peripheral serial logs show subscriptions on both input reports and each send succeeding.
- The device connected and read the battery service earlier in some sessions but never subscribed
  to the HID input reports (only battery); in those sessions the peripheral sees no HID subscription.

## Notes

- Regression: worked on this Pixel 9 around late June 2026; no firmware change since 2026-06-28.
- A Samsung A07 on Android 16 passes with Firmware A's library (per the library's test data).
- An HCI snoop log was not captured; I can attach one on request.
