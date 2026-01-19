# Bisik - Robot de notification avec ESP32

Le but du projet est de construire un petit robot de notification, capable d'émettre des sons, de bouger une patte et d'afficher quelques infos sur un petit écran LCD

## Description générale

Le robot est basé sur un ESP32, qui contrôle un servomoteur pour faire bouger un bras, un lecteur MP3 DFPlayer Mini connecté à un haut-parleur 8Ω pour émettre des sons, et un écran OLED pour afficher des informations.
Le robot va servir pour notifier l'utilisateur de certains événements (comme la réception d'un email ou d'un message....) en bougeant le bras, en jouant un son MP3 et en affichant un message sur l'écran. Le "cerveau" du robot sera une application web, écrite en PHP, hébergée sur un serveur local (Raspberry Pi par exemple) qui enverra des notifications à l'ESP32 via une connexion WiFi, par l'intermédiaire d'un serveur MQTT (mosquito)

![[Pasted image 20251208164209.png]]

Le but du projet est de construire l'architecture, d'en profiter pour apprendre à utiliser l'ESP32, le MQTT, et de se familiariser avec la programmation de robots simples, avec le C et d'améliorer vos connaissances en PHP. 
Le projet sera aussi un prétexte pour découvrir la gestion du versioning avec Git et GitHub (ou équivalent), et de documenter le projet avec du Markdown.

Et j'espère de faire un joli robot à la fin :) facile à reproduire chez soi.

## Robot

