# Pixel 9 / Android 17: BLE HID-over-GATT peripheral is "connected" but delivers no input events

**Filed:** [issuetracker.google.com/issues/568711381](https://issuetracker.google.com/issues/568711381) on 2026-10-02 (Android Public Tracker > Framework). Google assigned it and asked for an Android bug report (status NeedsInfo).
**Update 2026-10-03:** the repro narrowed to the advertised GAP appearance; see [Update](#update-2026-10-03-narrowed-repro) below. The text under Summary is what was originally filed.

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

## Update 2026-10-03: narrowed repro

The failure depends on the advertised GAP appearance (AD type 0x19). Same
peripheral code, same descriptor, only this constant changed:

| Appearance | Pixel 9 (Android 17) | iPhone |
|---|---|---|
| `0x03C1` Keyboard | reports dropped (no input events) | works |
| `0x0180` Remote Control | works | reports ignored |
| `0x03C0` Generic HID | works | works |

So HID input reports for a device whose appearance is Keyboard are silently
dropped while Android reports the device as connected. Workaround on the
peripheral: advertise `0x03C0`. Still worth Google's attention: the device is
shown connected and bonded, and no error is surfaced.

### Draft comment for the issue (not posted; needs the owner's OK)

> Update: I narrowed this down. The reports are dropped only when the peripheral
> advertises the GAP appearance Keyboard (0x03C1). With an identical ESP32
> sketch (arduino-esp32 BLEHIDDevice, consumer-control-only report map, Just
> Works bonding) the Pixel 9 / Android 17 delivers the Play/Pause notifications
> when the appearance is Remote Control (0x0180) or Generic HID (0x03C0), and
> drops them with 0x03C1. The link is bonded, encrypted and subscribed in every
> case; `dumpsys bluetooth_manager` shows HOGP connected and `getevent` shows no
> events in the failing case. iPhone delivers with 0x03C1 and 0x03C0.
> I have a workaround (advertise 0x03C0), so this is no longer blocking me, but
> a connected, bonded HOGP device silently dropping input reports based on its
> appearance looks unintended, and it appears to be a change since ~June 2026.
> I can capture the bug report you asked for with the failing configuration
> (appearance 0x03C1) if that is still useful.
