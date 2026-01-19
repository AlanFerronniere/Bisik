S# Plan de câblage ESP32 - Robot Bisik

## Vue d'ensemble

Ce document détaille toutes les connexions entre l'ESP32 et les différents composants du robot Bisik.

## Schéma de connexions

### ESP32 DevKit v1

```
                    ESP32 DevKit v1
                    +-----------+
                    |           |
         [OLED SDA] | D33 (GPIO33)
         [OLED SCL] | D25 (GPIO25)
                    |           |
   [DFPlayer TX →]  | D16 (GPIO16/RX2)
   [DFPlayer RX ←]  | D17 (GPIO17/TX2)
                    |           |
       [Servo PWM]  | D12 (GPIO12)
                    |           |
              [5V]  | 5V        |
             [GND]  | GND       |
                    |           |
       [USB Debug]  | USB       |
                    +-----------+
```

## Tableau détaillé des connexions

| Composant | Broche Composant | Broche ESP32 | Type Signal | Notes |
|-----------|------------------|--------------|-------------|-------|
| **Écran OLED SSD1306** | | | | |
| | VCC | 3.3V | Alimentation | Ou 5V selon modèle |
| | GND | GND | Masse | |
| | SCL | D25 (GPIO25) | I2C Clock | Pull-up 4.7kΩ recommandé |
| | SDA | D33 (GPIO33) | I2C Data | Pull-up 4.7kΩ recommandé |
| **Servomoteur SG90** | | | | |
| | Signal (Orange) | D12 (GPIO12) | PWM | 50Hz, 1-2ms |
| | VCC (Rouge) | 5V | Alimentation | ~150mA en mouvement |
| | GND (Marron) | GND | Masse | |
| **DFPlayer Mini** | | | | |
| | TX | D16 (GPIO16/RX2) | UART RX | Communication série 9600 baud |
| | RX | D17 (GPIO17/TX2) | UART TX | Résistance 1kΩ recommandée |
| | VCC | 5V | Alimentation | 20-30mA, pics à 100mA |
| | GND | GND | Masse | |
| | SPK_1 | HP+ | Audio | Sortie haut-parleur |
| | SPK_2 | HP- | Audio | Sortie haut-parleur |
| **Haut-parleur 8Ω** | | | | |
| | + | SPK_1 (DFPlayer) | Audio | 8Ω, 0.5W min |
| | - | SPK_2 (DFPlayer) | Audio | |
| **Carte micro-SD** | | | | |
| | (Insérée dans DFPlayer) | | Stockage | FAT32, max 32GB |

## Schéma visuel détaillé

```
┌─────────────────────────────────────────────────────────────┐
│                        ESP32 DevKit v1                       │
│                                                              │
│  3.3V ────────────────────────┐                             │
│  D33 (SDA) ───────────────┐   │                             │
│  D25 (SCL) ─────────────┐ │   │                             │
│  GND ─────────────────┐ │ │   │                             │
│                       │ │ │   │                             │
│  D16 (RX2) ──────┐    │ │ │   │                             │
│  D17 (TX2) ────┐ │    │ │ │   │                             │
│  5V ─────────┐ │ │    │ │ │   │                             │
│  GND ───────┐│ │ │    │ │ │   │                             │
│             ││ │ │    │ │ │   │                             │
│  D12 ─────┐ ││ │ │    │ │ │   │                             │
│  5V ────┐ │ ││ │ │    │ │ │   │                             │
│  GND ──┐│ │ ││ │ │    │ │ │   │                             │
└────────┼┼─┼─┼┼─┼─┼────┼─┼─┼───┼─────────────────────────────┘
         ││ │ ││ │ │    │ │ │   │
         ││ │ ││ │ │    │ │ │   └──────┐
         ││ │ ││ │ │    │ │ │          │
    ┌────┴┴─┴─┴┴─┴─┴────┘ │ │   ┌──────┴──────┐
    │  Servomoteur SG90   │ │ │   │  OLED 128x64│
    │                     │ │ │   │  I2C SSD1306│
    │  Marron = GND       │ │ │   │             │
    │  Rouge  = VCC (5V)  │ │ │   │  GND  SCL   │
    │  Orange = Signal    │ │ │   │  VCC  SDA   │
    └─────────────────────┘ │ │   └─────────────┘
                            │ │
                 ┌──────────┴─┴────────────┐
                 │   DFPlayer Mini         │
                 │                         │
                 │  GND  VCC  TX   RX      │
                 │              │   │      │
                 │              └───┼──────┤ Résistance 1kΩ
                 │                  │      │ (protection TX ESP32)
                 │  SPK_1  SPK_2    │      │
                 └────┬──────┬──────┴──────┘
                      │      │
                      │      │
                ┌─────┴──────┴─────┐
                │  Haut-parleur 8Ω │
                │      0.5W         │
                │   +         -     │
                └───────────────────┘
                      
    ┌──────────────────────────┐
    │  Carte micro-SD          │
    │  FAT32, max 32GB         │
    │  /mp3/0001.mp3           │
    │  /mp3/0002.mp3           │
    │  ...                     │
    └──────────────────────────┘
    (Insérée dans le DFPlayer)
```

