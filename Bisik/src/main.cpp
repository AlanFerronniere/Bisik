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

// Broches
#define OLED_SDA 33
#define OLED_SCL 25
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SERVO_PIN 12
#define DFPLAYER_RX 16
#define DFPLAYER_TX 17

constexpr int DEFAULT_VOLUME = 22; // Volume DFPlayer (0-30)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient espClient;
PubSubClient client(espClient);
Servo myservo;
HardwareSerial dfSerial(2);
DFRobotDFPlayerMini dfPlayer;
bool affichageInitialise = false;

// État d'animation au repos
bool estRepos = true;
unsigned long dernierMouvementRepos = 0;
unsigned long dernierClignotement = 0;
int decalageIrisX = 0;  // -4 à +4
int decalageIrisY = 0;  // -4 à +4
int phaseClignotement = 0;   // 0=ouvert, 1=fermeture, 2=fermé, 3=ouverture
int progressionClignotement = 0; // 0-100 pour animation fluide

// Déclarations anticipées
void afficherNormalise(const char* texte);

void dessinerOeilChat() {
  
  // Dessiner l'oeil externe (blanc) - très grand
  display.fillCircle(64, 32, 26, SSD1306_WHITE);
  
  // Dessiner l'iris (noir) avec décalage - rond large
  int irisX = 64 + decalageIrisX;
  int irisY = 32 + decalageIrisY;
  display.fillCircle(irisX, irisY, 14, SSD1306_BLACK);
  
  // Dessiner animation de clignotement (paupière supérieure - plein écran)
  if (phaseClignotement > 0) {
    int positionPaupiereY = (progressionClignotement * 64 / 100); // Descendre en fermant (0-64px plein écran)
    display.fillRect(0, 0, 128, positionPaupiereY, SSD1306_BLACK);
  }
}

void animerOeilRepos() {
  unsigned long maintenant = millis();
  
  // Déplacer l'iris tous les 500ms
  if (maintenant - dernierMouvementRepos > 500) {
    dernierMouvementRepos = maintenant;
    
    // Petits mouvements aléatoires dans une plage
    decalageIrisX = random(-5, 6);
    decalageIrisY = random(-4, 5);
    
    // Clignotement occasionnel (20% de chance à chaque mouvement, tous les 3-5 secondes)
    if (random(100) < 20 && phaseClignotement == 0) {
      phaseClignotement = 1;
      dernierClignotement = maintenant;
      progressionClignotement = 0;
    }
  }
  
  // Machine d'état pour animation de clignotement
  if (phaseClignotement == 1) {
    // Phase de fermeture (150ms)
    progressionClignotement = min(100, (int)((maintenant - dernierClignotement) / 1.5));
    if (progressionClignotement >= 100) {
      phaseClignotement = 2;
      dernierClignotement = maintenant;
    }
  } else if (phaseClignotement == 2) {
    // Phase fermé (100ms)
    progressionClignotement = 100;
    if ((maintenant - dernierClignotement) > 100) {
      phaseClignotement = 3;
      dernierClignotement = maintenant;
    }
  } else if (phaseClignotement == 3) {
    // Phase d'ouverture (150ms)
    progressionClignotement = max(0, (int)(100 - (maintenant - dernierClignotement) / 1.5));
    if (progressionClignotement <= 0) {
      phaseClignotement = 0;
      progressionClignotement = 0;
    }
  }
  
  // Redessiner l'écran
  display.clearDisplay();
  dessinerOeilChat();
  display.display();
}

void afficherEtat(const char* ligne1, const char* ligne2 = "") {
  if (!affichageInitialise) return;
  display.clearDisplay();
  display.setCursor(0, 0);
  
  // Normaliser et afficher le texte directement
  afficherNormalise(ligne1);
  
  if (ligne2 && ligne2[0] != '\0') {
    display.println();
    afficherNormalise(ligne2);
  }
  display.display();
}

