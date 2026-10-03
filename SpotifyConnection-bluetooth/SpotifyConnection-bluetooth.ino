#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#include <BLE2902.h>

// BLE HID "remote control" that only sends media keys, built on the ESP32
// core's own BLE stack. The earlier NimBLE + HijelHID_BLEKeyboard build looked
// healthy (connected, encrypted, subscribed, notifications sent) but a Pixel 9
// on Android 17 never turned them into input events. This minimal consumer-
// control device with a Remote Control appearance is delivered correctly.

// Name shown when pairing from the phone/PC.
const char *deviceName = "Noam DJ Buttons";

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
// How long a keypress is held before the release report is sent.
const unsigned long tapHoldMs = 25;

// Consumer Control usage IDs.
const uint16_t USAGE_PLAY_PAUSE = 0x00CD;
const uint16_t USAGE_NEXT_TRACK = 0x00B5;
const uint16_t USAGE_PREV_TRACK = 0x00B6;
const uint16_t USAGE_STOP       = 0x00B7;

// GAP appearance: Generic HID. (Keyboard, 0x03C1, was dropped by the Pixel's HID
// host in testing; Remote Control, 0x0180, worked on the Pixel but not an iPhone.)
const uint16_t appearanceRemoteControl = 0x03C0;

// Consumer Control, one 16-bit usage per report, Report ID 1.
const uint8_t hidReportDescriptor[] = {
  0x05, 0x0C,        // Usage Page (Consumer)
  0x09, 0x01,        // Usage (Consumer Control)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x15, 0x00,        //   Logical Minimum (0)
  0x26, 0xFF, 0x03,  //   Logical Maximum (0x3FF)
  0x19, 0x00,        //   Usage Minimum (0)
  0x2A, 0xFF, 0x03,  //   Usage Maximum (0x3FF)
  0x75, 0x10,        //   Report Size (16)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x00,        //   Input (Data, Array)
  0xC0               // End Collection
};

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

BLEHIDDevice      *hid         = nullptr;
BLECharacteristic *inputReport = nullptr;
BLE2902           *inputCccd   = nullptr;
BLEServer         *server      = nullptr;

volatile bool bleConnected = false;
bool          loggedConnected = false;

// ---- HID subscription watchdog ------------------------------------------
// notify() succeeds even when the host never enabled notifications on the HID
// input report, so a link can look connected while every press is silently
// dropped. Some hosts (seen on Android) connect and read the battery level but
// never subscribe to the HID report. Drop such a link so the host reconnects
// and sets HID up again.
const unsigned long subscribeTimeoutMs = 15000;
const unsigned long watchdogRetryMs    = 15000;

unsigned long connectedSinceMs = 0;
unsigned long lastKickMs       = 0;

bool hidSubscribed() {
  return inputCccd != nullptr && inputCccd->getNotifications();
}

// The CCCD keeps its "notifications enabled" bit after a disconnect, so clear it
// for every new connection; otherwise a host that never subscribed would look
// subscribed and the watchdog would not catch it.
void resetSubscription() {
  if (inputCccd != nullptr) {
    inputCccd->setNotifications(false);
  }
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *s) override {
    resetSubscription();
    bleConnected = true;
  }
  void onDisconnect(BLEServer *s) override {
    bleConnected = false;
    resetSubscription();
    // Restart advertising so the host can reconnect.
    BLEDevice::startAdvertising();
  }
};

void logBleConnection(unsigned long now) {
  if (bleConnected == loggedConnected) {
    return;
  }
  loggedConnected = bleConnected;
  if (bleConnected) {
    connectedSinceMs = now;
  }
  Serial.println(bleConnected ? "BLE connected" : "BLE disconnected");
}

