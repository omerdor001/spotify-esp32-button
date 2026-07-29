# Pixel / Android BLE HID Investigation — Summary & Resume Notes

**Status as of 2026-07-29:** Unresolved. Confirmed to be an Android/Pixel-side
Bluetooth stack issue, not a firmware bug. Paused pending access to a
non-Pixel Android device for further isolation.

## Symptom

- Device pairs successfully with Android (Pixel 9, Pixel 9a) and shows as a
  connected Bluetooth input device.
- **No button presses have any effect** on the Pixel — neither media keys
  (Play/Pause, Next, Prev, Stop) nor plain keyboard keystrokes.
- Works perfectly end-to-end on iPhone (tested on two different iPhones).
- Was previously working on this same Pixel 9 approximately one month ago
  (around late June 2026); stopped working since, cause unknown.

## What we ruled out

| Hypothesis | Status | Evidence |
|---|---|---|
| Stale Bluetooth bond / cache on phone | Ruled out | Reproduces on brand-new pairings on two different Pixel models |
| Google account cross-device BLE metadata sync | Ruled out | Pixel 9 and Pixel 9a are on **different** Google accounts, both fail identically |
| Uncommitted device rename (`"Spotify Button"` → `"Noam's DJ"`) | Ruled out | Rename was never flashed to the device before the bug was reproduced |
| Firmware/code regression | Ruled out | No firmware logic changes since commit `65099b4` (2026-06-28, ~1 month old — matches "worked a month ago" timing). Working tree diff confirmed no unintended changes at start of investigation. |
| Vendored library drift (NimBLE-Arduino / HijelHID_BLEKeyboard) | Ruled out | Both are local git clones pinned to fixed upstream commits since 2026-06-28 checkout (NimBLE-Arduino @ commit from 2026-06-17, HijelHID_BLEKeyboard @ commit from 2026-04-09). No local edits except temporary diagnostic changes made and reverted during this investigation. |
| Outdated Android/Pixel OS version | Ruled out | Applied pending system update on Pixel 9 — no change in behavior |
| Consumer/media report (Report ID 2) specifically broken, keyboard reports (Report ID 1) fine | Ruled out | Temporarily changed Stop button to send `KEY_A` (plain keyboard key, Report ID 1) instead of `MEDIA_STOP` — still completely non-functional on Pixel |
| Multi-Report-ID composite HID descriptor confusing Android's HOGP-to-uhid bridge | Ruled out | Temporarily stripped the HID report descriptor down to **keyboard-only, single Report ID** (removed Consumer Control collection entirely from `HijelHID_BLEKeyboard.cpp`, disabled the second Input Report characteristic) — still zero events reached the kernel, even with the simplest possible single-report descriptor |

## What we confirmed via direct Android diagnostics

Used `adb` (installed via `brew install android-platform-tools`) connected to
the Pixel 9 via USB. Key commands used:

```bash
adb shell dumpsys bluetooth_manager   # BT/GATT/HID stack state dump
adb shell getevent -lt                # raw kernel input event stream
```

Findings from `dumpsys bluetooth_manager`:
- LE link fully bonded and **encrypted** (`le_encrypted:T`, 16-byte key)
- GATT channel state: `GATT_CH_OPEN`
- Android's native HID Host stack (`BTA_HH`) reports the device as
  **`BTHH_CONN_STATE_CONNECTED`** / **`BTA_HH_CONN_ST`**, with a valid open
  file descriptor to the kernel HID layer (`fd:380`)
- `HidHostService` shows `HOGP connection state=2` (connected)

Findings from `getevent -lt` (run live while pressing physical buttons):
- Android's kernel **does** create a real evdev input device for the
  peripheral (seen as `/dev/input/eventN`, named `"Spotify Button Keyboard"`
  in the two-report build, `"Spotify Button"` in the single-report test build)
- **Zero key/consumer events ever appeared in the kernel event stream**,
  across multiple repeated button presses, in both the full firmware build
  and the stripped-down single-report test build

**Conclusion:** Every layer up through "Android recognizes and connects to
the HID device" succeeds. The failure is specifically in delivering BLE HID
Input Report notifications into an actual kernel input event — i.e. Android's
native `bta_hh_le` (HOGP-to-uhid bridge) is not converting received/expected
notifications into real key events, despite the connection otherwise
appearing fully healthy. This looks like a genuine Android/AOSP Bluetooth
stack regression rather than anything fixable in this project's firmware.

## HID descriptor / library internals reviewed (for reference)

- Report descriptor defined in
  `libraries/HijelHID_BLEKeyboard/src/HijelHID_BLEKeyboard.cpp`
  (`_hidReportDescriptor`, two collections: Report ID 1 = keyboard 6KRO,
  Report ID 2 = Consumer Control 16-bit array usage)
