# Bisik Project Context

## Project Overview
**Bisik** is an IoT notification robot project. The system consists of a physical robot powered by an ESP32 that reacts to events by moving a servo arm, playing sounds, and displaying messages on an OLED screen. The "brain" of the system is a PHP web application that manages "choreographies" (sequences of actions) and triggers them via MQTT.

## Directory Structure

*   **`Bisik/`**: PlatformIO project for the ESP32 firmware.
    *   `platformio.ini`: Configuration for `esp32dev`.
    *   `src/main.cpp`: Main firmware logic.
*   **`Appli/`**: PHP web application for managing choreographies and webhooks.
    *   `index.php`: Main entry point.
*   **`Documentation/`**: Project documentation.
    *   `Bisik.md`: Detailed functional specifications and hardware setup.

## System Architecture

The system follows a distributed architecture:
1.  **Web Application (`Appli/`):**
    *   **Role:** Choreography manager and webhook handler.
    *   **Features:** Define melodies, servo movements, and OLED messages. Expose webhooks to trigger these sequences.
    *   **Communication:** Publishes messages to an MQTT broker (Mosquitto) on `bellocq.local`.
    *   **Deployment:** Syncs to `https://bisik.bellocq.local` (via rsync).

2.  **Robot Firmware (`Bisik/`):**
    *   **Role:** Executes physical actions.
    *   **Logic:** Connects to WiFi and MQTT broker (`bellocq.local`), subscribes to `bisik/notification`, and executes commands received.

## Hardware Configuration (ESP32)

| Component | Pin / Interface |
| :--- | :--- |
| **OLED Screen** (128x64) | I2C (SCL: **D25**, SDA: **D33**) |
| **Servo Motor** (SG90) | **D12** |
| **Active Buzzer** | **D27** |

## Development & Usage

### Firmware (ESP32)
Built with **PlatformIO**.
*   **Build:** `pio run` (in `Bisik/` dir)
*   **Upload:** `pio run --target upload`
*   **Monitor:** `pio device monitor`

### Web Application (PHP)
*   **Local Run:** `php -S localhost:8000` (in `Appli/` dir)
*   **Production:** The app is intended to run on `https://bisik.bellocq.local`.

## Key Features to Implement
*   **MQTT Integration:** Both firmware and web app need to communicate via the `bisik/notification` topic.
*   **Choreography Engine:**
    *   **Melodies:** Sequences of notes and durations.
    *   **Movements:** Servo position sequences.
    *   **Display:** Dynamic text messages on OLED.
*   **Webhooks:** Triggering actions via HTTP requests with dynamic parameters.