// Normalise et affiche directement les caractères UTF-8 accentués en ASCII
void afficherNormalise(const char* texte) {
  if (!affichageInitialise || !texte) return;
  
  int i = 0;
  while (texte[i] != '\0') {
    uint8_t c = (uint8_t)texte[i];
    
    // Traiter les caractères UTF-8 multi-octets
    if (c >= 0xC0 && (i + 1) < 512) {
      uint8_t c1 = c;
      uint8_t c2 = (uint8_t)texte[i + 1];
      
      char replacement = 0;
      
      // ç (c cédille): C3 A7
      if (c1 == 0xC3 && c2 == 0xA7) replacement = 'c';
      // Ç: C3 87
      else if (c1 == 0xC3 && c2 == 0x87) replacement = 'C';
      // à, â, ä: C3 A0, C3 A2, C3 A4
      else if (c1 == 0xC3 && (c2 == 0xA0 || c2 == 0xA2 || c2 == 0xA4)) replacement = 'a';
      // À, Â, Ä: C3 80, C3 82, C3 84
      else if (c1 == 0xC3 && (c2 == 0x80 || c2 == 0x82 || c2 == 0x84)) replacement = 'A';
      // é, è, ê, ë: C3 A9, C3 A8, C3 AA, C3 AB
      else if (c1 == 0xC3 && (c2 == 0xA9 || c2 == 0xA8 || c2 == 0xAA || c2 == 0xAB)) replacement = 'e';
      // É, È, Ê, Ë: C3 89, C3 88, C3 8A, C3 8B
      else if (c1 == 0xC3 && (c2 == 0x89 || c2 == 0x88 || c2 == 0x8A || c2 == 0x8B)) replacement = 'E';
      // î, ï, ì, í: C3 AE, C3 AF, C3 AC, C3 AD
      else if (c1 == 0xC3 && (c2 == 0xAE || c2 == 0xAF || c2 == 0xAC || c2 == 0xAD)) replacement = 'i';
      // Î, Ï, Ì, Í: C3 8E, C3 8F, C3 8C, C3 8D
      else if (c1 == 0xC3 && (c2 == 0x8E || c2 == 0x8F || c2 == 0x8C || c2 == 0x8D)) replacement = 'I';
      // ô, ö, ò, ó: C3 B4, C3 B6, C3 B2, C3 B3
      else if (c1 == 0xC3 && (c2 == 0xB4 || c2 == 0xB6 || c2 == 0xB2 || c2 == 0xB3)) replacement = 'o';
      // Ô, Ö, Ò, Ó: C3 94, C3 96, C3 92, C3 93
      else if (c1 == 0xC3 && (c2 == 0x94 || c2 == 0x96 || c2 == 0x92 || c2 == 0x93)) replacement = 'O';
      // û, ü, ù, ú: C3 BB, C3 BC, C3 B9, C3 BA
      else if (c1 == 0xC3 && (c2 == 0xBB || c2 == 0xBC || c2 == 0xB9 || c2 == 0xBA)) replacement = 'u';
      // Û, Ü, Ù, Ú: C3 9B, C3 9C, C3 99, C3 9A
      else if (c1 == 0xC3 && (c2 == 0x9B || c2 == 0x9C || c2 == 0x99 || c2 == 0x9A)) replacement = 'U';
      // ñ: C3 B1
      else if (c1 == 0xC3 && c2 == 0xB1) replacement = 'n';
      // Ñ: C3 91
      else if (c1 == 0xC3 && c2 == 0x91) replacement = 'N';
      
      if (replacement) {
        display.write(replacement);
        i += 2;
      } else {
        // Caractère UTF-8 non reconnu, sauter
        i += 2;
      }
      continue;
    }
    
    // Caractères ASCII normaux
    display.write((char)c);
    i++;
  }
}


void journaliserEvenementsDFPlayer() {
  while (dfPlayer.available()) {
    uint8_t type = dfPlayer.readType();
    int valeur = dfPlayer.read();
    if (type == DFPlayerPlayFinished) {
      Serial.printf("Piste %d terminée\n", valeur);
    } else if (type == DFPlayerError) {
      Serial.printf("Erreur DFPlayer %d\n", valeur);
    } else if (type == DFPlayerCardRemoved) {
      Serial.println("Carte SD retirée");
    } else if (type == DFPlayerCardInserted) {
      Serial.println("Carte SD insérée");
    } else if (type == DFPlayerCardOnline) {
      Serial.println("Carte SD en ligne");
    }
  }
}

