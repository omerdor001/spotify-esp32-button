# Spotify Button Controller (ESP32)

Control Spotify with three physical buttons using an ESP32 over Bluetooth.

**Flow: Button → ESP32 (BLE) → Phone → Spotify**

The ESP32 pairs with your phone as a Bluetooth media controller (like a headset button). When you press a button, the ESP32 sends a media key to your phone, and your phone forwards it to Spotify — no WiFi or Spotify credentials needed on the ESP32.

![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue)
![Bluetooth](https://img.shields.io/badge/Bluetooth-BLE%20%28NimBLE%29-blue)
![Spotify](https://img.shields.io/badge/Spotify-via%20Phone-green)
![Arduino](https://img.shields.io/badge/Arduino-CLI%20%2F%20IDE-teal)

---

## Features

- **Play/Next button** — press to play if stopped; press to skip to the next track if already playing
- **Prev button** — skips to the previous track
- **Stop button** — stops playback and resets internal state
- **Next-press throttle** — Next is ignored if a Play or Next was sent in the last 5 seconds, preventing accidental double-skips
- **No WiFi required** — Bluetooth only; no network setup needed
- **No credentials** — no Spotify Client ID/Secret on the device
- **Native media keys** — phone handles Spotify routing automatically
- **NimBLE stack** — uses NimBLE-Arduino for lower RAM usage and better long-run stability than the legacy Bluedroid BLE stack

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

Both libraries live in the `libraries/` folder of this repo — no separate install step needed.

```bash
# Install ESP32 core (one-time)
arduino-cli core install esp32:esp32

# Compile and flash in one step (replace port as needed)
arduino-cli compile --upload \
  --fqbn esp32:esp32:nodemcu-32s \
  --port /dev/cu.usbserial-0001 \
  --library libraries/HijelHID_BLEKeyboard \
  --library libraries/NimBLE-Arduino \
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

3. Install libraries via `Sketch → Include Library → Manage Libraries`:

| Library | Author |
|---|---|
| **NimBLE-Arduino** | h2zero |
| **HijelHID_BLEKeyboard** | Hijel |

---

## Setup

### 1. Upload to ESP32

**Arduino CLI:**
```bash
arduino-cli compile --upload \
  --fqbn esp32:esp32:nodemcu-32s \
  --port /dev/cu.usbserial-0001 \
  --library libraries/HijelHID_BLEKeyboard \
  --library libraries/NimBLE-Arduino \
  SpotifyConnection-bluetooth/
```

**Arduino IDE:**
- `Tools → Board → NodeMCU-32S`
- `Tools → Port → [your port]`
- Click **Upload**

### 2. Pair with Your Phone

After uploading, open the Serial Monitor (115200 baud). You'll see:

```
Starting BLE HID keyboard...
Advertising as 'Spotify Button'. Pair from your phone/PC.
```

On your phone:
- **Android:** Settings → Bluetooth → pair **Spotify Button**
- **iOS:** Settings → Bluetooth → pair **Spotify Button**

### 3. Use It

Open Spotify on your phone, then use the buttons. The ESP32 sends media keys and Spotify responds immediately.

---

## Usage

| Button | Situation | Action |
|---|---|---|
| Play/Next (GPIO 4) | Spotify stopped | Plays |
| Play/Next (GPIO 4) | Spotify playing | Skips to next track |
| Play/Next (GPIO 4) | Within 5s of last Play/Next | Ignored (throttle) |
| Prev (GPIO 17) | Any | Skips to previous track |
| Stop (GPIO 18) | Any | Stops playback |
| Any button | BLE not connected | Serial prints "not connected" |

**Requirement:** Spotify must be open on your phone. The button controls an active Spotify session.

### State sync note

The device tracks playback state locally based on what commands it has sent. If playback is changed from the phone (e.g. someone pauses Spotify manually), the device won't know. In that case, press **Stop** once to reset the device's state, then **Play/Next** to resume. This is an inherent limitation of BLE HID, which is a write-only channel.

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

### Phone doesn't see "Spotify Button"
- Make sure Bluetooth is on
- Press the ESP32 reset button and try pairing again

### Paired but Play/Pause doesn't work
- **Android:** check that the BLE device has "Media" permissions in Bluetooth device settings
- **iOS:** make sure Spotify is in the foreground the first time you press

### Serial Monitor shows garbage
- Set baud rate to **115200**

### Compile error: missing library headers
- Make sure you pass both `--library` paths (CLI) or have **NimBLE-Arduino** and **HijelHID_BLEKeyboard** installed (IDE). See [Software Requirements](#software-requirements).

---

## Project Structure

```
SpotifyConnection-bluetooth/
    SpotifyConnection-bluetooth.ino   ← firmware (NimBLE / HijelHID)
libraries/                            ← NimBLE-Arduino + HijelHID_BLEKeyboard (gitignored; clone or install separately)
```

---

## Acknowledgments

- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) by h2zero
- [HijelHID_BLEKeyboard](https://github.com/HijelHub/HijelHID_BLEKeyboard) by Hijel
