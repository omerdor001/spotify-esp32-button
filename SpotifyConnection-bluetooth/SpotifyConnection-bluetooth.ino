#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>

// Name shown when pairing from the phone/PC. Manufacturer + battery % are cosmetic.
HijelHID_BLEKeyboard bleKeyboard("Spotify Button", "DIY", 100);

const int playNextPin = 4;
const int prevPin     = 17;
const int stopPin     = 18;

// Pin must stay continuously LOW for minHoldMs before a press is registered.
// Filters out EMI noise spikes (< 1ms) and BLE radio antenna pickup.
const unsigned long minHoldMs = 10;
// After a press fires, ignore all input for cooldownMs to suppress bounce on release.
const unsigned long cooldownMs = 200;
// Minimum gap between a Play/Next command and the next Next command.
// Prevents an accidental double-tap from skipping two tracks.
const unsigned long nextThrottleMs = 5000;
const unsigned long heartbeatMs = 30000;

struct ButtonState {
  bool         pinIsLow;
  unsigned long lowSinceMs;
  unsigned long lastPressMs;
};

ButtonState playNextBtn = {false, 0, 0};
ButtonState prevBtn     = {false, 0, 0};
ButtonState stopBtn     = {false, 0, 0};

// Tracks whether we believe music is currently playing, based solely on what
// commands we have sent. Cannot reflect phone-side state (playlist end, manual
// pause from phone, etc.) — that's an inherent BLE HID limitation.
bool isPlaying = false;

// Timestamp of the last Play or Next command sent; used for nextThrottleMs.
unsigned long lastPlayOrNextMs = 0;
unsigned long lastHeartbeatMs  = 0;
bool bleConnected              = false;

// Returns true once when a button has been held LOW for minHoldMs, then resets
// so it won't fire again until the button is released and pressed again
// (enforced by cooldownMs after the last firing).
bool checkButton(ButtonState &btn, int pin, unsigned long now) {
  int raw = digitalRead(pin);
  if (raw == LOW) {
    if (!btn.pinIsLow) {
      btn.pinIsLow   = true;
      btn.lowSinceMs = now;
    } else if (now - btn.lowSinceMs >= minHoldMs &&
               now - btn.lastPressMs >= cooldownMs) {
      btn.lastPressMs = now;
      btn.pinIsLow    = false;
      return true;
    }
  } else {
    btn.pinIsLow = false;
  }
  return false;
}

void logBleConnection(bool connected) {
  if (connected == bleConnected) {
    return;
  }
  bleConnected = connected;
  Serial.println(connected ? "BLE connected" : "BLE disconnected");
}

void logHeartbeat(unsigned long now) {
  if (now - lastHeartbeatMs < heartbeatMs) {
    return;
  }
  lastHeartbeatMs = now;
  Serial.printf("[heartbeat] ble=%s\n",
                bleKeyboard.isConnected() ? "connected" : "disconnected");
}

void setup() {
  Serial.begin(115200);
  // INPUT_PULLUP: button must short pin to GND when pressed (LOW = pressed).
  pinMode(playNextPin, INPUT_PULLUP);
  pinMode(prevPin,     INPUT_PULLUP);
  pinMode(stopPin,     INPUT_PULLUP);

  Serial.println("Starting BLE HID keyboard...");
  bleKeyboard.begin();
  Serial.println("Advertising as 'Spotify Button'. Pair from your phone/PC.");
}

void loop() {
  unsigned long now = millis();

  logBleConnection(bleKeyboard.isConnected());
  logHeartbeat(now);

  if (checkButton(playNextBtn, playNextPin, now)) {
    if (!bleKeyboard.isConnected()) {
      Serial.println("Play/Next: no BLE connection");
    } else if (!isPlaying) {
      Serial.println("Play/Next: play");
      bleKeyboard.tap(MEDIA_PLAY_PAUSE);
      isPlaying        = true;
      lastPlayOrNextMs = now;
    } else if (now - lastPlayOrNextMs < nextThrottleMs) {
      Serial.println("Play/Next: next ignored (throttle)");
    } else {
      Serial.println("Play/Next: next");
      bleKeyboard.tap(MEDIA_NEXT_TRACK);
      lastPlayOrNextMs = now;
    }
  }

  if (checkButton(prevBtn, prevPin, now)) {
    if (!bleKeyboard.isConnected()) {
      Serial.println("Prev: no BLE connection");
    } else {
      Serial.println("Prev: previous track");
      bleKeyboard.tap(MEDIA_PREV_TRACK);
    }
  }

  if (checkButton(stopBtn, stopPin, now)) {
    if (!bleKeyboard.isConnected()) {
      Serial.println("Stop: no BLE connection");
    } else {
      Serial.println("Stop: stop");
      bleKeyboard.tap(MEDIA_STOP);
      isPlaying = false;
    }
  }
}
