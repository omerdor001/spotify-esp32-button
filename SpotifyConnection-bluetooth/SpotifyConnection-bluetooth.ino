#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>

// Name shown when pairing from the phone/PC. Manufacturer + battery % are cosmetic.
HijelHID_BLEKeyboard bleKeyboard("Spotify Button", "DIY", 100);

const int playNextPin = 4;
const int stopPin    = 18;

// Pin must stay continuously LOW for minHoldMs before a press is registered.
// Filters out EMI noise spikes (< 1ms) and BLE radio antenna pickup.
const unsigned long minHoldMs = 10;
// After a press fires, ignore all input for cooldownMs to suppress bounce on release.
const unsigned long cooldownMs = 200;
// Minimum gap between a Play/Next command and the next Next command.
// Prevents an accidental double-tap from skipping two tracks.
const unsigned long nextThrottleMs = 5000;

struct ButtonState {
  bool         pinIsLow;
  unsigned long lowSinceMs;
  unsigned long lastPressMs;
};

ButtonState playNextBtn = {false, 0, 0};
ButtonState stopBtn     = {false, 0, 0};

// Tracks whether we believe music is currently playing, based solely on what
// commands we have sent. Cannot reflect phone-side state (playlist end, manual
// pause from phone, etc.) — that's an inherent BLE HID limitation.
bool isPlaying = false;

// Timestamp of the last Play or Next command sent; used for nextThrottleMs.
unsigned long lastPlayOrNextMs = 0;

// Returns true once when a button has been held LOW for minHoldMs, then resets
// so it won't fire again until the button is released and pressed again
// (enforced by cooldownMs after the last firing).
bool checkButton(ButtonState &btn, int pin, unsigned long now) {
  int raw = digitalRead(pin);
  if (raw == LOW) {
    if (!btn.pinIsLow) {
      btn.pinIsLow    = true;
      btn.lowSinceMs  = now;
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

void setup() {
  Serial.begin(115200);
  // INPUT_PULLUP: button must short pin to GND when pressed (LOW = pressed).
  pinMode(playNextPin, INPUT_PULLUP);
  pinMode(stopPin,     INPUT_PULLUP);

  Serial.println("Starting BLE HID keyboard...");
  bleKeyboard.begin();
  Serial.println("Advertising as 'Spotify Button'. Pair from your phone/PC.");
}

void loop() {
  unsigned long now = millis();

  if (checkButton(playNextBtn, playNextPin, now)) {
    if (!bleKeyboard.isConnected()) {
      Serial.println("Play/Next button pressed but no BLE host connected");
    } else if (!isPlaying) {
      Serial.println("Play/Next -> Play");
      bleKeyboard.tap(MEDIA_PLAY_PAUSE);
      isPlaying          = true;
      lastPlayOrNextMs   = now;
    } else if (now - lastPlayOrNextMs < nextThrottleMs) {
      Serial.print("Play/Next -> Next IGNORED (throttle, ");
      Serial.print((nextThrottleMs - (now - lastPlayOrNextMs)) / 1000UL);
      Serial.println("s remaining)");
    } else {
      Serial.println("Play/Next -> Next");
      bleKeyboard.tap(MEDIA_NEXT_TRACK);
      lastPlayOrNextMs = now;
      // isPlaying stays true — next track starts immediately on most players
    }
  }

  if (checkButton(stopBtn, stopPin, now)) {
    if (!bleKeyboard.isConnected()) {
      Serial.println("Stop button pressed but no BLE host connected");
    } else {
      Serial.println("Stop button -> Stop");
      bleKeyboard.tap(MEDIA_STOP);
      isPlaying = false;
    }
  }
}
