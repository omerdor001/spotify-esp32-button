# 🎵 Spotify Playlist Button Controller

Control your Spotify playlists with a physical button using ESP32!

Press a button and instantly start playing your favorite playlist - no phone or computer needed.

![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue)
![Spotify](https://img.shields.io/badge/Spotify-API-green)
![Arduino](https://img.shields.io/badge/Arduino-IDE-teal)

---

## ✨ Features

- 🎯 **One-click playlist control** - Press button to play your playlist
- ⏯️ **Smart toggle** - Play/pause if something is already playing
- 🔄 **Auto token refresh** - Never need to re-authenticate
- 📶 **WiFi multi-network support** - Works anywhere
- 🎨 **Accessible design ready** - Easy to add large buttons for accessibility
- 🔐 **Secure OAuth2** - Industry-standard authentication

---

## 🛠️ Hardware Requirements

### Required:
- **ESP32 Development Board** (~$5-10)
- **Push Button** or any momentary switch
- **USB Cable** for programming
- **2 Jumper Wires** (optional if button plugs directly)

### Optional (for accessibility):
- **Large Arcade Button** (60mm) - easier to press
- **Dome Button with LED** - visual feedback
- **Foot Switch** - hands-free operation

---

## 📚 Software Requirements

### Arduino IDE Setup:

1. **Install Arduino IDE** (1.8.x or 2.x)
   - Download from: https://www.arduino.cc/en/software

2. **Add ESP32 Board Support:**
   - Open Arduino IDE
   - Go to: `File → Preferences`
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to: `Tools → Board → Boards Manager`
   - Search for "ESP32" and install

3. **Install Required Libraries:**
   
   Via Library Manager (`Sketch → Include Library → Manage Libraries`):
   
   - **SpotifyEsp32** (by Finian Landes)
   - **ArduinoJson** (version 7.x)

---

## 🔌 Wiring Diagram

### Simple Connection:

```
ESP32                    Button
┌────────┐              ┌──────┐
│        │              │      │
│ GPIO 2 ├──────────────┤  Pin1│
│        │              │      │
│  GND   ├──────────────┤  Pin2│
│        │              │      │
└────────┘              └──────┘
```

**That's it!** Only 2 wires needed.

### Connection Details:
- Button Pin 1 → ESP32 **GPIO 2**
- Button Pin 2 → ESP32 **GND**

> **Note:** We use `INPUT_PULLUP` mode, so no external resistor needed!

---

## 🚀 Quick Start Guide

### Step 1: Get Spotify API Credentials

1. Go to [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Log in with your Spotify account
3. Click **"Create App"**
4. Fill in the details:
   - **App Name:** ESP32 Button Controller
   - **App Description:** Physical button to control Spotify
   - **Redirect URI:** `https://spotifyesp32.vercel.app/api/spotify/callback`
   - **API:** Check "Web API"
5. Click **Save**
6. Copy your **Client ID** and **Client Secret**

---

### Step 2: Get Your Playlist ID

#### Option A - Desktop:
1. Open Spotify
2. Right-click on your playlist
3. Share → Copy Playlist Link
4. You'll get: `https://open.spotify.com/playlist/37i9dQZF1DXcBWIGoYBM5M`
5. The ID is: `37i9dQZF1DXcBWIGoYBM5M`

#### Option B - Mobile:
1. Open playlist in Spotify app
2. Tap ⋯ (three dots)
3. Share → Copy Link
4. Extract the ID from the link

---

### Step 3: Configure the Code

1. **Clone this repository:**
   ```bash
   git clone https://github.com/omerdor001/spotify_playlist_connection_to_button.git
   ```

2. **Open `spotify_button.ino` in Arduino IDE**

3. **Update your credentials:**

```cpp
// WiFi Settings
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Spotify Settings
const char* client_id = "YOUR_CLIENT_ID";
const char* client_secret = "YOUR_CLIENT_SECRET";

// Your Playlist
const char* MY_PLAYLIST = "YOUR_PLAYLIST_ID";
```

---

### Step 4: First Time Setup (Authentication)

1. **Upload the code to ESP32:**
   - Select: `Tools → Board → ESP32 Dev Module`
   - Select: `Tools → Port → [Your ESP32 Port]`
   - Click **Upload**

2. **Open Serial Monitor:**
   - `Tools → Serial Monitor`
   - Set baud rate to: **115200**

3. **You'll see:**
   ```
   📡 Connecting to WiFi...
   ✅ WiFi Connected!
   
   🔑 To authenticate, open this URL in your browser:
   https://spotifyesp32.vercel.app/?clientId=YOUR_ID
   
   ⏳ Waiting for authorization...
   ```

4. **Copy the URL** and open it in your browser

5. **Log in to Spotify** and authorize the app

6. **Return to Serial Monitor:**
   ```
   ✅ Connected!
   💾 SAVE THIS TOKEN:
   BQC8x...very_long_token...xyz
   ```

7. **Copy the Refresh Token** and save it!

---

### Step 5: Add Refresh Token (One-time)

Update your code with the token:

```cpp
// Add this line:
const char* refresh_token = "YOUR_REFRESH_TOKEN_HERE";

// Change this line:
// FROM:
Spotify spotify(client_id, client_secret);

// TO:
Spotify spotify(client_id, client_secret, refresh_token);
```

**Remove or comment out the authentication loop:**

```cpp
/*
while (!spotify.is_auth()) {
  spotify.handle_client();
  delay(100);
}
*/
```

Upload the code again. Now it will connect automatically! ✅

---

## 🎮 Usage

### Basic Operation:

- **Press button once:**
  - If nothing playing → Start your playlist
  - If playing → Pause
  - If paused → Resume

### Serial Monitor Output:

```
🎵 Button pressed!
🎵 Starting playlist...
📡 Response: 204
✅ Playing!
```

---

## 🎨 Customization Ideas

### Multiple Playlists with Multiple Buttons:

```cpp
const char* PLAYLIST_MORNING = "37i9dQZF1DXcBWIGoYBM5M";
const char* PLAYLIST_WORKOUT = "37i9dQZF1DX0XUsuxWHRQd";
const char* PLAYLIST_SLEEP = "37i9dQZF1DX4sWSpwq3LiO";

const int BUTTON_1 = 2;
const int BUTTON_2 = 4;
const int BUTTON_3 = 5;

void loop() {
  if (digitalRead(BUTTON_1) == LOW) {
    playPlaylist("spotify:playlist:" + String(PLAYLIST_MORNING));
  }
  if (digitalRead(BUTTON_2) == LOW) {
    playPlaylist("spotify:playlist:" + String(PLAYLIST_WORKOUT));
  }
  if (digitalRead(BUTTON_3) == LOW) {
    playPlaylist("spotify:playlist:" + String(PLAYLIST_SLEEP));
  }
  delay(50);
}
```

### Add Visual Feedback with LED:

```cpp
const int LED_PIN = 5;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  // ... rest of setup
}

void playPlaylist(const char* uri) {
  digitalWrite(LED_PIN, HIGH);  // LED on while playing
  // ... play code
  digitalWrite(LED_PIN, LOW);   // LED off
}
```

---

## 🔧 Troubleshooting

### Button Not Responding:

**Check:**
- ✅ Wiring: GPIO 2 and GND are connected
- ✅ Code: `pinMode(buttonPin, INPUT_PULLUP);` is set
- ✅ Button works: short the two wires together

### WiFi Not Connecting:

**Check:**
- ✅ SSID and password are correct (case-sensitive!)
- ✅ ESP32 is in range of WiFi
- ✅ WiFi is 2.4GHz (ESP32 doesn't support 5GHz)

### Spotify Not Playing:

**Error 401 - Unauthorized:**
- Token expired → Run authentication again

**Error 404 - Device Not Found:**
- Open Spotify on at least one device (phone/computer) before pressing button

**Error 403 - Forbidden:**
- Check that your Spotify account has correct permissions
- Make sure the playlist is accessible

### Serial Monitor Shows Garbage:

**Fix:**
- Set baud rate to **115200** in Serial Monitor

---

## 🌍 Alternative Hardware

### Compatible ESP32 Boards:
- ESP32 DevKit V1
- ESP32 WROOM
- ESP32 WROVER
- NodeMCU-32S

### Alternative Button Types:
- **Arcade Button** - Large, colorful, easy to press
- **Tactile Switch** - Small PCB-mounted button
- **Mechanical Switch** - From old appliances
- **DIY Pressure Pad** - Made with aluminum foil and foam

---

## 🔐 Security Notes

- **Never commit your credentials** to public repositories
- Store `client_secret` and `refresh_token` securely
- Use `.gitignore` to exclude configuration files:

```gitignore
# Add to .gitignore
config.h
secrets.h
*.private
```

**Better approach:** Store credentials in a separate file:

```cpp
// Create config.h (add to .gitignore)
#ifndef CONFIG_H
#define CONFIG_H

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* client_id = "YOUR_CLIENT_ID";
const char* client_secret = "YOUR_CLIENT_SECRET";
const char* refresh_token = "YOUR_TOKEN";

#endif
```

Then in main file:
```cpp
#include "config.h"
```

---

## 📖 API Reference

### SpotifyEsp32 Library Methods Used:

```cpp
// Authentication
spotify.begin();
spotify.is_auth();
spotify.handle_client();
spotify.get_user_tokens();

// Playback Control
spotify.get_current_playback();
spotify.pause_playback();
spotify.start_a_users_playback();
```

### Custom Function:

```cpp
void playPlaylist(const char* playlist_uri)
```
Plays a specific playlist using direct API call with HTTPClient.

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

### To Do:
- [ ] Add support for album playback
- [ ] Implement volume control with potentiometer
- [ ] Add OLED display for track info
- [ ] Support for multiple WiFi networks
- [ ] Add sleep mode to save power

---

## 📜 License

This project is open source and available under the [MIT License](LICENSE).

---

## 🙏 Acknowledgments

- **SpotifyEsp32 Library** by [Finian Landes](https://github.com/FinianLandes/Spotify_Esp32)
- **Spotify Web API** for the amazing API
- **Arduino Community** for ESP32 support

---

## 📞 Support

Having issues? Here's how to get help:

1. **Check Troubleshooting section** above
2. **Open an issue** on GitHub with:
   - Your hardware setup
   - Serial Monitor output
   - Error messages
3. **Join discussions** in Issues tab

---

## 🌟 Show Your Support

If this project helped you, please give it a ⭐ on GitHub!

---

**Built with ❤️ for music lovers who want a simple, physical way to control their playlists**
