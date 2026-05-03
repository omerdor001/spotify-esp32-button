// Flow: Button → ESP32 (BLE HID) → Phone → Spotify
// The ESP32 pairs as a Bluetooth media button; the phone forwards the
// Play/Pause key to Spotify natively — no WiFi or Spotify credentials needed.
//
// Required library: "ESP32 BLE Keyboard" by T-vK
//   Install via Arduino Library Manager or https://github.com/T-vK/ESP32-BLE-Keyboard

#include <BleKeyboard.h>

#define BUTTON_PIN 15

BleKeyboard bleKeyboard("Spotify Button", "ESP32", 100);

bool wasPressed = false;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    bleKeyboard.begin();
    Serial.println("BLE started. Pair 'Spotify Button' with your phone.");
}

void loop() {
    bool isPressed = (digitalRead(BUTTON_PIN) == LOW);
    if (isPressed && !wasPressed) {
        if (bleKeyboard.isConnected()) {
            Serial.println("Button — sending Play/Pause");
            bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        } else {
            Serial.println("Button — not connected to phone yet");
        }
    }
    wasPressed = isPressed;
    delay(50);
}
