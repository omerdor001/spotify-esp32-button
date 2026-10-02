#include <Arduino.h>
#include <HijelHID_BLEKeyboard.h>
#include <NimBLEDevice.h>

// Name shown when pairing from the phone/PC. Manufacturer + battery % are cosmetic.
HijelHID_BLEKeyboard bleKeyboard("Noam DJ Buttons", "TOM - Tikkun Olam Makers", 100);

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
// Minimum gap between consecutive Prev commands.
// Prevents an accidental double-tap from skipping two tracks back.
const unsigned long prevThrottleMs = 5000;
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
// Timestamp of the last Prev command sent; used for prevThrottleMs.
unsigned long lastPrevMs       = 0;
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

// ---- HID subscription watchdog ------------------------------------------
// NimBLE's notify() returns true even when the host never enabled notifications
// on the HID input reports, so a link can look connected while every press is
// silently dropped. Some hosts (seen on Android) connect and read the battery
// level but never subscribe to the HID reports. We track subscriptions and
// drop such a link so the host reconnects and sets up HID again.
const unsigned long subscribeTimeoutMs = 15000;
const unsigned long watchdogRetryMs    = 15000;

volatile bool hidSubscribed = false;  // any HID input report currently subscribed
unsigned long connectedSinceMs = 0;
unsigned long lastKickMs       = 0;

class HidSubscribeCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic *chr, NimBLEConnInfo &connInfo, uint16_t subValue) override {
    hidSubscribed = subValue != 0;
    Serial.printf("HID report 0x%04X %s\n", chr->getHandle(),
                  subValue ? "subscribed" : "unsubscribed");
  }
};
HidSubscribeCallbacks hidSubscribeCallbacks;

// Attach to the HID service's notifiable (input) report characteristics.
void watchHidSubscriptions() {
  NimBLEServer *server = NimBLEDevice::getServer();
  NimBLEService *hid   = server ? server->getServiceByUUID((uint16_t)0x1812) : nullptr;
  if (!hid) {
    Serial.println("Watchdog: HID service not found, subscription check disabled");
    return;
  }
  for (NimBLECharacteristic *chr : hid->getCharacteristics((uint16_t)0x2A4D)) {
    if (chr->getProperties() & NIMBLE_PROPERTY::NOTIFY) {
      chr->setCallbacks(&hidSubscribeCallbacks);
    }
  }
}

void checkSubscriptionWatchdog(unsigned long now) {
  if (!bleConnected || hidSubscribed) {
    return;
  }
  if (now - connectedSinceMs < subscribeTimeoutMs ||
      now - lastKickMs < watchdogRetryMs) {
    return;
  }
  lastKickMs = now;
  Serial.println("Watchdog: connected but host never subscribed to HID reports; disconnecting");
  NimBLEServer *server = NimBLEDevice::getServer();
  if (server) {
    for (uint16_t handle : server->getPeerDevices()) {
      server->disconnect(handle);
    }
  }
}

void logBleConnection(bool connected, unsigned long now) {
  if (connected == bleConnected) {
    return;
  }
  bleConnected = connected;
  hidSubscribed = false;
  if (connected) {
    connectedSinceMs = now;
  }
  Serial.println(connected ? "BLE connected" : "BLE disconnected");
}

// True when a press can actually reach the host; logs why not otherwise.
bool canSend(const char *name) {
  if (!bleKeyboard.isConnected()) {
    Serial.printf("%s: no BLE connection\n", name);
    return false;
  }
  if (!hidSubscribed) {
    Serial.printf("%s: host has not subscribed to HID reports, press dropped\n", name);
    return false;
  }
  return true;
}

void logHeartbeat(unsigned long now) {
  if (now - lastHeartbeatMs < heartbeatMs) {
    return;
  }
  lastHeartbeatMs = now;
  Serial.printf("[heartbeat] ble=%s hid=%s\n",
                bleKeyboard.isConnected() ? "connected" : "disconnected",
                hidSubscribed ? "subscribed" : "not-subscribed");
}

void setup() {
  Serial.begin(115200);
  // INPUT_PULLUP: button must short pin to GND when pressed (LOW = pressed).
  pinMode(playNextPin, INPUT_PULLUP);
  pinMode(prevPin,     INPUT_PULLUP);
  pinMode(stopPin,     INPUT_PULLUP);

  Serial.println("Starting BLE HID keyboard...");
  bleKeyboard.begin();
  watchHidSubscriptions();
  Serial.println("Advertising as 'Spotify Button'. Pair from your phone/PC.");
}

void loop() {
  unsigned long now = millis();

  logBleConnection(bleKeyboard.isConnected(), now);
  checkSubscriptionWatchdog(now);
  logHeartbeat(now);

  if (checkButton(playNextBtn, playNextPin, now)) {
    if (!canSend("Play/Next")) {
      // logged by canSend
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
    if (!canSend("Prev")) {
      // logged by canSend
    } else if (now - lastPrevMs < prevThrottleMs) {
      Serial.println("Prev: ignored (throttle)");
    } else {
      Serial.println("Prev: previous track");
      bleKeyboard.tap(MEDIA_PREV_TRACK);
      lastPrevMs = now;
    }
  }

  if (checkButton(stopBtn, stopPin, now)) {
    if (!canSend("Stop")) {
      // logged by canSend
    } else {
      Serial.println("Stop: stop");
      bleKeyboard.tap(MEDIA_STOP);
      isPlaying = false;
    }
  }
}