void jouerSon(int piste, int volume) {
  if (piste < 1) {
    Serial.println("Piste invalide");
    return;
  }

  if (volume >= 0 && volume <= 30) {
    dfPlayer.volume(volume);
  }

  Serial.printf("Lecture de la piste %d à volume %d\n", piste, volume);
  dfPlayer.play(piste);
  delay(300);

  // Attendre jusqu'à ~15s pour la fin, quitter tôt en cas d'événement
  unsigned long debut = millis();
  while (millis() - debut < 15000) {
    if (dfPlayer.available()) {
      uint8_t type = dfPlayer.readType();
      int valeur = dfPlayer.read();
      if (type == DFPlayerPlayFinished) {
        Serial.printf("Piste %d terminée\n", valeur);
        break;
      }
      if (type == DFPlayerError) {
        Serial.printf("Erreur DFPlayer %d\n", valeur);
        break;
      }
    }
    delay(80);
  }
}

void executerChoreo(String chargeUtileJson) {
  estRepos = false;  // Marquer comme non au repos
  
  DynamicJsonDocument doc(4096);
  DeserializationError erreur = deserializeJson(doc, chargeUtileJson);

  if (erreur) {
    Serial.print(F("Erreur deserializeJson() : "));
    Serial.println(erreur.f_str());
    estRepos = true;  // Reprendre le repos en cas d'erreur d'analyse
    return;
  }

  JsonArray actions = doc.as<JsonArray>();
  for (JsonObject action : actions) {
    const char* type = action["type"];

    if (strcmp(type, "display") == 0) {
      display.clearDisplay();
      display.setCursor(0, 0);
      const char* texte = action["text"].as<const char*>();
      afficherNormalise(texte);
      display.display();
      if (action.containsKey("duration")) {
        delay(action["duration"].as<int>());
      }

    } else if (strcmp(type, "servo") == 0) {
      int angle = action["angle"];
      int vitesse = action["speed"]; // délai après la commande
      myservo.attach(SERVO_PIN);
      delay(10);
      myservo.write(angle);
      delay(vitesse);
      myservo.detach();

    } else if (strcmp(type, "sound") == 0) {
      int piste = action["track"];
      int volume = action.containsKey("volume") ? action["volume"].as<int>() : DEFAULT_VOLUME;
      jouerSon(piste, volume);

    } else if (strcmp(type, "wait") == 0) {
      delay(action["duration"].as<int>());
    } else {
      Serial.print("Type d'action inconnu: ");
      Serial.println(type);
    }
  }

  display.clearDisplay();
  display.display();
  
  estRepos = true;  // Reprendre le repos après la fin de la chorégraphie
}

void rappel(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Chorégraphie reçue");
  Serial.println(message);
  executerChoreo(message);
}

void connecterWiFi() {
  Serial.print("Connexion WiFi");
  afficherEtat("Connexion WiFi...");
  WiFi.begin(ssid, password);
  
  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED && tentatives < 40) {
    delay(500);
    Serial.print(".");
    tentatives++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connecté");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" timeout");
    afficherEtat("WiFi timeout", "Redemarrage...");
    delay(2000);
    ESP.restart();
  }
}

void reconnecter() {
  while (!client.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    String idClient = "Bisik-" + String(random(0xffff), HEX);
    if (client.connect(idClient.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connecté");
      client.subscribe(mqtt_topic);
      afficherEtat("MQTT OK", mqtt_topic);
    } else {
      Serial.print("échec, rc=");
      Serial.print(client.state());
      String err = "rc=" + String(client.state());
      afficherEtat("MQTT fail", err.c_str());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Laisser le temps aux composants de s'initialiser

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Initialisation SSD1306 échouée");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Bisik démarrage...");
    display.display();
    affichageInitialise = true;
  }

  afficherEtat("Init DFPlayer...");
  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  delay(500); // Délai important pour le DFPlayer
  
  if (!dfPlayer.begin(dfSerial, false, false)) { // Pas d'ACK pour éviter les blocages
    Serial.println("Initialisation DFPlayer échouée: vérifier le câblage/SD");
    afficherEtat("DFPlayer NOK", "Continuer...");
    delay(1000);
  } else {
    dfPlayer.volume(DEFAULT_VOLUME);
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
    Serial.printf("DFPlayer prêt, volume %d\n", DEFAULT_VOLUME);
    afficherEtat("DFPlayer OK");
    delay(500);
  }

  connecterWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(rappel);
  client.setBufferSize(2048);

  afficherEtat("WiFi OK", "En attente MQTT");
}

void loop() {
  if (!client.connected()) {
    reconnecter();
  }
  client.loop();
  journaliserEvenementsDFPlayer();
  
  // Afficher l'animation de repos quand la chorégraphie n'est pas en cours
  if (estRepos) {
    animerOeilRepos();
  }
  
  delay(10);
}
