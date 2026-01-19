# Guide de Migration - Buzzer vers DFPlayer MP3

## Résumé des changements

Cette migration remplace le système de mélodies basé sur buzzer LEDC par un lecteur MP3 DFPlayer Mini avec haut-parleur.

## Fichiers modifiés

### 1. Application PHP

#### **Appli/api_sounds.php** (NOUVEAU)
- API REST pour lister les fichiers MP3 du dossier `sons/`
- Génère automatiquement le mapping track → fichier
- Crée `sons/mapping.json` pour référence

#### **Appli/editor.php**
- ✅ Remplacement du bouton "Ajouter Mélodie" par "Ajouter Son MP3"
- ✅ Template `tpl-melody` → `tpl-sound` avec dropdown
- ✅ Ajout fonction `previewSound()` pour écouter les MP3
- ✅ Chargement dynamique via `api_sounds.php`
- ✅ Affichage du numéro de piste DFPlayer
- ✅ Conversion automatique fichier → track number

**Ancien format action:**
```json
{
  "type": "melody",
  "notes": "C4,D4,E4",
  "durations": "4,4,4"
}
```

**Nouveau format action:**
```json
{
  "type": "sound",
  "track": 5,
  "file": "ROBTVox_Robot_3.mp3"
}
```

### 2. Firmware ESP32

#### **Bisik/platformio.ini**
- ✅ Ajout dépendance: `DFRobotDFPlayerMini @ ^1.0.6`

#### **Bisik/src/main.cpp**
- ✅ Remplacement du système LEDC/buzzer par DFPlayer
- ✅ Pins: `DFPLAYER_RX=16`, `DFPLAYER_TX=17` (UART2)
- ✅ Suppression `playMelody()`, `getNoteFreq()`, LEDC setup
- ✅ Ajout `playSound(int track)` avec attente fin lecture
- ✅ Modification `executeChoreo()`: case `sound` au lieu de `melody`
- ✅ Setup DFPlayer avec vérification SD et volume initial
- ✅ Monitoring erreurs DFPlayer dans `loop()`

**Ancien code (buzzer):**
```cpp
#define BUZZER_PIN 27
const int buzzerChannel = 0;
ledcWriteTone(buzzerChannel, freq);
```

**Nouveau code (DFPlayer):**
```cpp
#define DFPLAYER_RX 16
#define DFPLAYER_TX 17
DFRobotDFPlayerMini myDFPlayer;
HardwareSerial dfSerial(2);
myDFPlayer.play(track);
```

## Étapes de déploiement

### Phase 1 : Préparation du matériel

1. **Déconnecter le buzzer**
   - Retirer le buzzer de la pin D27
   - Libérer le canal LEDC 0

2. **Câbler le DFPlayer Mini**
   ```
   DFPlayer TX  → ESP32 D16 (RX2)
   DFPlayer RX  → ESP32 D17 (TX2) + résistance 1kΩ
   DFPlayer VCC → 5V
   DFPlayer GND → GND
   DFPlayer SPK_1 → Haut-parleur +
   DFPlayer SPK_2 → Haut-parleur -
   ```

3. **Préparer la carte SD**
   ```bash
   # Formater en FAT32
   # Créer dossier /mp3/ à la racine
   # Copier et renommer les fichiers
   cd Appli/sons
   i=1
   for file in *.mp3; do
     printf -v padded "%04d" $i
     cp "$file" /path/to/sdcard/mp3/${padded}.mp3
     ((i++))
   done
   ```
   
   Résultat attendu sur la SD:
   ```
   /mp3/0001.mp3  (ANMLCat_Chat qui rale...)
   /mp3/0002.mp3  (ANMLCat_Feulement chat 1...)
   /mp3/0003.mp3  (ANMLCat_Grognement chat 2...)
   ...
   /mp3/0020.mp3  (ROBTVox_Robot 8...)
   ```

### Phase 2 : Mise à jour du code

1. **Synchroniser le code PHP**
   ```bash
   # Le rsync automatique devrait le faire
   # Vérifier que api_sounds.php et editor.php sont bien déployés
   ```

2. **Compiler et flasher le firmware**
   ```bash
   cd Bisik
   pio run
   pio run --target upload
   pio device monitor -b 115200
   ```

3. **Vérifier les logs de démarrage**
   ```
   Bisik Robot - Initialisation...
   Initializing DFPlayer...
   DFPlayer Mini online.
   Files on SD: 20
   Connecting to WiFi...
   WiFi connected
   IP: 192.168.0.XX
   Bisik Ready!
   ```

### Phase 3 : Test de l'interface web

1. **Ouvrir l'éditeur**
   ```
   https://bisik.bellocq.local/editor.php
   ```

2. **Vérifier le bouton "Ajouter Son MP3"**
   - Le dropdown doit afficher 20 sons
   - Icons 🐱 pour les sons de chat, 🤖 pour les sons robot

3. **Tester la prévisualisation**
   - Sélectionner un son
   - Cliquer "Écouter" → doit jouer depuis le navigateur
   - Vérifier que le numéro de piste s'affiche (ex: "Piste: #5")

4. **Créer une chorégraphie de test**
   ```json
   [
     {"type": "display", "text": "Test son!", "duration": 2000},
     {"type": "sound", "track": 1, "file": "..."},
     {"type": "wait", "duration": 500},
     {"type": "servo", "angle": 90, "speed": 500}
   ]
   ```

