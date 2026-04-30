#include <WiFi.h>
#include <Arduino.h>
#include <SpotifyEsp32.h>
#include <HTTPClient.h> 

// WiFi
const char* ssid = "";
const char* password = "";

// Spotify
const char* client_id = "";
const char* client_secret = "";

//const char* refresh_token = "REFRESH_TOKEN_שלך";
// Spotify spotify(client_id, client_secret, refresh_token);

Spotify spotify(client_id, client_secret);

const int buttonPin = 2; 
bool wasPressed = false;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);  
  wifiConnection();  
  spotify.begin();
  Serial.println("Go to this address to connect: ");
  while (!spotify.is_auth()) {
    spotify.handle_client();
    delay(100);
  }
  Serial.print("Save this token: ");
  Serial.println(spotify.get_user_tokens().refresh_token);
}

void loop() {
  bool isPressed = (digitalRead(buttonPin) == LOW);
  if (isPressed && !wasPressed) {
    Serial.println("Button Pressed");
    playStopPlaylist();
  }
  wasPressed = isPressed;
  delay(50);
}

void wifiConnection() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi Connected");
}

void playStopPlaylist() {
  response res = spotify.get_current_playing_track();
  if (res.status_code == 200) {
    Serial.print("Response: ");
    Serial.println(res.status_code);
    bool playing = res.reply["is_playing"];
    
    if (playing) {
      Serial.println("Pause playing ");
      spotify.pause_playback();
    } else {
      Serial.println("Continue playing ");
      spotify.start_a_users_playback();
    }
  } else {
    Serial.println("Start playing ");
    playPlaylist("spotify:playlist:YOUR_PLAYLIST_ID");  
  }
}

void playPlaylist(const char* playlist_uri) {
  if (!spotify.is_auth()) {
    Serial.println("User not connected");
    return;
  }
  HTTPClient http;
  http.begin("https://api.spotify.com/v1/me/player/play");
  String auth = "Bearer " + String(spotify.get_user_tokens().refresh_token);
  http.addHeader("Authorization", auth);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"context_uri\":\"" + String(playlist_uri) + "\"}";
  int httpCode = http.PUT(body);
  Serial.println(httpCode);
  
  if (httpCode == 200 || httpCode == 204) {
    Serial.println("Start Playlist after HTTP call");
  } else {
    Serial.println("Error ");
    Serial.println(httpCode);
  }
  http.end();
}