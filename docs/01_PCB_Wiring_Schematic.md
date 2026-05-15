# SmartServoStepperV2 – PCB Wiring & Schaltplan (v1.6 Final)

Dieses Dokument ist der **Master-Bauplan** für das PCB-Layout, abgeglichen mit der Hardware-Spezifikation v1.6.

---

## 1. Systemarchitektur & Power Grid

### VMOT (12V–24V)
- **Schutz:** SMBJ24A TVS-Diode (transiente Überspannungen) + 100nF Keramik (HF)
- **Puffer:** 470 µF / 50V Elko (Low-ESR) unmittelbar am TMC2209 VM/GND

### 5V System (SYS)
- **Quelle:** MP1584EN Buck-Converter (Ausgang lokal gepuffert mit 22µF Keramik)
- **MCU-Puffer:** 100 µF Elko + 47 µF Keramik (X7R, 1206) + 100 nF direkt am ESP32 5V-Pin

### 3.3V Logik (LOGIC)
- **VIO TMC2209:** Zwingend mit 3.3V verbinden (nicht 5V!)
- **AS5600 Puffer:** 10 µF Keramik nahe am Sensor

---

## 2. Finales Pin-Mapping (ESP32-C3 Super Mini)

| ESP32-C3 Pin | Funktion | Beschreibung | Hardware-Beschaltung |
| :--- | :--- | :--- | :--- |
| **GPIO 1** | Feetech TX | Bus Senden (Half-Duplex) | Via BSS138 Level Shifter (3.3V→5V) |
| **GPIO 2** | DIAG TMC | StallGuard / Diagnose | 4.7kΩ Pull-UP gegen 3.3V (Open-Drain) |
| **GPIO 3** | ENABLE | TMC2209 EN (Active LOW) | 10kΩ Pull-UP gegen 3.3V (Boot = AUS) |
| **GPIO 4** | STEP | Schritt-Impuls | Direkt |
| **GPIO 5** | DIR | Richtungs-Signal | Direkt |
| **GPIO 6** | UART TMC | PDN_UART Kommunikation | 1kΩ Schutzwiderstand in Serie |
| **GPIO 7** | Feetech RX | Bus Empfangen (Half-Duplex) | Via Spannungsteiler (4.7kΩ Serie + 10kΩ→GND) |
| **GPIO 8** | Status-LED | On-Board LED (Blau) | Intern (Active LOW) |
| **GPIO 10** | SDA | I2C Daten (AS5600) | 4.7kΩ Pull-UP gegen 3.3V |
| **GPIO 20** | SCL | I2C Takt (AS5600) | 4.7kΩ Pull-UP gegen 3.3V |
| **GPIO 18/19** | USB/JTAG | D- / D+ | **Physisch unbeschaltet lassen!** |

---

## 3. Spezifische Schaltungsdetails

### 3.1 ENABLE-Logik (Kritisch)
- **Logik:** TMC2209 aktiv bei LOW. 10kΩ Pull-Up zieht GPIO 3 beim Booten auf HIGH (Disabled).
- **Firmware:** Erst nach Initialisierung aller Sensoren (AS5600, TMC2209) wird GPIO 3 aktiv auf LOW gezogen.

### 3.2 DIAG-Schnittstelle
- **DIAG-Ausgang:** Open-Drain
- **Anbindung:** Direkt an GPIO 2. Externer 4.7kΩ Pull-Up gegen 3.3V definiert das High-Level.
- **Firmware:** Hardware-Interrupt (RISING) für Echtzeit-Stallerkennung.

### 3.3 Feetech-Bus (High-Speed 1 Mbps)

Getrennte TX/RX-Pfade am ESP32, auf der 5V-Seite zusammengeführt:

```
ESP32-C3                                           Feetech Bus (5V)
─────────                                          ────────────────
GPIO 1 (TX) ──→ BSS138 Level Shifter ──→ 5V Bus ──┐
                                                    ├── DATA (1-Wire)
GPIO 7 (RX) ←── Spannungsteiler ←──────── 5V Bus ──┘
                (4.7kΩ Serie + 10kΩ→GND)
                                                    │
                                              2.2kΩ Pull-Up → 5V
                                           (Terminierung / scharfe Flanken)
```

- **TX-Pfad:** GPIO 1 → BSS138 → 5V Bus
- **RX-Pfad:** 5V Bus → 4.7kΩ in Serie → GPIO 7 (+ 10kΩ gegen GND = Spannungsteiler 5V→3.3V)
- **Terminierung:** 2.2kΩ Pull-Up gegen 5V am Bus-Ausgang

### 3.4 TMC2209 UART (Single-Wire)
- **GPIO 6** → 1kΩ Serienwiderstand → TMC2209 PDN_UART
- Bidirektionale Kommunikation über eine Leitung (SoftwareSerial, 115200 baud)

---

## 4. Stückliste (BOM)