void checkSubscriptionWatchdog(unsigned long now) {
  if (!bleConnected || hidSubscribed()) {
    return;
  }
  if (now - connectedSinceMs < subscribeTimeoutMs ||
      now - lastKickMs < watchdogRetryMs) {
    return;
  }
  lastKickMs = now;
  Serial.println("Watchdog: connected but host never subscribed to HID reports; disconnecting");
  server->disconnect(server->getConnId());
}

void logHeartbeat(unsigned long now) {
  if (now - lastHeartbeatMs < heartbeatMs) {
    return;
  }
  lastHeartbeatMs = now;
  Serial.printf("[heartbeat] ble=%s hid=%s\n",
                bleConnected ? "connected" : "disconnected",
                hidSubscribed() ? "subscribed" : "not-subscribed");
}

// True when a press can actually reach the host; logs why not otherwise.
bool canSend(const char *name) {
  if (!bleConnected) {
    Serial.printf("%s: no BLE connection\n", name);
    return false;
  }
  if (!hidSubscribed()) {
    Serial.printf("%s: host has not subscribed to HID reports, press dropped\n", name);
    return false;
  }
  return true;
}

// Press and release a single Consumer Control usage.
void tapUsage(uint16_t usage) {
  uint8_t press[2]   = {(uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8)};
  uint8_t release[2] = {0, 0};
  inputReport->setValue(press, sizeof(press));
  inputReport->notify();
  delay(tapHoldMs);
  inputReport->setValue(release, sizeof(release));
  inputReport->notify();
}

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

void setup() {
  Serial.begin(115200);
  // INPUT_PULLUP: button must short pin to GND when pressed (LOW = pressed).
  pinMode(playNextPin, INPUT_PULLUP);
  pinMode(prevPin,     INPUT_PULLUP);
  pinMode(stopPin,     INPUT_PULLUP);

  Serial.println("Starting BLE HID remote...");
  BLEDevice::init(deviceName);

  // "Just Works" pairing with bonding: no passkey, like a real remote.
  BLESecurity *security = new BLESecurity();
  security->setCapability(ESP_IO_CAP_NONE);
  security->setAuthenticationMode(true, false, true);

  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  hid = new BLEHIDDevice(server);
  hid->manufacturer()->setValue("TOM - Tikkun Olam Makers");
  hid->pnp(0x02, 0x05ac, 0x820a, 0x0110);  // vendor id source, vendor, product, version
  hid->hidInfo(0x00, 0x01);
  hid->reportMap((uint8_t *)hidReportDescriptor, sizeof(hidReportDescriptor));
  inputReport = hid->inputReport(1);
  inputCccd   = (BLE2902 *)inputReport->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  if (inputCccd == nullptr) {
    Serial.println("WARNING: input report has no CCCD, subscription check will fail");
  }
  hid->setBatteryLevel(100);
  hid->startServices();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->setAppearance(appearanceRemoteControl);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.printf("Advertising as '%s'. Pair from your phone/PC.\n", deviceName);
}

void loop() {
  unsigned long now = millis();

  logBleConnection(now);
  checkSubscriptionWatchdog(now);
  logHeartbeat(now);

  if (checkButton(playNextBtn, playNextPin, now)) {
    if (!canSend("Play/Next")) {
      // logged by canSend
    } else if (!isPlaying) {
      Serial.println("Play/Next: play");
      tapUsage(USAGE_PLAY_PAUSE);
      isPlaying        = true;
      lastPlayOrNextMs = now;
    } else if (now - lastPlayOrNextMs < nextThrottleMs) {
      Serial.println("Play/Next: next ignored (throttle)");
    } else {
      Serial.println("Play/Next: next");
      tapUsage(USAGE_NEXT_TRACK);
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
      tapUsage(USAGE_PREV_TRACK);
      lastPrevMs = now;
    }
  }

  if (checkButton(stopBtn, stopPin, now)) {
    if (!canSend("Stop")) {
      // logged by canSend
    } else {
      Serial.println("Stop: stop");
      tapUsage(USAGE_STOP);
      isPlaying = false;
    }
  }
}