5. **Déclencher via webhook**
   ```bash
   curl "https://bisik.bellocq.local/webhook.php?id=1&param=test"
   ```

### Phase 4 : Validation complète

**Tests à effectuer:**

- [ ] **Test 1: Son seul**
  - Action: `{"type": "sound", "track": 5}`
  - Attendu: Le son #5 joue entièrement depuis le haut-parleur

- [ ] **Test 2: Son + Affichage**
  - Actions: `display` puis `sound`
  - Attendu: Message s'affiche, puis son joue

- [ ] **Test 3: Son + Servo**
  - Actions: `sound` puis `servo`
  - Attendu: Son joue complètement, puis servo bouge

- [ ] **Test 4: Séquence complète**
  - Actions: `display` → `sound` → `servo` → `wait` → `display`
  - Attendu: Exécution séquentielle sans interruption

- [ ] **Test 5: Erreur SD retirée**
  - Retirer la carte SD pendant le fonctionnement
  - Attendu: Log "SD Card removed!" dans Serial Monitor

- [ ] **Test 6: Track invalide**
  - Envoyer `{"type": "sound", "track": 999}`
  - Attendu: Log "Invalid track number"

- [ ] **Test 7: Chorégraphie existante**
  - Éditer une chorégraphie avec ancienne action `melody`
  - Attendu: Doit être convertie manuellement en `sound`

## Migration des chorégraphies existantes

Les chorégraphies avec actions `melody` doivent être migrées manuellement.

**Script SQL de migration (optionnel):**
```sql
-- Lister les chorégraphies avec melody
SELECT id, name, actions 
FROM choreographies 
WHERE JSON_CONTAINS(actions, '{"type":"melody"}');

-- Migration manuelle requise via l'interface web
-- Ouvrir chaque chorégraphie, supprimer l'action melody, 
-- ajouter une action sound équivalente
```

**Mapping suggestions:**
- Mélodie joyeuse → `track: 12` (Notification lasomarie 1)
- Mélodie alertante → `track: 16` (Robot 10)
- Mélodie douce → `track: 1` (Miaulement chat)

## Troubleshooting

### Problème : DFPlayer ne démarre pas

**Symptôme:**
```
Unable to begin DFPlayer:
1. Check SD card
```

**Solutions:**
1. Vérifier le formatage FAT32 de la SD (max 32GB)
2. Vérifier câblage TX/RX (croisé !)
3. Ajouter résistance 1kΩ sur RX DFPlayer
4. Vérifier alimentation 5V suffisante
5. Tester avec une autre carte SD

### Problème : Aucun son ne sort

**Symptôme:** DFPlayer online, mais haut-parleur silencieux

**Solutions:**
1. Vérifier connexions SPK_1/SPK_2
2. Augmenter le volume: `myDFPlayer.volume(25);`
3. Vérifier que les fichiers sont dans `/mp3/`
4. Tester un fichier connu: `myDFPlayer.play(1);`
5. Vérifier bitrate MP3 (max 320kbps)

### Problème : Sons coupés ou grésillements

**Solutions:**
1. Ajouter condensateur 100µF entre VCC/GND DFPlayer
2. Alimentation séparée 5V/1A pour DFPlayer
3. Réduire bitrate des MP3 (128kbps recommandé)
4. Vérifier qualité carte SD (classe 10)

### Problème : Dropdown vide dans l'éditeur

**Symptôme:** Le select "Sélectionner un son..." reste vide

**Solutions:**
1. Vérifier que `api_sounds.php` est accessible
2. Console navigateur (F12) pour erreurs JavaScript
3. Vérifier permissions dossier `Appli/sons/`
4. Test direct: `https://bisik.bellocq.local/api_sounds.php`

### Problème : Lecture bloquée indéfiniment

**Symptôme:** Le robot se fige lors d'une action `sound`

**Solutions:**
1. Réduire le timeout dans `playSound()`:
   ```cpp
   unsigned long startTime = millis();
   while (myDFPlayer.readState() != -1) {
     if (millis() - startTime > 30000) break; // 30s max
     delay(100);
   }
   ```
2. Passer en mode non-bloquant (avancé)

## Rollback (retour au buzzer)

Si besoin de revenir au système buzzer:

1. **Restaurer les anciens fichiers**
   ```bash
   cd Bisik
   git checkout HEAD~1 src/main.cpp platformio.ini
   ```

2. **Restaurer editor.php**
   ```bash
   cd Appli
   git checkout HEAD~1 editor.php
   rm api_sounds.php
   ```

3. **Recâbler le buzzer sur D27**

4. **Recompiler et flasher**

## Prochaines améliorations possibles

- [ ] Contrôle du volume depuis l'interface web
- [ ] Upload de nouveaux MP3 via l'interface
- [ ] Prévisualisation serveur-side (génération de waveform)
- [ ] Lecture non-bloquante pour paralléliser servo + son
- [ ] Support des sous-dossiers DFPlayer (01/, 02/, etc.)
- [ ] Migration automatique SQL des anciennes chorégraphies
- [ ] Interface de gestion de la bibliothèque de sons
- [ ] Tags/catégories pour les sons (notification, chat, robot, etc.)

## Références

- [DFPlayer Mini Documentation](https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299)
- [DFRobotDFPlayerMini Library](https://github.com/DFRobot/DFRobotDFPlayerMini)
- [Bisik Specification v2](Bisik2.md)
- [Wiring Diagram](Cablage_ESP32.md)