- HID GATT service setup in
  `libraries/NimBLE-Arduino/src/NimBLEHIDDevice.cpp` — confirmed a complete,
  spec-compliant HOGP implementation (Protocol Mode, HID Control Point, HID
  Info, Report Map, PnP ID, Report Reference descriptors all present and
  correctly structured). Nothing unusual found here.
- The `HijelHID_BLEKeyboard` library's own bundled test report
  (`test_data/Samsung_A07_Android16.txt`) shows media keys **passing** on a
  Samsung Android 16 device with this exact library/descriptor — reinforcing
  that this is likely a Pixel/AOSP-stack-specific issue rather than a
  universal Android bug or a library defect.

## Next step (in progress, paused)

Test on a **non-Pixel Android device** (e.g. Samsung) with the current
production firmware, to determine whether this is:
- **Pixel/AOSP-Bluetooth-stack-specific** (would explain why Samsung's own
  test data shows it working, since Samsung uses its own BT stack fork), or
- A broader Android issue that happens to also affect the specific
  Android/security-patch level running on these two Pixels

This is the cheapest remaining diagnostic and should be done before
considering firmware-side workarounds.

## Options considered for after non-Pixel test

1. **If non-Pixel Android works:** File a detailed bug report against
   Android/Pixel (Google Issue Tracker) using the evidence gathered above
   (dumpsys + getevent logs demonstrate a clear, reproducible bug with a
   spec-compliant HOGP peripheral). Continue using the device normally on
   iOS/other Android in the meantime.
2. **If non-Pixel Android also fails:** Points to something broader in
   current Android BLE HID handling, or possibly specific to this exact
   Android version/security patch level shared across test devices. Would
   need further scoping.
3. **Considered but not started: switch ESP32 to Classic Bluetooth (BR/EDR)
   HID instead of BLE HID.** This would use a completely different Android
   Bluetooth code path (`bta_hh` BR/EDR vs the LE/GATT bridge implicated
   above) and might sidestep the bug entirely. However:
   - Requires switching from NimBLE-Arduino to the Bluedroid stack (NimBLE is
     BLE-only) — a full rewrite of the Bluetooth layer, not a config change.
   - `HijelHID_BLEKeyboard` (BLE-only) would need to be replaced with a
     Bluedroid-based HID Device API implementation.
   - **Battery life tradeoff:** BLE's connection-interval/slave-latency model
     lets a peripheral sleep aggressively between connection events — the
     current library already exploits this (idle timer drops duty cycle from
     ~133 to ~1.6 connection events/sec after 5s of inactivity). Classic BT's
     master-polls-slave (TDD) model has no equivalent even in sniff mode, and
     Bluedroid is heavier and less sleep-friendly on ESP32 than NimBLE. This
     option would likely noticeably reduce battery life for what is
     essentially an idle-most-of-the-time button device.
   - Treat as a last resort if the Pixel is a hard requirement and Google
     doesn't fix the underlying bug in a reasonable timeframe.

## Diagnostic tooling set up (for next session)

- `adb` installed via Homebrew (`brew install android-platform-tools`) on
  this Mac — already available for future sessions, no need to reinstall.
- Pixel 9 has Developer Options + USB debugging enabled, and "Enable
  Bluetooth HCI snoop log" toggled on (found under Settings → Connected
  devices → Bluetooth → Debugging, not the classic global Developer Options
  toggle on this Android version).
- Note: `adb bugreport` / manual `adb pull` of the bugreport zip was
  unreliable in this environment (USB transport kept dropping mid-transfer,
  unrelated to cable/device — likely an artifact of the sandboxed shell
  environment). The faster, reliable path for future diagnostics is small
  targeted `adb shell` commands (`dumpsys bluetooth_manager`, `getevent -lt`)
  rather than large file pulls.
- If a full HCI snoop log is needed later, try pulling it from a stable
  non-sandboxed terminal, or use the Pixel's built-in Bluetooth diagnostics
  share feature if it becomes available in Settings on this Android version.

## Firmware/library changes made during investigation

All temporary — confirmed reverted / to be reverted immediately after this
summary was written:

1. `SpotifyConnection-bluetooth.ino` — Stop button temporarily changed to
   send `KEY_A` instead of `MEDIA_STOP` (for Report ID 1 vs 2 isolation test).
   **Reverted.**
2. `libraries/HijelHID_BLEKeyboard/src/HijelHID_BLEKeyboard.cpp` — Consumer
   Control collection temporarily removed from `_hidReportDescriptor`, and
   `_pConsumerInput` characteristic creation temporarily disabled (for
   single-report-ID isolation test). **Reverted** (confirmed via `git diff`
   showing clean working tree in this vendored library).
3. Device also had bonds cleared once earlier in the process (via a
   temporary `bleKeyboard.clearBonds()` call in `setup()`, flashed once then
   removed) as part of an earlier, since-superseded hypothesis about stale
   ESP32-side bonds.

The device is currently flashed with the single-report-only **test** build
(from the last diagnostic step). The `.ino` will be reverted and **should be
reflashed with the normal production firmware** before resuming normal use —
see next steps below.
