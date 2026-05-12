# Smart Servo Stepper V2

Ein ESP32-C3-basiertes Closed-Loop-Servosystem. Der ESP32 kompensiert die Einschränkungen des Schrittmotors (Schrittverluste, Resonanzen) durch eine hochfrequente Regelstrecke und emuliert dabei ein Feetech (STS/SCS) Servo am seriellen Bus.

**Neuerungen in V2:**
- **Microcontroller:** ESP32-C3 Super Mini (Kompakter, Single-Core, extrem zuverlässig)
- **Motortreiber:** TMC2209 im UART-Modus (Flüsterleise, Sensorless Homing, Software-Stromregelung)
- **Hardening:** Vollständige Industrie-Schutzbeschaltung (I2C Pull-Ups, Boot-Safe Pull-Ups, Bus-Kurzschlussschutz, Filterkondensatoren)

## Dokumentation

Die vollständigen Schaltpläne, BOM und Verdrahtungstabellen für die Platinenfertigung findest du im Ordner `docs`.
- [01_PCB_Wiring_Schematic.md](./docs/01_PCB_Wiring_Schematic.md) - Der komplette Blueprint zur Platinenerstellung.
