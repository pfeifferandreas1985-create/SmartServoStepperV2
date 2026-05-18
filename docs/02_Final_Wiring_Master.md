# Hardware-Spezifikation: Smart Servo Stepper V2

**Dokument-Version:** 1.6 (Final Revision – Approved)
**Projekt-Status:** Finalisiert für PCB-Layout
**Letzte Änderung:** 13.05.2026

---

## 1. Systemarchitektur & Power Grid

Das Design ist auf maximale Stabilität bei hohen Motorlasten und Schutz der MCU ausgelegt.

### VMOT (12V–24V)

| Komponente | Funktion | Spezifikation |
| :--- | :--- | :--- |
| **SMBJ24A** | TVS-Diode, transiente Überspannungen | Kathode → VMOT, Anode → GND |
| **C_HF (100 nF)** | Keramik-Kondensator, HF-Filterung | VMOT ↔ GND, direkt am Eingang |
| **C_BULK (470 µF / 50V)** | Elko (Low-ESR), Pufferung | VMOT ↔ GND, unmittelbar am TMC2209 VM/GND |

### 5V System (SYS)

| Komponente | Funktion | Spezifikation |
| :--- | :--- | :--- |
| **MP1584EN** | Buck-Converter | Eingang: VMOT, Ausgang: 5V |
| **C_BUCK (22 µF)** | Keramik, lokaler Ausgangspuffer | Am MP1584EN Ausgang |
| **C_MCU_E (100 µF)** | Elko, MCU-Puffer | Am ESP32 5V-Pin |
| **C_MCU_K (47 µF)** | Keramik (X7R, 1206, ≥16V) | Am ESP32 5V-Pin |
| **C_MCU_HF (100 nF)** | Keramik, HF-Filterung | Direkt am ESP32 5V-Pin |

> [!IMPORTANT]
> Der 47 µF Keramik-Kondensator **muss** X7R-Dielektrikum besitzen und für mindestens 10V (besser 16V) spezifiziert sein, um den Kapazitätsverlust bei 5V Bias zu minimieren.

### 3.3V Logik (LOGIC)

| Komponente | Funktion | Spezifikation |
| :--- | :--- | :--- |
| **VIO TMC2209** | Logik-Pegel Referenz | **Zwingend 3.3V** (nicht 5V!) |
| **C_AS5600 (10 µF)** | Keramik, Sensor-Puffer | Nahe am AS5600, 3.3V ↔ GND |

> [!CAUTION]
> VIO des TMC2209 **muss** mit 3.3V verbunden werden, um Pegel-Kompatibilität zum ESP32 sicherzustellen. Eine Verbindung mit 5V kann den ESP32 beschädigen!

---

## 2. Finales Pin-Mapping (ESP32-C3 Super Mini)

Optimiert zur Vermeidung von Boot-Strap-Konflikten und Sicherstellung der USB-Funktionalität.

| ESP32-C3 Pin | Funktion | Beschreibung | Hardware-Beschaltung |
| :--- | :--- | :--- | :--- |
| **GPIO 6** | UART TMC | PDN_UART Kommunikation | 1 kΩ Schutzwiderstand in Serie |
| **GPIO 1** | Feetech TX | Bus Senden (Half-Duplex) | Via BSS138 Level Shifter (3.3V → 5V) |
| **GPIO 7** | Feetech RX | Bus Empfangen (Half-Duplex) | Via Spannungsteiler (5V → 3.3V) |
| **GPIO 2** | DIAG TMC | StallGuard / Diagnose | 4.7 kΩ Pull-UP gegen 3.3V |
| **GPIO 3** | ENABLE | TMC2209 EN (Active LOW) | 10 kΩ Pull-UP gegen 3.3V (Boot = AUS) |
| **GPIO 10** | SDA | I2C Daten (AS5600) | 4.7 kΩ Pull-UP gegen 3.3V |
| **GPIO 20** | SCL | I2C Takt (AS5600) | 4.7 kΩ Pull-UP gegen 3.3V |
| **GPIO 4** | STEP | Schritt-Impuls | Direkt |
| **GPIO 5** | DIR | Richtungs-Signal | Direkt |
| **GPIO 8** | Status-LED | On-Board LED (Blau) | Intern (Active LOW) |
| **GPIO 18/19** | USB/JTAG | D- / D+ | **Physisch unbeschaltet lassen!** |

---

## 3. Spezifische Schaltungsdetails

### 3.1 ENABLE-Logik (Kritisch)

Um unkontrollierte Motorbewegungen beim Einschalten oder Flashen zu verhindern:

| Aspekt | Detail |
| :--- | :--- |
| **Logik** | TMC2209 EN ist **Active LOW**. Der 10 kΩ Pull-Up an GPIO 3 zieht den Pin beim Booten auf **HIGH** → Motor **deaktiviert**. |
| **Firmware-Handling** | Erst nach vollständiger Initialisierung aller Sensoren wird GPIO 3 aktiv auf **LOW** gezogen (Motor aktiviert). |

> [!WARNING]
> Ohne den Pull-Up könnte GPIO 3 beim Boot floaten und den Motor unkontrolliert ansteuern. Dies ist ein **sicherheitskritisches** Detail.

### 3.2 DIAG-Schnittstelle

Der DIAG-Ausgang des TMC2209 ist **Open-Drain**.

| Aspekt | Detail |
| :--- | :--- |
| **Anbindung** | Direkt an GPIO 2 |
| **Pull-Up** | Externer 4.7 kΩ gegen 3.3V definiert das High-Level |
| **Hinweis** | Keine Anbindung an 5V oder GND-Teiler erforderlich |