## Notes importantes sur le câblage

### Alimentation

**Alimentation 5V :**
- ESP32 via USB (développement) ou régulateur externe (production)
- Servomoteur SG90 : consommation ~150mA en mouvement, pics à 300mA
- DFPlayer Mini : 20-30mA normal, pics à 100mA lors du démarrage
- **Total estimé : ~400mA max**
- **Recommandation** : alimentation 5V/1A minimum via régulateur LM7805 ou alimentation USB

**Alimentation 3.3V :**
- OLED peut fonctionner en 3.3V ou 5V selon le modèle
- Vérifier les specs de votre écran

### Protection du DFPlayer

**Résistance série sur TX ESP32 → RX DFPlayer :**
- L'ESP32 sort 3.3V logique, le DFPlayer accepte 3.3V-5V
- Résistance 1kΩ recommandée pour protéger la broche ESP32
- Alternative : diviseur de tension si le DFPlayer est strict 5V

**Condensateur de découplage :**
- Ajouter un condensateur 100µF entre VCC et GND du DFPlayer
- Réduit les grésillements et stabilise l'alimentation lors des pics de consommation

### I2C (OLED)

**Résistances pull-up :**
- SDA et SCL nécessitent des pull-up (4.7kΩ vers 3.3V)
- Souvent intégrées sur les modules OLED I2C
- Vérifier avec un multimètre si nécessaire

**Adresse I2C :**
- Adresse par défaut : `0x3C` (parfois `0x3D`)
- Vérifier avec un scanner I2C si problème de détection

### Servomoteur

**Alimentation séparée recommandée :**
- Pour éviter les chutes de tension et resets de l'ESP32
- Si servo alimenté par l'ESP32 : condensateur 470µF-1000µF entre 5V et GND
- Attention aux interférences : le servo peut perturber le WiFi

**Signal PWM :**
- Fréquence : 50Hz
- Durée d'impulsion : 1ms (0°) à 2ms (180°)
- Le servo est détaché après chaque mouvement pour éviter les vibrations

### DFPlayer et carte SD

**Format carte SD :**
- FAT32 obligatoire (pas exFAT, pas NTFS)
- Max 32GB (limitation du DFPlayer)
- Classe 10 recommandée pour lecture fluide

**Organisation des fichiers :**
- Créer un dossier `/mp3/` à la racine
- Nommer les fichiers : `0001.mp3`, `0002.mp3`, etc.
- Maximum 255 fichiers par dossier

**Qualité audio :**
- Format : MP3 ou WAV
- Bitrate : 128kbps recommandé (max 320kbps)
- Fréquence d'échantillonnage : 44.1kHz ou 22.05kHz

## Pins ESP32 à éviter

Les pins suivants sont utilisés au démarrage ou ont des fonctions spéciales :