Le code du robot sera en C, compilé avec PlateformIO (extension VSCode) https://platformio.org/
Le robot se connectera au réseau WiFi local, puis au serveur MQTT, et s'abonnera au topic "bisik/henry". Lorsqu'un message est publié sur ce topic, le robot réagira en fonction du contenu du message (par exemple, bouger le bras, jouer un fichier MP3, afficher un message sur l'écran).

Le DFPlayer Mini communique avec l'ESP32 via UART (Serial) et lit les fichiers audio depuis une carte micro-SD formatée en FAT32. Les fichiers MP3 doivent être numérotés séquentiellement (ex: `0001.mp3`, `0002.mp3`, etc.) pour être adressables par index.

## Configuration firmware DFPlayer

### Initialisation

Le DFPlayer Mini communique avec l'ESP32 via UART (Serial2 sur ESP32) :

**Connexions matérielles :**
- TX DFPlayer → GPIO 16 (RX2 ESP32)
- RX DFPlayer → GPIO 17 (TX2 ESP32)
- VCC → 5V
- GND → GND

**Bibliothèque requise :**
```cpp
#include <DFRobotDFPlayerMini.h>

DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  
  if (!myDFPlayer.begin(Serial2)) {
    Serial.println("Erreur DFPlayer: carte SD, câblage ou fichiers manquants");
    // Gérer l'erreur (LED d'erreur, affichage OLED, etc.)
  }
  
  myDFPlayer.volume(20); // Volume 0-30
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
}
```

**Configuration PlatformIO :**
Ajouter dans `platformio.ini` :
```ini
lib_deps = 
    DFRobotDFPlayerMini
    adafruit/Adafruit GFX Library
    adafruit/Adafruit SSD1306
    bblanchon/ArduinoJson
    ESP32Servo
    PubSubClient
```

### Exécution de l'action "sound"

**Parsing et lecture :**
```cpp
void playSound(int track, int volume = 20) {
  if (track < 1 || track > 255) {
    Serial.println("Numéro de piste invalide");
    return;
  }
  
  // Ajuster le volume (0-30)
  if (volume >= 0 && volume <= 30) {
    myDFPlayer.volume(volume);
  }
  
  Serial.print("Playing track: ");
  Serial.print(track);
  Serial.print(" at volume: ");
  Serial.println(volume);
  myDFPlayer.play(track);
  
  // Mode bloquant : attendre la fin de lecture
  delay(500); // Délai pour que le DFPlayer démarre
  
  while (myDFPlayer.readState() != -1) {
    delay(100);
    if (myDFPlayer.available()) {
      uint8_t type = myDFPlayer.readType();
      if (type == DFPlayerPlayFinished) {
        Serial.println("Playback finished");
        break;
      }
    }
  }
}
```

Dans la fonction `executeChoreo()` :
```cpp
else if (strcmp(type, "sound") == 0) {
  int track = action["track"];
  int volume = action.containsKey("volume") ? action["volume"].as<int>() : 20;
  playSound(track, volume);
}
```

**Gestion d'erreurs :**
```cpp
void checkDFPlayerStatus() {
  if (myDFPlayer.available()) {
    uint8_t type = myDFPlayer.readType();
    
    switch (type) {
      case DFPlayerCardRemoved:
        Serial.println("Erreur: carte SD retirée");
        displayError("SD manquante");
        break;
      case DFPlayerError:
        Serial.println("Erreur DFPlayer");
        break;
      case DFPlayerPlayFinished:
        Serial.println("Lecture terminée");
        break;
    }
  }
}
```

### Synchronisation des fichiers MP3

**Étapes de déploiement :**
1. Les fichiers MP3 sont placés dans `Appli/sons/` sur le serveur web
2. Créer un dossier `/mp3/` à la racine de la carte micro-SD
3. Copier et renommer les fichiers en ordre alphabétique :
   ```
   ANMLCat_Miaulement_01.mp3 → /mp3/0001.mp3
   ANMLCat_Miaulement_02.mp3 → /mp3/0002.mp3
   ROBTVox_Robot_Notif_01.mp3 → /mp3/0003.mp3
   ...
   ```
4. L'application génère automatiquement `Appli/sons/mapping.json` :
   ```json
   {
     "0001": "ANMLCat_Miaulement_01.mp3",
     "0002": "ANMLCat_Miaulement_02.mp3",
     "0003": "ROBTVox_Robot_Notif_01.mp3"
   }
   ```

**Script de synchronisation suggéré :**
```bash
#!/bin/bash
# sync_sounds.sh
cd Appli/sons
i=1
for file in *.mp3; do
  printf -v padded "%04d" $i
  cp "$file" /media/sdcard/mp3/${padded}.mp3
  echo "\"$padded\": \"$file\"," >> mapping.json
  ((i++))
done
```

### Contraintes et limitations

- **Temps de réponse** : délai ~200-500ms entre commande et début de lecture (dépend de la carte SD)
- **Taille SD** : maximum 32GB, formatage FAT32 obligatoire
- **Concurrent actions** : la lecture MP3 peut se superposer aux mouvements servo/affichage si implémentée en mode non-bloquant
- **Volume** : réglable via `myDFPlayer.volume(0-30)`, peut être exposé dans l'interface web
- **Formats supportés** : MP3, WAV (MP3 recommandé pour taille/qualité)
- **Nombre de fichiers** : jusqu'à 255 par dossier (limitation DFPlayer)

### Debugging

**Vérifications sur le Serial Monitor (115200 baud) :**
```
Initialisation DFPlayer...
DFPlayer Mini online.
Volume réglé à 20
Carte SD détectée: 2048 MB
Fichiers disponibles: 20
Playing track 5...
Lecture terminée.
```

**Codes d'erreur DFPlayer :**
- Pas de réponse → vérifier câblage TX/RX
- "Carte SD manquante" → reformater en FAT32, vérifier contacts
- "Fichier introuvable" → vérifier numérotation `/mp3/0001.mp3`
- Grésillements → alimentation insuffisante, ajouter condensateur 100µF

## Application web

L'application web sera écrite en PHP, et permettra d'envoyer des notifications au robot. L'application se connectera au serveur MQTT et publiera des messages sur le topic "bisik/henry" lorsque l'utilisateur souhaite envoyer une notification.

L'idée de l'application est double :
- une interface de **gestion de choregraphies** de notification (par exemple, pour un email, faire bouger le bras 3 fois, jouer un son, afficher "Nouveau mail" sur l'écran)
- des webhooks pour permettre à d'autres applications (comme un serveur de messagerie) d'envoyer des notifications automatiquement.

### Gestion des chorégraphie

La gestion des chorégraphie doit permettre d'ajouter, modifier, supprimer des chorégraphies de notification. Chaque chorégraphie sera composée d'une série d'actions (bouger le bras, jouer un son MP3, afficher un message) avec des paramètres spécifiques.

#### Sons MP3

L'outil doit permettre de sélectionner un fichier MP3 à jouer parmi une liste déroulante. Cette liste est alimentée dynamiquement par les fichiers présents dans le dossier `Appli/sons/` de l'application web.

**Fonctionnalités :**
- Liste déroulante affichant tous les fichiers `.mp3` disponibles dans `Appli/sons/`
- Bouton de pré-écoute permettant d'écouter le son directement depuis l'interface web avant de l'ajouter à la chorégraphie
- Mapping automatique entre les noms de fichiers et les numéros de piste DFPlayer (ordre alphabétique → index 0001, 0002, etc.)
- **Slider de volume** (0-30) pour contrôler le niveau sonore de chaque action

**Format de l'action dans la chorégraphie :**
```json
{
  "type": "sound",
  "track": 5,
  "file": "ROBTVox_Robot_Notification_03.mp3",
  "volume": 20
}
```
- `track` : numéro de piste DFPlayer (1-255), calculé automatiquement par l'application
- `file` : nom du fichier MP3 source (pour référence et débogage)
- `volume` : niveau de volume (0-30, optionnel, défaut=20)

**Contrôle du volume dans l'interface :**
- Slider interactif affichant la valeur en temps réel (0=silencieux, 30=maximum)
- Valeur par défaut : 20 (niveau moyen recommandé)
- Le volume est ajusté avant chaque lecture du DFPlayer

**Exigences techniques :**
- Les fichiers MP3 dans `Appli/sons/` doivent être copiés sur la carte micro-SD du DFPlayer en respectant la numérotation séquentielle (`/mp3/0001.mp3`, `/mp3/0002.mp3`, etc.)
- Format audio recommandé : MP3, 44.1kHz ou 22.05kHz, bitrate 128kbps max
- Taille maximale par fichier : 32MB (limitation DFPlayer)
- L'application web génère un fichier de mapping (ex: `sons/mapping.json`) pour la correspondance fichier → numéro de piste
- Le paramètre `volume` est optionnel ; si omis, le volume par défaut (20) sera utilisé

#### Mouvement du bras

L'outil doit permettre de définir une série de positions pour le servomoteur, avec des durées pour chaque position.

#### Messages à afficher

L'outil doit permettre de définir des messages à afficher sur l'écran OLED, avec la possibilité de définir la durée d'affichage. Une partie du message pourra être dynamique (par exemple, afficher l'objet d'un email reçu) en étant remplacée par une variable envoyée par le webhook.

### Schéma des actions de chorégraphie

Chaque chorégraphie est stockée sous forme de tableau JSON d'actions séquentielles. Les actions sont exécutées l'une après l'autre par le firmware ESP32.

**Actions disponibles :**

1. **`sound`** - Jouer un fichier MP3
   ```json
   {
     "type": "sound",
     "track": 5,
     "file": "ROBTVox_Robot_Notification_03.mp3",
     "volume": 20
   }
   ```
   - `track` : numéro de piste DFPlayer (1-255)
   - `file` : nom du fichier source (optionnel, pour référence)
   - `volume` : niveau de volume (0-30, optionnel, défaut=20)

2. **`servo`** - Mouvement du servomoteur
   ```json
   {
     "type": "servo",
     "angle": 90,
     "speed": 500
   }
   ```
   - `angle` : position cible (0-180°)
   - `speed` : durée du mouvement en millisecondes

3. **`display`** - Affichage sur écran OLED
   ```json
   {
     "type": "display",
     "text": "Nouveau mail: {param}",
     "duration": 3000
   }
   ```
   - `text` : message à afficher (peut contenir `{param}` pour substitution dynamique)
   - `duration` : durée d'affichage en millisecondes (optionnel)

4. **`wait`** - Pause
   ```json
   {
     "type": "wait",
     "duration": 1000
   }
   ```
   - `duration` : durée de la pause en millisecondes

**Exemple de chorégraphie complète :**
```json
[
  {"type": "display", "text": "Notification!", "duration": 2000},
  {"type": "sound", "track": 3, "file": "notification.mp3", "volume": 25},
  {"type": "servo", "angle": 45, "speed": 300},
  {"type": "wait", "duration": 500},
  {"type": "servo", "angle": 135, "speed": 300}
]
```

### Webhooks

L'application web doit permettre de définir des webhooks pour chaque chorégraphie. Un webhook est une URL spécifique qui, lorsqu'elle est appelée (par exemple, par un serveur de messagerie), déclenche l'exécution de la chorégraphie associée.
Chaque webhook doit pouvoir recevoir des paramètres (par exemple, l'objet d'un email) qui seront utilisés dans la chorégraphie (par exemple, pour afficher l'objet sur l'écran OLED).

**Format de l'API webhook :**
- **Méthode** : GET
- **URL** : `https://bisik.bellocq.local/webhook.php?id={choreography_id}&param={dynamic_text}`
- **Paramètres** :
  - `id` (obligatoire) : identifiant de la chorégraphie à exécuter
  - `param` (optionnel) : texte à substituer dans les actions `display` contenant `{param}`
  - `test` (optionnel) : mode test (actuellement non utilisé)

**Réponse :**
```json
{
  "status": "success",
  "message": "Notification envoyée"
}
```

**Comportement :**
1. Charge la chorégraphie depuis la base de données MySQL
2. Substitue `{param}` dans les champs `text` des actions `display`
3. Publie le tableau JSON d'actions sur le topic MQTT `bisik/henry`
4. Le firmware ESP32 reçoit et exécute les actions séquentiellement

**Codes d'erreur :**
- 400 : paramètre `id` manquant
- 404 : chorégraphie introuvable
- 500 : erreur serveur (DB, MQTT)

### Exposition de l'interface web

L'interface web est hébergée sur un serveur local https://bisik.bellocq.local
Le projet PHP présent dans le dossier est automatiquement synchronisé sur le serveur local via rsync (rien à faire).
Le serveur MQTT utilisé est mqtt.latetedanslatoile.fr:1883 (auth: Epsi / EpsiWis2018!).

## Matériel utilisé

- **ESP32** (DevKit v1 ou équivalent)
- **Écran OLED 128 x 64 Pixel I2C** (contrôleur SSD1306)
  - SCL sur D25
  - SDA sur D33
- **Servomoteur SG90** (signal PWM sur D12)
- **DFPlayer Mini** (lecteur MP3)
  - TX DFPlayer → RX ESP32 (D16)
  - RX DFPlayer → TX ESP32 (D17)
  - Alimentation : 5V (partage avec ESP32 ou alimentation séparée recommandée)
- **Haut-parleur 8Ω 0.5W** (connecté aux sorties SPK_1 et SPK_2 du DFPlayer)
- **Carte micro-SD** (formatée FAT32, max 32GB)
  - Contient les fichiers MP3 numérotés dans le dossier `/mp3/`

## Wifi

SSID : la_tete_dans_la_toile
Mot de passe : concoulcabis

## Base de données 

host  : localhost
user : bisik
database : bisik
password : Bsk32Bsk32#

### Structure de la table `choreographies`

```sql
CREATE TABLE choreographies (
  id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(255) NOT NULL,
  description TEXT,
  actions JSON NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

Le champ `actions` contient un tableau JSON avec les actions de la chorégraphie.

**Exemple d'enregistrement :**
```json
{
  "id": 1,
  "name": "Notification email",
  "description": "Alerte pour nouveau mail important",
  "actions": [
    {"type": "sound", "track": 5, "file": "notification.mp3", "volume": 20},
    {"type": "display", "text": "Mail: {param}", "duration": 3000},
    {"type": "servo", "angle": 90, "speed": 500}
  ],
  "created_at": "2026-01-15 10:30:00"
}
```