### Aktive Bauteile
| Typ | Bauteil / Modul | Menge | Funktion |
| :--- | :--- | :--- | :--- |
| **MCU** | ESP32-C3 Super Mini | 1x | Hauptprozessor (Single-Core, 160MHz, WLAN) |
| **Driver** | TMC2209 Stepper Driver | 1x | Ultra-leiser Treiber (UART, StealthChop, CoolStep, StallGuard) |
| **Sensor** | AS5600 Magnetic Encoder | 1x | 12-Bit Absolut-Encoder (I²C) |
| **Comms** | BSS138 Level Shifter | 1x | 3.3V→5V für Feetech TX-Pfad |
| **Power** | MP1584EN Buck Converter | 1x | 12V→5V Step-Down |
| **Schutz** | SMBJ24A TVS-Diode | 1x | Transientenschutz VMOT |

### Passivbauteile
| ID | Bauteil | Wert | Position |
| :--- | :--- | :--- | :--- |
| C1 | Elko (Low-ESR, 50V) | 470 µF | VMOT / GND (am TMC2209) |
| C2 | Keramik | 100 nF | VMOT / GND (HF-Filter) |
| C3 | Keramik (X7R, 1206) | 47 µF | ESP32 5V / GND |
| C4 | Keramik | 100 nF | ESP32 5V / GND (Entkopplung) |
| C5 | Elko | 100 µF | ESP32 5V / GND (Bulk) |
| C6 | Keramik | 22 µF | Buck-Converter Ausgang |
| C7 | Keramik | 10 µF | AS5600 3.3V / GND |
| R1 | Widerstand | 4.7 kΩ | SDA (GPIO 10) → 3.3V (I2C Pull-Up) |
| R2 | Widerstand | 4.7 kΩ | SCL (GPIO 20) → 3.3V (I2C Pull-Up) |
| R3 | Widerstand | 10 kΩ | EN (GPIO 3) → 3.3V (Boot-Safe) |
| R4 | Widerstand | 4.7 kΩ | DIAG (GPIO 2) → 3.3V (Open-Drain Pull-Up) |
| R5 | Widerstand | 1 kΩ | GPIO 6 → TMC PDN_UART (Schutz) |
| R6 | Widerstand | 4.7 kΩ | Feetech RX Spannungsteiler (Serie) |
| R7 | Widerstand | 10 kΩ | Feetech RX Spannungsteiler (→GND) |
| R8 | Widerstand | 2.2 kΩ | Feetech Bus Terminierung (→5V) |

---

## 5. Layout- & Fertigungshinweise

- **Thermal Management:** Großflächige Ground-Planes auf Top- und Bottom-Layer. Unter TMC2209 "Exposed Pad" mindestens **5 Thermal Vias** (0.3mm Bohrung).
- **EMI-Schutz:** I2C-Leitungen (SDA/SCL) so kurz wie möglich. Räumlich von Motor-Phasen (A/B) trennen.
- **Kondensatoren:** 47µF Keramik muss **X7R-Dielektrikum** besitzen, min. 10V (besser 16V).
- **Mechanik:** Zentrales **6mm Loch** für Magnet-Halterung der Motorwelle.
- **USB/JTAG:** GPIO 18/19 physisch **unbeschaltet** lassen.

---

## 6. Architektur-Diagramm

```mermaid
graph TD
    %% Power
    PSU((12V-24V)) -->|VMOT| TVS[SMBJ24A TVS]
    TVS --> C_VMOT[470µF + 100nF]
    C_VMOT --> TMC_VM[TMC2209 VMOT]
    PSU --> BUCK[MP1584EN Buck]
    BUCK -->|5V| C_5V[22µF + 100µF + 47µF + 100nF]
    C_5V --> ESP[ESP32-C3 5V]
    C_5V --> SHIFT[BSS138 HV]

    %% 3.3V
    ESP -->|3.3V| TMC_VIO[TMC2209 VIO]
    ESP -->|3.3V| C_AS[10µF → AS5600]

    %% I2C
    ESP_I2C["GPIO 10 (SDA) + GPIO 20 (SCL)"] -->|I2C 400kHz| AS5600
    3V3[3.3V] -.->|R1/R2 4.7kΩ| ESP_I2C

    %% TMC Control
    ESP4["GPIO 4"] -->|STEP| TMC_STEP[TMC2209]
    ESP5["GPIO 5"] -->|DIR| TMC_DIR[TMC2209]
    ESP3["GPIO 3"] -->|EN| TMC_EN[TMC2209]
    3V3 -.->|R3 10kΩ| ESP3

    %% TMC UART
    ESP6["GPIO 6"] -->|R5 1kΩ| TMC_UART["TMC2209 PDN_UART"]

    %% TMC DIAG
    TMC_DIAG["TMC2209 DIAG"] -->|R4 4.7kΩ→3.3V| ESP2["GPIO 2 (IRQ)"]

    %% Feetech Bus
    ESP1["GPIO 1 (TX)"] --> SHIFT_TX["BSS138"] --> BUS_5V["5V Bus"]
    BUS_5V --> DIVIDER["R6 4.7kΩ + R7 10kΩ→GND"]
    DIVIDER --> ESP7["GPIO 7 (RX)"]
    BUS_5V --> R8["R8 2.2kΩ→5V"]
    BUS_5V --> FEETECH((FEETECH DATA))
```