| Pin | Raison | Note |
|-----|--------|------|
| GPIO0 | Boot mode | Doit être HIGH au démarrage |
| GPIO2 | Boot mode | LED embarquée sur certains modèles |
| GPIO5 | Strapping pin | Éviter si possible |
| GPIO12 | Strapping pin | **Utilisé pour Servo** (acceptable) |
| GPIO15 | Strapping pin | Doit être LOW au boot |
| GPIO34-39 | Input only | Pas de pull-up interne |

## Vérifications avant mise sous tension

- [ ] Vérifier toutes les connexions GND et VCC (pas de court-circuit)
- [ ] Carte SD insérée dans le DFPlayer avec fichiers MP3 numérotés
- [ ] Résistance 1kΩ sur TX ESP32 → RX DFPlayer (optionnelle mais recommandée)
- [ ] Condensateur 100µF sur alimentation DFPlayer (recommandé)
- [ ] Haut-parleur 8Ω connecté (ne pas faire fonctionner le DFPlayer sans charge)
- [ ] Alimentation suffisante : 5V/1A minimum
- [ ] Pas de connexion croisée TX↔TX ou RX↔RX

## Schéma de test minimal

Pour tester chaque composant individuellement :

### Test 1 : OLED seul
```
ESP32 D33 → OLED SDA
ESP32 D25 → OLED SCL
ESP32 3.3V → OLED VCC
ESP32 GND → OLED GND
```

### Test 2 : Servo seul
```
ESP32 D12 → Servo Signal
ESP32 5V → Servo VCC (Rouge)
ESP32 GND → Servo GND (Marron)
```

### Test 3 : DFPlayer seul
```
ESP32 D16 → DFPlayer TX
ESP32 D17 → [1kΩ] → DFPlayer RX
ESP32 5V → DFPlayer VCC
ESP32 GND → DFPlayer GND
DFPlayer SPK_1 → HP+
DFPlayer SPK_2 → HP-
```

## Troubleshooting

| Problème | Cause probable | Solution |
|----------|----------------|----------|
| OLED ne s'allume pas | Mauvaise adresse I2C | Tester 0x3C et 0x3D |
| Servo ne bouge pas | Alimentation insuffisante | Alimentation externe 5V/1A |
| DFPlayer ne répond pas | TX/RX inversés | Vérifier TX→RX et RX←TX |
| Grésillements audio | Alimentation instable | Ajouter condensateur 100µF |
| ESP32 redémarre | Chute de tension | Alimentation séparée pour servo |
| Carte SD non détectée | Format incorrect | Reformater en FAT32 |
| WiFi instable | Interférences servo | Détacher servo après usage |

## Liste du matériel nécessaire

### Composants principaux
- [ ] ESP32 DevKit v1 (ou compatible)
- [ ] Écran OLED 128x64 I2C SSD1306
- [ ] Servomoteur SG90 (ou MG90S pour plus de couple)
- [ ] DFPlayer Mini MP3
- [ ] Haut-parleur 8Ω 0.5W minimum
- [ ] Carte micro-SD (8-32GB, classe 10)

### Câblage
- [ ] Fils Dupont mâle-femelle (20cm minimum)
- [ ] Fils Dupont mâle-mâle
- [ ] Breadboard (optionnel pour prototypage)

### Composants électroniques
- [ ] Résistance 1kΩ (protection DFPlayer RX)
- [ ] Condensateur électrolytique 100µF 16V (DFPlayer)
- [ ] Condensateur électrolytique 470µF-1000µF 16V (Servo, optionnel)
- [ ] Résistances pull-up 4.7kΩ x2 (I2C, si non intégrées)

### Alimentation
- [ ] Câble micro-USB (développement)
- [ ] Alimentation 5V/1A-2A (production)
- [ ] OU régulateur de tension LM7805 + condensateurs

### Outils
- [ ] Multimètre (vérification continuité et tensions)
- [ ] Fer à souder + étain (connexions permanentes)
- [ ] Pince coupante
- [ ] Pince à dénuder

![[Pasted image 20260115182259.png]]