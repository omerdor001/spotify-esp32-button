# Spotify Button Controller (ESP32)

Control Spotify play/pause with a physical button using an ESP32 over Bluetooth.

**Flow: Button → ESP32 (BLE) → Phone → Spotify**

The ESP32 pairs with your phone as a Bluetooth media controller (like a headset button). When you press the physical button, the ESP32 sends a Play/Pause media key to your phone, and your phone forwards it to Spotify — no WiFi or Spotify credentials needed on the ESP32.

![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue)
![Bluetooth](https://img.shields.io/badge/Bluetooth-BLE-blue)
![Spotify](https://img.shields.io/badge/Spotify-via%20Phone-green)
![Arduino](https://img.shields.io/badge/Arduino-IDE-teal)

---

## Features

- **One-button toggle** — press to play, press again to pause/resume
- **No WiFi required** — Bluetooth only; no network setup needed
- **No credentials** — no Spotify Client ID/Secret on the device
- **Native media keys** — phone handles Spotify routing automatically
- **Minimal code** — simple BLE HID implementation

---

## Hardware Requirements

| Part | Notes |
|---|---|
| ESP32 Development Board | Any WROOM/WROVER variant |
| Momentary push button | Arcade button, tactile switch, etc. |
| USB cable | For programming |
| 2 jumper wires | To connect button |

---

## Wiring

```
ESP32          Button
GPIO 15 ───── Pin 1
GND     ───── Pin 2
```

`INPUT_PULLUP` is used — no external resistor needed.

---

## Software Requirements

### Arduino IDE

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - `File → Preferences` → Additional Boards Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - `Tools → Board → Boards Manager` → search **ESP32** → Install

### Library

Install via `Sketch → Include Library → Manage Libraries`:

| Library | Author |
|---|---|
| **ESP32 BLE Keyboard** | T-vK |

> Or install manually from [github.com/T-vK/ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard)

---

## Setup

### 1. Upload to ESP32

- `Tools → Board → ESP32 Dev Module`
- `Tools → Port → [your port]`
- Click **Upload**

### 2. Pair with Your Phone

After uploading, open the Serial Monitor (115200 baud). You'll see:

```
BLE started. Pair 'Spotify Button' with your phone.
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
| Press button (phone not connected) | Serial prints "not connected" |

**Requirement:** Spotify must be open on your phone. The button controls an active Spotify session.

---

## Troubleshooting

### Button not responding
- Confirm wiring: GPIO 15 → button → GND
- Short the two button wires together to test without the button

### Phone doesn't see "Spotify Button"
- Make sure Bluetooth is on and your phone isn't already paired at its device limit
- Press the ESP32 reset button and try pairing again

### Paired but Play/Pause doesn't work
- Some Android phones require you to grant "Media" permissions to the BLE device — check the Bluetooth device settings on your phone
- On iOS, make sure Spotify is in the foreground the first time you press the button

### Serial Monitor shows garbage
- Set baud rate to **115200**

---

## Acknowledgments

- [ESP32 BLE Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) by T-vK
