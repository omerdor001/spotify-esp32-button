# Spotify Button Controller (ESP32)

Control Spotify with three physical buttons using an ESP32 over Bluetooth.

**Flow: Button → ESP32 (BLE) → Phone → Spotify**

The ESP32 pairs with your phone as a Bluetooth media controller (like a headset button). When you press a button, the ESP32 sends a media key to your phone, and your phone forwards it to Spotify — no WiFi or Spotify credentials needed on the ESP32.

![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue)
![Bluetooth](https://img.shields.io/badge/Bluetooth-BLE%20HID-blue)
![Spotify](https://img.shields.io/badge/Spotify-via%20Phone-green)
![Arduino](https://img.shields.io/badge/Arduino-CLI%20%2F%20IDE-teal)

---

## Features

- **Play/Next button** — press to play if stopped; press to skip to the next track if already playing
- **Prev button** — goes to the previous track. Players restart the current track if it has played for more than ~3 s, so the firmware sends one Prev if it changed track in the last 3 s, otherwise two (restart, then previous). It can't read playback position, so this is a best guess; see `prevRestartWindowMs`
- **Stop button** — stops playback and resets internal state
- **Next-press throttle** — Next is ignored if a Play or Next was sent in the last 5 seconds, preventing accidental double-skips
- **No WiFi required** — Bluetooth only; no network setup needed
- **No credentials** — no Spotify Client ID/Secret on the device
- **Native media keys** — phone handles Spotify routing automatically
- **Subscription watchdog** — if a phone connects but never enables HID notifications (so every press would be silently lost), the firmware disconnects so the phone retries, and logs dropped presses
- **ESP32 core BLE stack, no extra libraries** — a minimal media-key-only HID device (see [Design notes](#design-notes-read-before-changing-the-bluetooth-code))

---

## Hardware Requirements

| Part | Notes |
|---|---|
| ESP32 Development Board | NodeMCU-32S or any WROOM/WROVER variant |
| 3× Momentary push button | Tactile switch, arcade button, etc. |
| USB cable | For programming |
| 6 jumper wires | 2 per button |

---

## Wiring

```
ESP32           Play/Next button
GPIO 4  ──────── Pin 1
GND     ──────── Pin 2

ESP32           Prev button
GPIO 17 ──────── Pin 1
GND     ──────── Pin 2

ESP32           Stop button
GPIO 18 ──────── Pin 1
GND     ──────── Pin 2
```

`INPUT_PULLUP` is used — no external resistor needed. Use a **soldered or direct connection**; breadboards with poor contact will cause phantom keypresses (the floating pin picks up RF noise from the on-chip BLE radio).

---

## Software Requirements

### Option A — Arduino CLI (recommended)

No extra libraries are needed — the sketch only uses the ESP32 core's built-in BLE.

```bash
# Install ESP32 core (one-time)
arduino-cli core install esp32:esp32

# Compile and flash in one step (replace port as needed)
arduino-cli compile --upload \
  --fqbn esp32:esp32:nodemcu-32s \
  --port /dev/cu.usbserial-0001 \
  SpotifyConnection-bluetooth/
```

### Option B — Arduino IDE 2.x

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - `File → Preferences` → Additional Boards Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - `Tools → Board → Boards Manager` → search **ESP32** → Install


---

## Setup

### 1. Upload to ESP32

**Arduino CLI:**
```bash
arduino-cli compile --upload \
  --fqbn esp32:esp32:nodemcu-32s \
  --port /dev/cu.usbserial-0001 \
  SpotifyConnection-bluetooth/
```

**Arduino IDE:**
- `Tools → Board → NodeMCU-32S`
- `Tools → Port → [your port]`
- Click **Upload**

### 2. Pair with Your Phone

After uploading, open the Serial Monitor (115200 baud). You'll see:

```
Starting BLE HID remote...
Advertising as 'Noam DJ Buttons'. Pair from your phone/PC.
```

On your phone:
- **Android:** Settings → Bluetooth → pair **Noam DJ Buttons**
- **iOS:** Settings → Bluetooth → pair **Noam DJ Buttons**

If you re-flash the board or change anything about its Bluetooth identity, **Forget** the device on the phone and pair again. Phones cache pairing and GATT details.

### 3. Use It

Open Spotify on your phone, then use the buttons. The ESP32 sends media keys and Spotify responds immediately.

---

## Usage

| Button | Situation | Action |
|---|---|---|
| Play/Next (GPIO 4) | Spotify stopped | Plays |
| Play/Next (GPIO 4) | Spotify playing | Skips to next track |
| Play/Next (GPIO 4) | Within 5s of last Play/Next | Ignored (throttle) |
| Prev (GPIO 17) | Track changed < 3 s ago | One Prev: previous track |
| Prev (GPIO 17) | Otherwise | Two Prevs: restart track, then previous track |
| Prev (GPIO 17) | Within 5s of last Prev | Ignored (throttle) |
| Stop (GPIO 18) | Any | Stops playback |
| Any button | BLE not connected | Serial prints "not connected" |

**Requirement:** Spotify must be open on your phone. The button controls an active Spotify session.

### State sync note

The device tracks playback state locally based on what commands it has sent. If playback is changed from the phone (e.g. someone pauses Spotify manually), the device won't know. In that case, press **Stop** once to reset the device's state, then **Play/Next** to resume. This is an inherent limitation of BLE HID, which is a write-only channel.

**iOS caveat:** on iPhone, `MEDIA_STOP` is not honored by iOS/Spotify — pressing Stop never actually pauses the music, but it *always* resets the device's internal state to "not playing" anyway (the device has no way to know the Stop was ignored). This means on iOS, Stop should only be pressed when you already know playback needs re-syncing (e.g. you just paused manually from the phone) — **not** as a "just in case" tap while music is genuinely still playing. If you press Stop while Spotify is actually still playing, the device will (wrongly) believe it's stopped, and the next Play/Next press will send a Play/Pause toggle that **pauses** the music instead of doing nothing/resuming. This is the same write-only limitation described above, just encountered from the other direction: Stop is a "tell the device I believe playback is now stopped" signal, not a verified action, and on iOS that signal is never actually true.

---

## Troubleshooting

### Button not responding
- Confirm wiring: GPIO 4 → Play/Next, GPIO 17 → Prev, GPIO 18 → Stop (each button other leg → GND)
- Avoid breadboards — poor contact causes unreliable readings. Solder the button or use a direct wire for testing.
- Short the two button wires together to test without a button
- Open the Serial Monitor (115200 baud) and press the button — you should see a log line. If nothing appears, it's a wiring/pin issue, not a software one.

### Phantom keypresses / songs skipping without pressing
- This is RF noise from the BLE radio coupling into a floating or poorly-connected GPIO pin
- Ensure the button or wire makes solid contact with GND
- On iOS/macOS, two rapid Play/Pause keypresses = skip track (double-tap behavior) — so a noisy pin can skip songs instead of toggling

### Phone doesn't see "Noam DJ Buttons"
- Make sure Bluetooth is on
- Press the ESP32 reset button and try pairing again

### Paired but Play/Pause doesn't work
- **Android:** check that the BLE device has "Media" permissions in Bluetooth device settings
- **iOS:** make sure Spotify is in the foreground the first time you press

### Stop doesn't work on iPhone / pressing Stop then Play/Next pauses the music
- **Known iOS limitation:** `MEDIA_STOP` is not honored by iOS, so Stop never actually pauses playback on iPhone.
- Worse, pressing Stop still resets the device's internal "is playing" state regardless (see [State sync note](#state-sync-note)). If music was actually still playing, the device now incorrectly believes it's stopped.
- So pressing Play/Next right after Stop sends a Play/Pause **toggle** — and since the music never really stopped, that toggle pauses it instead of doing nothing or resuming.
- **Takeaway:** on iOS, only use Stop when you know the state actually needs resyncing (e.g. you manually paused from the Spotify app). Don't tap it "just in case" while music is playing.

### Connected, but no button does anything
Watch the Serial Monitor (115200 baud). The heartbeat every 30 s shows `hid=subscribed` or `hid=not-subscribed`.
- `hid=not-subscribed`: the phone connected but didn't enable HID notifications. The watchdog disconnects after 15 s so the phone retries; if it keeps happening, Forget the device on the phone and re-pair.
- `hid=subscribed` and presses are logged but nothing happens on the phone: see [Design notes](#design-notes-read-before-changing-the-bluetooth-code). This was caused by the advertised GAP appearance on a Pixel; do not change it without testing on a Pixel.
- Opening the serial port from a computer resets the ESP32 (DTR/RTS), so a stuck state disappears when you start logging.
- Garbage serial output right after boot plus "Brownout detector was triggered" means weak power: re-seat or replace the USB cable.

### Serial Monitor shows garbage
- Set baud rate to **115200**

### Compile error: missing BLE headers
- Install the ESP32 core (`arduino-cli core install esp32:esp32`). No other libraries are needed.

---

## Project Structure

```
SpotifyConnection-bluetooth/
    SpotifyConnection-bluetooth.ino   ← firmware (ESP32 core BLE, media-key HID)
PIXEL_BUG_INVESTIGATION.md            ← the Pixel investigation and its resolution
GOOGLE_BUG_REPORT.md                  ← report filed with Google (Android Issue Tracker 568711381)
```

---

## Design notes (read before changing the Bluetooth code)

The phone showed this device as connected while every button press was silently dropped. After a long investigation (details in [PIXEL_BUG_INVESTIGATION.md](PIXEL_BUG_INVESTIGATION.md)), the lessons are:

- **The advertised GAP appearance matters.** On a Pixel 9 (Android 17) the Bluetooth HID host dropped our media-key reports when the device advertised as a **Keyboard** (`0x03C1`), even though the link was encrypted, subscribed and the ESP32 sent every notification. Advertising **Generic HID** (`0x03C0`) or **Remote Control** (`0x0180`) fixed it. Generic HID is used because Remote Control stopped working on iPhone. This is `appearanceRemoteControl` in the sketch (the name is historical).
- **`notify()` returning true proves nothing.** Neither NimBLE nor Bluedroid tell you whether the phone is subscribed. That's why the sketch tracks the input report's CCCD (the notification-enable flag, which keeps its state across disconnects and so is cleared on every connection) and watches for non-subscribing hosts.
- **Test on both an iPhone and a Pixel** after any change to the descriptor, appearance, name, security settings or BLE stack. Forget the device on each phone before every test; they cache pairing and GATT data.
- **The stack was replaced for a reason.** The previous NimBLE-Arduino + HijelHID_BLEKeyboard build advertised as a Keyboard and had the same failure (we assume for the same reason; it was not re-tested with a different appearance before switching stacks). The current code is a ~250 line sketch based on the ESP32 core's `BLEHIDDevice` with a consumer-control-only report descriptor; every button is a media usage, so no keyboard report is needed. Trade-off: no NimBLE-style idle power saving.
- **Google issue:** [568711381](https://issuetracker.google.com/issues/568711381) tracks the underlying Android behavior (silently dropping valid HID reports for a keyboard-appearance device). A weekly scheduled task in the Claude desktop app checks it.

---

## Acknowledgments

- Based on the `Server_Gamepad` BLE HID example from [arduino-esp32](https://github.com/espressif/arduino-esp32)
