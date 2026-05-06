# Spotify Button Controller (ESP32)

Control Spotify play/pause with a physical button using an ESP32 over Bluetooth.

**Flow: Button → ESP32 (BLE) → Phone → Spotify**

The ESP32 pairs with your phone as a Bluetooth media controller (like a headset button). When you press the physical button, the ESP32 sends a Play/Pause media key to your phone, and your phone forwards it to Spotify — no WiFi or Spotify credentials needed on the ESP32.

![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue)
![Bluetooth](https://img.shields.io/badge/Bluetooth-BLE%20%28NimBLE%29-blue)
![Spotify](https://img.shields.io/badge/Spotify-via%20Phone-green)
![Arduino](https://img.shields.io/badge/Arduino-CLI%20%2F%20IDE-teal)

---

## Features

- **One-button toggle** — press to play, press again to pause/resume
- **No WiFi required** — Bluetooth only; no network setup needed
- **No credentials** — no Spotify Client ID/Secret on the device
- **Native media keys** — phone handles Spotify routing automatically
- **NimBLE stack** — uses NimBLE-Arduino for lower RAM usage and better long-run stability than the legacy Bluedroid BLE stack

---

## Hardware Requirements

| Part | Notes |
|---|---|
| ESP32 Development Board | NodeMCU-32S or any WROOM/WROVER variant |
| Momentary push button | Tactile switch, arcade button, etc. |
| USB cable | For programming |
| 2 jumper wires | To connect button |

---

## Wiring

```
ESP32          Button
GPIO 4  ───── Pin 1
GND     ───── Pin 2
```

`INPUT_PULLUP` is used — no external resistor needed. Use a **soldered or direct connection**; breadboards with poor contact will cause phantom keypresses (the floating pin picks up RF noise from the on-chip BLE radio).

---

## Software Requirements

### Option A — Arduino CLI (recommended)

```bash
# Install ESP32 core
arduino-cli core install esp32:esp32

# Install libraries
arduino-cli lib install "NimBLE-Arduino" "HijelHID_BLEKeyboard"

# Compile
arduino-cli compile --fqbn esp32:esp32:nodemcu-32s SpotifyConnection-bluetooth

# Flash (replace port as needed)
arduino-cli upload --fqbn esp32:esp32:nodemcu-32s --port /dev/cu.usbserial-0001 SpotifyConnection-bluetooth
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
arduino-cli upload --fqbn esp32:esp32:nodemcu-32s --port /dev/cu.usbserial-0001 SpotifyConnection-bluetooth
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

Open Spotify on your phone, then press the physical button. The ESP32 sends a Play/Pause media key and Spotify responds immediately.

---

## Usage

| Action | Result |
|---|---|
| Press button (Spotify playing) | Pauses |
| Press button (Spotify paused) | Resumes |
| Press button (BLE not connected) | Serial prints "not connected" |

**Requirement:** Spotify must be open on your phone. The button controls an active Spotify session.

---

## Troubleshooting

### Button not responding
- Confirm wiring: GPIO 4 → button → GND
- Avoid breadboards — poor contact causes unreliable readings. Solder the button or use a direct wire for testing.
- Short the two button wires together to test without a button

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

### Compile error: `BleKeyboard.h: No such file or directory`
- You are trying to compile the old Bluedroid-based `SpotifyConnection.ino`. Use `SpotifyConnection-bluetooth/` instead, which uses NimBLE and compiles against ESP32 core 3.x.

---

## Project Structure

```
SpotifyConnection-bluetooth/
    SpotifyConnection-bluetooth.ino   ← current firmware (NimBLE / HijelHID)
SpotifyConnection.ino                 ← legacy Bluedroid version (does not compile on core 3.x)
SpotifyConnection-wifi.ino            ← WiFi-based version (separate approach)
```

---

## Acknowledgments

- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) by h2zero
- [HijelHID_BLEKeyboard](https://github.com/HijelHub/HijelHID_BLEKeyboard) by Hijel
