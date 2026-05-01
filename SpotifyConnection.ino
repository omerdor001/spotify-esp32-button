#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <SpotifyArduino.h>
#include <SpotifyArduinoCert.h>
#include <Preferences.h>
#include <WebServer.h>

#define CLIENT_ID     "your_spotify_client_id"
#define CLIENT_SECRET "your_spotify_client_secret"
#define BUTTON_PIN    15

Preferences      prefs;
WiFiClientSecure secureClient;
WebServer        server(80);
SpotifyArduino   spotify(secureClient, CLIENT_ID, CLIENT_SECRET);

bool authDone    = false;
bool wasPressed  = false;
bool g_isPlaying = false;

String callbackUri() {
    return "http://" + WiFi.localIP().toString() + "/callback";
}

void handleRoot() {
    String url = String("https://accounts.spotify.com/authorize") +
                 "?client_id="         + CLIENT_ID +
                 "&response_type=code" +
                 "&redirect_uri="      + callbackUri() +
                 "&scope=user-read-playback-state%20user-modify-playback-state";
    server.sendHeader("Location", url, true);
    server.send(302, "text/plain", "");
}

void handleCallback() {
    for (uint8_t i = 0; i < server.args(); i++) {
        if (server.argName(i) != "code") continue;
        const char* token = spotify.requestAccessTokens(
            server.arg(i).c_str(), callbackUri().c_str());
        if (token) {
            prefs.begin("spotify", false);
            prefs.putString("refresh_token", token);
            prefs.end();
            server.send(200, "text/html",
                "<h2>Authenticated!</h2>"
                "<p>Token saved. Close this tab — press the button to control Spotify.</p>");
            authDone = true;
            Serial.println("Refresh token saved. Ready.");
            return;
        }
    }
    server.send(500, "text/plain", "Auth failed — check Serial Monitor.");
}

void playerDetailsCallback(PlayerDetails details) {
    g_isPlaying = details.isPlaying;
}

void togglePlayback() {
    int status = spotify.getPlayerDetails(playerDetailsCallback);
    if (status == 200) {
        if (g_isPlaying) {
            spotify.pause();
            Serial.println("Paused.");
        } else {
            spotify.play();
            Serial.println("Playing.");
        }
    } else if (status == 204) {
        Serial.println("No active Spotify device — open Spotify on any device first.");
    } else {
        Serial.printf("getPlayerDetails failed: %d\n", status);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    WiFiManager wm;
    if (!wm.autoConnect("ESP32-Spotify-Setup")) {
        Serial.println("WiFi failed — restarting.");
        ESP.restart();
    }
    Serial.println("WiFi: " + WiFi.localIP().toString());
    secureClient.setCACert(spotify_server_cert);
    prefs.begin("spotify", true);
    String saved = prefs.getString("refresh_token", "");
    prefs.end();
    if (saved.length() > 0) {
        spotify.setRefreshToken(saved.c_str());
        authDone = true;
        Serial.println("Loaded saved token. Ready.");
    } else {
        // First boot: serve OAuth flow from the ESP32 itself.
        // Add  http://<ESP32_IP>/callback  to your Spotify app's Redirect URIs.
        Serial.println("No token — open: http://" + WiFi.localIP().toString());
        server.on("/", handleRoot);
        server.on("/callback", handleCallback);
        server.begin();
    }
}

void loop() {
    if (!authDone) {
        server.handleClient();
        return;
    }
    bool isPressed = (digitalRead(BUTTON_PIN) == LOW);
    if (isPressed && !wasPressed) {
        Serial.println("Button — toggling Spotify");
        togglePlayback();
    }
    wasPressed = isPressed;
    delay(50);
}
