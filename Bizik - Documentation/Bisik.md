# Bisik - Robot de notification avec ESP32

Le but du projet est de construire un petit robot de notification, capable d'émettre des sons, de bouger une patte et d'afficher quelques infos sur un petit écran LCD

## Description générale

Le robot est basé sur un ESP32, qui contrôle un servomoteur pour faire bouger un bras, un buzzer pour émettre des sons et un écran OLED pour afficher des informations.
Le robot va servir pour notifier l'utilisateur de certains événements (comme la réception d'un email ou d'un message....) en bougeant le bras, en émettant un son et en affichant un message sur l'écran. Le "cerveau" du robot sera une application web, écrite en PHP, hébergée sur un serveur local (Raspberry Pi par exemple) qui enverra des notifications à l'ESP32 via une connexion WiFi, par l'intermédiaire d'un serveur MQTT (mosquito)

![[Pasted image 20251208164209.png]]

Le but du projet est de construire l'architecture, d'en profiter pour apprendre à utiliser l'ESP32, le MQTT, et de se familiariser avec la programmation de robots simples, avec le C et d'améliorer vos connaissances en PHP. 
Le projet sera aussi un prétexte pour découvrir la gestion du versioning avec Git et GitHub (ou équivalent), et de documenter le projet avec du Markdown.

Et j'espère de faire un joli robot à la fin :) facile à reproduire chez soi.
## Robot

Le code du robot sera en C, compilé avec PlateformIO (extension VSCode) https://platformio.org/
Le robot se connectera au réseau WiFi local, puis au serveur MQTT, et s'abonnera au topic "bisik/henry". Lorsqu'un message est publié sur ce topic, le robot réagira en fonction du contenu du message (par exemple, bouger le bras, émettre un son, afficher un message sur l'écran).

## Application web
L'application web sera écrite en PHP, et permettra d'envoyer des notifications au robot. L'application se connectera au serveur MQTT et publiera des messages sur le topic "bisik/henry" lorsque l'utilisateur souhaite envoyer une notification.

L'idée de l'application est double :
- une interface de **gestion de choregraphies** de notification (par exemple, pour un email, faire bouger le bras 3 fois, émettre une mélodie, afficher "Nouveau mail" sur l'écran)
- des webhooks pour permettre à d'autres applications (comme un serveur de messagerie) d'envoyer des notifications automatiquement.

### Gestion des chorégraphie

La gestion des chorégraphie doit permetter d'ajouter, modifier, supprimer des chorégraphies de notification. Chaque chorégraphie sera composée d'une série d'actions (bouger le bras, émettre une mélodie, afficher un message) avec des paramètres spécifiques.
#### Mélodies
L'outil doit permettre de créer une mélodie en saisissant une série illimitée de notes (chaque demi-ton sur 2 octaves au moins) et la durée de chaque note.
#### Mouvement du bras
L'outil doit permettre de définir une série de positions pour le servomoteur, avec des durées pour chaque position.
#### Messages à afficher
L'outil doit permettre de définir des messages à afficher sur l'écran OLED, avec la possibilité de définir la durée d'affichage. Une partie du message pourra être dynamique (par exemple, afficher l'objet d'un email reçu) en étant remplacée par une variable envoyée par le webhook.

### Webhooks
L'application web doit permettre de définir des webhooks pour chaque chorégraphie. Un webhook est une URL spécifique qui, lorsqu'elle est appelée (par exemple, par un serveur de messagerie), déclenche l'exécution de la chorégraphie associée.
Chaque webhook doit pouvoir recevoir des paramètres (par exemple, l'objet d'un email) qui seront utilisés dans la chorégraphie (par exemple, pour afficher l'objet sur l'écran OLED).

### Exposition de l'interface web
L'interface web est hébergée sur un serveur local https://bisik.bellocq.local
Le projet PHP présent dans le dossier est automatiquement synchronisé sur le serveur local via rsync (rien à faire).
Le serveur MQTT utilisé est mqtt.latetedanslatoile.fr:1883 (auth: Epsi / EpsiWis2018!).

## Matériel utilisé

- un ESP32
- un écran OLED 128 x 64 Pixel I2C (SCL sur D25, SDA sur D33)
- un servomoteur SG90 (contrôlé sur D12)
- un buzzer actif (D27)

## Wifi

SSID : la_tete_dans_la_toile
Mot de passe : concoulcabis

# Base de données 

host  : localhost
user : bisik
database : bisik
password : Bsk32Bsk32#