### 3.3 Feetech-Bus (High-Speed 1 Mbps)

Um Signalverzögerungen durch den BSS138 im bidirektionalen Betrieb zu umgehen, nutzen wir **zwei getrennte Pfade** am ESP32, die auf der **5V-Seite zusammengeführt** werden:

```
┌─────────────┐                              ┌──────────────┐
│  ESP32-C3   │                              │  5V Feetech  │
│             │    BSS138 Level Shifter      │     Bus      │
│   GPIO 1 ──┼──── 3.3V → 5V ──────────────►├──────────────┤
│   (TX)      │                              │              │
│             │    Spannungsteiler            │              │
│   GPIO 7 ──┼◄── 4.7kΩ + 10kΩ/GND ────────┤              │
│   (RX)      │    (5V → 3.3V)              │              │
└─────────────┘                              └──────┬───────┘
                                                    │
                                              2.2kΩ Pull-Up
                                              gegen 5V
                                           (Terminierung)
```

| Pfad | Signal-Weg | Beschaltung |
| :--- | :--- | :--- |
| **TX-Pfad** | GPIO 1 → BSS138 → 5V-Bus | Level Shifter 3.3V → 5V |
| **RX-Pfad** | 5V-Bus → Spannungsteiler → GPIO 7 | 4.7 kΩ in Serie, 10 kΩ gegen GND |
| **Terminierung** | Am Bus-Ausgang | 2.2 kΩ Pull-Up gegen 5V für scharfe Signalflanken |

---

## 4. Passive Bauteile – Zusammenfassung

### 4.1 Kondensatoren

| Bezeichnung | Wert | Typ | Position |
| :--- | :--- | :--- | :--- |
| C_TVS | 100 nF | Keramik | VMOT ↔ GND, am Eingang |
| C_BULK | 470 µF / 50V | Elko (Low-ESR) | VMOT ↔ GND, am TMC2209 |
| C_BUCK | 22 µF | Keramik | MP1584EN Ausgang |
| C_MCU_E | 100 µF | Elko | ESP32 5V-Pin |
| C_MCU_K | 47 µF | Keramik (X7R, 1206, ≥16V) | ESP32 5V-Pin |
| C_MCU_HF | 100 nF | Keramik | ESP32 5V-Pin |
| C_AS5600 | 10 µF | Keramik | AS5600 3.3V ↔ GND |

### 4.2 Widerstände

| Bezeichnung | Wert | Position / Funktion |
| :--- | :--- | :--- |
| R_SDA | 4.7 kΩ | GPIO 10 (SDA) → 3.3V Pull-Up |
| R_SCL | 4.7 kΩ | GPIO 20 (SCL) → 3.3V Pull-Up |
| R_EN | 10 kΩ | GPIO 3 (ENABLE) → 3.3V Pull-Up (Boot-Safe) |
| R_DIAG | 4.7 kΩ | GPIO 2 (DIAG) → 3.3V Pull-Up |
| R_UART | 1 kΩ | GPIO 6 → TMC2209 PDN_UART (Serienwiderstand) |
| R_TERM | 2.2 kΩ | Feetech 5V-Bus → 5V Pull-Up (Terminierung) |
| R_DIV1 | 4.7 kΩ | Feetech RX Spannungsteiler (Serie) |
| R_DIV2 | 10 kΩ | Feetech RX Spannungsteiler (gegen GND) |

### 4.3 Schutzkomponenten

| Bezeichnung | Typ | Position |
| :--- | :--- | :--- |
| D_TVS | SMBJ24A | VMOT Eingang, Kathode → VMOT |
| U_LS | BSS138 Level Shifter | Feetech TX (3.3V → 5V) |

---

## 5. Layout- & Fertigungshinweise

| Thema | Vorgabe |
| :--- | :--- |
| **Thermal Management** | Großflächige Ground-Planes auf Top- und Bottom-Layer. Unter dem TMC2209 „Exposed Pad" mindestens **5 Thermal Vias** (0.3 mm Bohrung) setzen. |
| **EMI-Schutz** | I2C-Leitungen (SDA/SCL) so kurz wie möglich und **räumlich getrennt** von Motor-Leitungen (Phasen A/B). |
| **Kondensator-Platzierung** | Alle Abblock-Kondensatoren (100 nF) direkt an den zugehörigen IC-Pins. MCU-Puffer-Kondensatoren in unmittelbarer Nähe zum ESP32 5V-Pin. |
| **Mechanik** | Zentrales **6 mm Loch** für Magnet-Halterung der Motorwelle (AS5600 Encoder). |

---

## 6. Änderungshistorie

| Version | Änderungen |
| :--- | :--- |
| **V1.5 → V1.6** | ENABLE-Logik auf Pull-Up korrigiert (Boot-Sicherheit). DIAG-Schaltung auf simplen Pull-Up vereinfacht. Feetech-Bus auf dedizierte TX/RX-Pins (GPIO 1 / GPIO 7) für 1 Mbps Stabilität optimiert. GPIO 0 Boot-Konflikt aufgelöst (STEP → GPIO 4, DIR → GPIO 5). I2C auf GPIO 10/20 verlegt. Vollständiges Power-Grid mit TVS-Diode und spezifizierten Puffer-Kondensatoren dokumentiert. |
| **V1.4 → V1.5** | Initiale Konsolidierung des Verdrahtungsplans für V2-Architektur. |

---

*Erstellt am 12.05.2026, aktualisiert am 13.05.2026 durch Antigravity AI für Smart Servo Stepper V2 Projekt.*
*Dokument-Status: **FREIGEGEBEN** für PCB-Layout.*
