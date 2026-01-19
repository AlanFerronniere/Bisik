#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// WiFi Credentials
const char* ssid = "la_tete_dans_la_toile";
const char* password = "concoulcabis";

// MQTT Broker
const char* mqtt_server = "192.168.0.25";
const int mqtt_port = 1883;

// Pins
#define OLED_SDA 33
#define OLED_SCL 25
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SERVO_PIN 12
#define BUZZER_PIN 27

// LEDC setup for tone generation
const int buzzerChannel = 0; // Use LEDC channel 0
const int buzzerResolution = 10; // 10-bit resolution
const int buzzerFreq = 5000; // Base frequency (not actual note, just for LEDC setup)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo;

// Notes mapping (Basic Octave 2 to 5)
int getNoteFreq(String noteName) {
  if (noteName == "C2") return 65;
  if (noteName == "D2") return 73;
  if (noteName == "E2") return 82;
  if (noteName == "F2") return 87;
  if (noteName == "G2") return 98;
  if (noteName == "A2") return 110;
  if (noteName == "B2") return 123;
  if (noteName == "C3") return 131;
  if (noteName == "D3") return 147;
  if (noteName == "E3") return 165;
  if (noteName == "F3") return 175;
  if (noteName == "G3") return 196;
  if (noteName == "A3") return 220;
  if (noteName == "B3") return 247;
  if (noteName == "C4") return 262;
  if (noteName == "D4") return 294;
  if (noteName == "E4") return 330;
  if (noteName == "F4") return 349;
  if (noteName == "G4") return 392;
  if (noteName == "A4") return 440;
  if (noteName == "B4") return 494;
  if (noteName == "C5") return 523;
  if (noteName == "D5") return 587;
  if (noteName == "E5") return 659;
  if (noteName == "F5") return 698;
  if (noteName == "G5") return 784;
  if (noteName == "A5") return 880;
  if (noteName == "B5") return 988;
  return 0; // Invalid note
}

void playMelody(String notesStr, String durationsStr) {
  char notesCopy[100];
  char durationsCopy[100];
  notesStr.toCharArray(notesCopy, 100);
  durationsStr.toCharArray(durationsCopy, 100);

  char* note = strtok(notesCopy, ",");
  char* duration = strtok(durationsCopy, ",");

  while (note != NULL && duration != NULL) {
    int freq = getNoteFreq(note);
    int dur = atoi(duration);
    
    int noteDuration = 1000 / dur; // Duration in ms

    if (freq > 0) {
      ledcWriteTone(buzzerChannel, freq); // Play tone using LEDC
      delay(noteDuration * 1.30); // Note duration + small pause
      ledcWriteTone(buzzerChannel, 0); // Stop tone
    } else {
      delay(noteDuration * 1.30); // Just pause for silence
    }

    note = strtok(NULL, ",");
    duration = strtok(NULL, ",");
  }
}

void executeChoreo(String jsonPayload) {
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, jsonPayload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
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
      if(action.containsKey("duration")) {
        delay(action["duration"].as<int>());
      }

    } else if (strcmp(type, "servo") == 0) {
      int angle = action["angle"];
      int speed = action["speed"]; // Simple delay for speed simulation
      myservo.attach(SERVO_PIN); // Attach the servo
      delay(10); // Short delay to ensure attachment is complete
      myservo.write(angle);
      delay(speed); // Wait for movement
      myservo.detach(); // Detach the servo to stop signals and vibration

    } else if (strcmp(type, "melody") == 0) {
      playMelody(action["notes"].as<String>(), action["durations"].as<String>());

    } else if (strcmp(type, "wait") == 0) {
      delay(action["duration"].as<int>());
    }
  }
  
  // Reset Display after choreo
  display.clearDisplay();
  display.display();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Choreo received");
  executeChoreo(message);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "BisikClient-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("bisik/notification");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // LEDC setup for buzzer
  ledcSetup(buzzerChannel, buzzerFreq, buzzerResolution);
  ledcAttachPin(BUZZER_PIN, buzzerChannel);

  // Hardware Setup (OLED, Servo)
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  myservo.attach(SERVO_PIN); // Attach here, but detach after use in executeChoreo

  // WiFi Setup
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}