# Smart Servo Stepper V2

Ein ESP32-C3-basiertes Closed-Loop-Servosystem. Der ESP32 kompensiert die Einschränkungen des Schrittmotors (Schrittverluste, Resonanzen) durch eine hochfrequente Regelstrecke und emuliert dabei ein Feetech (STS/SCS) Servo am seriellen Bus.

**Neuerungen in V2:**
- **Microcontroller:** ESP32-C3 Super Mini (Kompakter, Single-Core, extrem zuverlässig)
- **Motortreiber:** TMC2209 im UART-Modus (Flüsterleise, Sensorless Homing, Software-Stromregelung)
- **Hardening:** Vollständige Industrie-Schutzbeschaltung (I2C Pull-Ups, Boot-Safe Pull-Ups, Bus-Kurzschlussschutz, Filterkondensatoren)

## Dokumentation

Die vollständigen Schaltpläne, BOM und Verdrahtungstabellen für die Platinenfertigung findest du im Ordner `docs`.
- [TMC2209 UART Guide (WICHTIG)](./software/TMC2209_UART_GUIDE.md) - **Lösung für UART-Verbindungsprobleme.**
- [01_PCB_Wiring_Schematic.md](./docs/01_PCB_Wiring_Schematic.md) - Der komplette Blueprint zur Platinenerstellung.

## 🚀 Software (Firmware)

Die Firmware befindet sich im Ordner `software/SmartServoStepperV2`. 
- **Plattform:** Arduino IDE (ESP32 Core)
- **Features:** 
  - AS5600 Encoder-Anbindung (I2C)
  - TMC2209 Konfiguration & Steuerung (UART)
  - Feetech Servo Protokoll Emulation
  - Closed-Loop PID Regelung

**Installation:**
1. Installiere den [ESP32 Core](https://github.com/espressif/arduino-esp32) in der Arduino IDE.
2. Installiere die Bibliotheken: `TMCStepper`, `AS5600`, `PID`.
3. Öffne `SmartServoStepperV2.ino` und lade es auf den ESP32-C3 Super Mini hoch.

## 🛠 Hardware

Das KiCad-Projekt befindet sich im Ordner `hardware/SmartServoStepperV2`.
- Nutze die [01_PCB_Wiring_Schematic.md](./docs/01_PCB_Wiring_Schematic.md) als Referenz für die Verdrahtung.
