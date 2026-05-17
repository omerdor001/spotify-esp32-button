#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>
// Name shown when pairing from the phone/PC. Manufacturer + battery % are cosmetic.
HijelHID_BLEKeyboard bleKeyboard("Spotify Button", "DIY", 100);
const int buttonPin = 4;
// Pin must stay continuously LOW for minHoldMs before a press is registered.
// This filters out EMI noise spikes (< 1ms) and antenna pickup from the BLE radio.
// With a proper tactile button, 10ms is more than enough.
const unsigned long minHoldMs = 10;
// After a press fires, ignore all input for cooldownMs to suppress bounce on release.
const unsigned long cooldownMs = 200;
unsigned long lowSinceMs = 0;
bool pinIsLow = false;
unsigned long lastPressMs = 0;
void setup() {
  Serial.begin(115200);
  // INPUT_PULLUP: button must short pin to GND when pressed (LOW = pressed).
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Starting BLE HID keyboard...");
  bleKeyboard.begin();
  Serial.println("Advertising as 'Spotify Button'. Pair from your phone/PC.");
}
void loop() {
  int raw = digitalRead(buttonPin);
  unsigned long now = millis();
  if (raw == LOW) {
    if (!pinIsLow) {
      pinIsLow = true;
      lowSinceMs = now;
    } else if (now - lowSinceMs >= minHoldMs && now - lastPressMs >= cooldownMs) {
      lastPressMs = now;
      pinIsLow = false;
      if (bleKeyboard.isConnected()) {
        Serial.println("Button pressed -> Stop");
        bleKeyboard.tap(KEY_MEDIA_STOP); 
      } else {
        Serial.println("Button pressed but no BLE host connected");
      }
    }
  } else {
    pinIsLow = false;
  }
}