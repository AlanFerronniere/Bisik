#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>

// WiFi
const char* ssid = "la_tete_dans_la_toile";
const char* password = "concoulcabis";

// MQTT
const char* mqtt_server = "mqtt.latetedanslatoile.fr";
const int mqtt_port = 1883;
const char* mqtt_user = "Epsi";
const char* mqtt_pass = "EpsiWis2018!";
const char* mqtt_topic = "bisik/henry";

// Pins
#define OLED_SDA 33
#define OLED_SCL 25
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SERVO_PIN 12
#define DFPLAYER_RX 16
#define DFPLAYER_TX 17

constexpr int DEFAULT_VOLUME = 22; // DFPlayer volume (0-30)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo;
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini dfPlayer;
bool displayReady = false;

// Idle animation state
bool isIdle = true;
unsigned long lastIdleMove = 0;
unsigned long lastBlink = 0;
int irisOffsetX = 0;  // -4 to +4
int irisOffsetY = 0;  // -4 to +4
int blinkPhase = 0;   // 0=open, 1=closing, 2=closed, 3=opening
int blinkProgress = 0; // 0-100 for smooth animation

void drawCatEye() {
  if (!displayReady) return;
  
  // Draw outer eye (white) - very large
  display.fillCircle(64, 32, 26, SSD1306_WHITE);
  
  // Draw iris (black) with offset - large round
  int irisX = 64 + irisOffsetX;
  int irisY = 32 + irisOffsetY;
  display.fillCircle(irisX, irisY, 14, SSD1306_BLACK);
  
  // Draw blinking animation (top eyelid - fullscreen)
  if (blinkPhase > 0) {
    int eyelidY = (blinkProgress * 64 / 100); // Move down as we close (0-64px fullscreen)
    display.fillRect(0, 0, 128, eyelidY, SSD1306_BLACK);
  }
}

void animateIdleEye() {
  unsigned long now = millis();
  
  // Move iris every 500ms
  if (now - lastIdleMove > 500) {
    lastIdleMove = now;
    
    // Random small movements within a range
    irisOffsetX = random(-5, 6);
    irisOffsetY = random(-4, 5);
    
    // Occasional blink (20% chance per movement, every 3-5 seconds)
    if (random(100) < 20 && blinkPhase == 0) {
      blinkPhase = 1;
      lastBlink = now;
      blinkProgress = 0;
    }
  }
  
  // Blink animation state machine
  if (blinkPhase == 1) {
    // Closing phase (150ms)
    blinkProgress = min(100, (int)((now - lastBlink) / 1.5));
    if (blinkProgress >= 100) {
      blinkPhase = 2;
      lastBlink = now;
    }
  } else if (blinkPhase == 2) {
    // Closed phase (100ms)
    blinkProgress = 100;
    if ((now - lastBlink) > 100) {
      blinkPhase = 3;
      lastBlink = now;
    }
  } else if (blinkPhase == 3) {
    // Opening phase (150ms)
    blinkProgress = max(0, (int)(100 - (now - lastBlink) / 1.5));
    if (blinkProgress <= 0) {
      blinkPhase = 0;
      blinkProgress = 0;
    }
  }
  
  // Redraw screen
  display.clearDisplay();
  drawCatEye();
  display.display();
}

void showStatus(const char* line1, const char* line2 = "") {
  if (!displayReady) return;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(line1);
  if (line2 && line2[0] != '\0') {
    display.println(line2);
  }
  display.display();
}

void logDfPlayerEvents() {
  while (dfPlayer.available()) {
    uint8_t type = dfPlayer.readType();
    int value = dfPlayer.read();
    if (type == DFPlayerPlayFinished) {
      Serial.printf("Track %d finished\n", value);
    } else if (type == DFPlayerError) {
      Serial.printf("DFPlayer error %d\n", value);
    } else if (type == DFPlayerCardRemoved) {
      Serial.println("SD card removed");
    } else if (type == DFPlayerCardInserted) {
      Serial.println("SD card inserted");
    } else if (type == DFPlayerCardOnline) {
      Serial.println("SD card online");
    }
  }
}

void playSound(int track, int volume) {
  if (track < 1) {
    Serial.println("Invalid track");
    return;
  }

  if (volume >= 0 && volume <= 30) {
    dfPlayer.volume(volume);
  }

  Serial.printf("Playing track %d at volume %d\n", track, volume);
  dfPlayer.play(track);
  delay(300);

  // Wait up to ~15s for completion, exit early on event
  unsigned long start = millis();
  while (millis() - start < 15000) {
    if (dfPlayer.available()) {
      uint8_t type = dfPlayer.readType();
      int value = dfPlayer.read();
      if (type == DFPlayerPlayFinished) {
        Serial.printf("Track %d finished\n", value);
        break;
      }
      if (type == DFPlayerError) {
        Serial.printf("DFPlayer error %d\n", value);
        break;
      }
    }
    delay(80);
  }
}

void executeChoreo(String jsonPayload) {
  isIdle = false;  // Mark as no longer idle
  
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, jsonPayload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    isIdle = true;  // Resume idle if parse fails
    return;
  }

  JsonArray actions = doc.as<JsonArray>();
  for (JsonObject action : actions) {
    const char* type = action["type"];

    if (strcmp(type, "display") == 0) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(action["text"].as<const char*>());
      display.display();
      if (action.containsKey("duration")) {
        delay(action["duration"].as<int>());
      }

    } else if (strcmp(type, "servo") == 0) {
      int angle = action["angle"];
      int speed = action["speed"]; // delay after command
      myservo.attach(SERVO_PIN);
      delay(10);
      myservo.write(angle);
      delay(speed);
      myservo.detach();

    } else if (strcmp(type, "sound") == 0) {
      int track = action["track"];
      int volume = action.containsKey("volume") ? action["volume"].as<int>() : DEFAULT_VOLUME;
      playSound(track, volume);

    } else if (strcmp(type, "wait") == 0) {
      delay(action["duration"].as<int>());
    } else {
      Serial.print("Unknown action type: ");
      Serial.println(type);
    }
  }

  display.clearDisplay();
  display.display();
  
  isIdle = true;  // Resume idle after choreo completes
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Choreo received");
  Serial.println(message);
  executeChoreo(message);
}

void connectWiFi() {
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Bisik-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
      showStatus("MQTT OK", mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      String err = "rc=" + String(client.state());
      showStatus("MQTT fail", err.c_str());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Bisik boot...");
    display.display();
    displayReady = true;
  }

  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer init failed: check wiring/SD");
  } else {
    dfPlayer.volume(DEFAULT_VOLUME);
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
    Serial.printf("DFPlayer ready, volume %d\n", DEFAULT_VOLUME);
  }

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  client.setBufferSize(2048);

  showStatus("WiFi OK", "Waiting MQTT");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  logDfPlayerEvents();
  
  // Show idle animation when not executing choreo
  if (isIdle) {
    animateIdleEye();
  }
  
  delay(10);
}