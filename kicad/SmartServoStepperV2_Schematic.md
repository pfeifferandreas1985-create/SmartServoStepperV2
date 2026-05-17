# SmartServoStepperV2 – Konsistenter KiCad-Stromlaufplan

**für ESP32-C3 Super Mini + TMC2209 Tenstar Robot V2 + AS5600**

> Verifiziert gegen Firmware `SmartServoStepperV2.ino` (Stand: Mai 2026)

---

## 1. Power Architecture

### VMOT Eingang (12–24V)

```text
VIN_MOTOR ────────────────+─────────────> VMOT (TMC2209)
                          |
                          |
                       [SMBJ24A]
                          |
                         GND

VIN_MOTOR ──────+────────────+
                |            |
              [470µF]      [100nF]
                |            |
               GND          GND
```

### Buck Converter (MP1584EN)

```text
VIN_MOTOR ─────────> MP1584EN IN+

GND ───────────────> MP1584EN IN-

MP1584 OUT+ ───────> +5V_SYS
MP1584 OUT- ───────> GND
```

### 5V Ausgangspufferung

```text
+5V_SYS ──────+────────+────────+
              |        |        |
            [100µF] [47µF] [100nF]
              |        |        |
             GND      GND      GND
```

---

## 2. ESP32-C3 Super Mini Versorgung

```text
+5V_SYS ─────────────> ESP32 5V
GND ─────────────────> ESP32 GND
```

---

## 3. 3.3V Logic Rail

```text
ESP32 3V3 ───────────> +3V3_LOGIC
```

**Verbraucher:**
- AS5600 VCC
- TMC2209 VIO
- I2C Pullups
- UART Pullup
- ENABLE Pullup

---

## 4. AS5600 Encoder

### Versorgung

```text
+3V3_LOGIC ───────> AS5600 VCC
GND ──────────────> AS5600 GND

+3V3_LOGIC ──[10µF]── GND
```

### I2C

```text
ESP32 GPIO0  ─────────────> SDA
                     |
                   [4.7k]
                     |
                  +3V3

ESP32 GPIO10 ────────────> SCL
                     |
                   [4.7k]
                     |
                  +3V3
```

---

## 5. TMC2209 Tenstar V2

### Versorgung

```text
VMOT  ─────────────> VM
GND   ─────────────> GND

+3V3_LOGIC ────────> VIO
```

### STEP / DIR

```text
ESP32 GPIO4 ───────> STEP

ESP32 GPIO2 ───────> DIR
```

### ENABLE (Boot-Safe)

```text
ESP32 GPIO3 ───────> EN

EN ─────[10k]──────> +3V3
```

> Damit bleibt der Treiber beim Boot deaktiviert.

---

## 6. UART Verbindung (WICHTIG)

Beim Tenstar TMC2209 V2 wird UART über PDN_UART geführt.

### Verschaltung

```text
ESP32 GPIO5 (RX) ───────────────┐
                                │
                                ├────> PDN_UART
                                │
ESP32 GPIO6 (TX) ──[1k]─────────┘

PDN_UART ──[4.7k]──> +3V3
```

> **Hinweis:** Der 1kΩ Serienwiderstand sitzt am **TX-Pin (GPIO 6)**, damit bei der Two-Wire-Konfiguration der TMC2209 den Bus beim Antworten übersteuern kann.

---

## 7. Feetech Half-Duplex Bus (1 Mbps)

**Verwendet:**
- GPIO1 = TX
- GPIO7 = RX
- BSS138 Bidirectional Level Shifter

---

## 8. Level Shifter Verschaltung

### LV-Seite (3.3V)

```text
ESP32 GPIO1 ──[220R]──> LV1

ESP32 GPIO7 ──────────> LV2

LV ───────────────────> +3V3
GND ──────────────────> GND
```

### HV-Seite (5V)

```text
HV1 ───────┐
           ├──────> FEETECH_DATA
HV2 ───────┘

HV ────────────────> +5V_SYS

FEETECH_DATA ──[2.2k]──> +5V_SYS
```

---

## 9. Motoranschlüsse

```text
TMC2209 A1 ─────> MOTOR_A+
TMC2209 A2 ─────> MOTOR_A-

TMC2209 B1 ─────> MOTOR_B+
TMC2209 B2 ─────> MOTOR_B-
```

---

## 10. USB / JTAG

```text
GPIO18 → NC
GPIO19 → NC
```

> **Nicht verbinden.**

---

## 11. Empfohlene KiCad-Netlabels

### Power

| Label | Beschreibung |
| :--- | :--- |
| `VIN_MOTOR` | Eingangsversorgung 12–24V |
| `+5V_SYS` | Buck-Converter Ausgang |
| `+3V3_LOGIC` | ESP32 3.3V Rail |
| `GND` | Masse |
| `VMOT` | TMC2209 Motor-Versorgung |

### Motor

| Label | Beschreibung |
| :--- | :--- |
| `MOTOR_A+` | Spule A+ |
| `MOTOR_A-` | Spule A- |
| `MOTOR_B+` | Spule B+ |
| `MOTOR_B-` | Spule B- |

### I2C

| Label | Beschreibung |
| :--- | :--- |
| `I2C_SDA` | GPIO0 |
| `I2C_SCL` | GPIO10 |

### TMC

| Label | Beschreibung |
| :--- | :--- |
| `TMC_STEP` | GPIO4 |
| `TMC_DIR` | GPIO2 |
| `TMC_EN` | GPIO3 |
| `TMC_UART` | PDN_UART (GPIO5 + GPIO6) |

### Feetech

| Label | Beschreibung |
| :--- | :--- |
| `FEETECH_TX` | GPIO1 |
| `FEETECH_RX` | GPIO7 |
| `FEETECH_DATA` | 5V Bus (1-Wire) |

---

## 12. Kritische Layoutregeln

### TMC2209

- 470µF Elko **maximal 15 mm** vom VM-Pin entfernt
- Breite VMOT-Polygonfläche
- **Thermal-Vias** unter Exposed Pad:
  - Mindestens **5 Stück**
  - **0.3 mm** Drill
  - Via-in-pad optional

### I2C

- SDA/SCL **kurz** halten
- **Nicht** parallel zu Motorphasen

### UART

- PDN_UART **kurz** routen
- **Keine** Nähe zu STEP-Leitung

### Motorphasen

- A/B **differenziell und symmetrisch**
- Möglichst **breite Leiterbahnen**

---

## 13. ERC-kritische Hinweise

> **WICHTIG:** Der ESP32-C3 besitzt einige Boot-Strapping-Pins.

Die gewählte Belegung ist gültig, weil:

- **GPIO3** durch Pull-Up definiert
- **GPIO0** nur via I2C Pull-Up belastet
- **GPIO1** TX nur über 220R gekoppelt
- **GPIO18/19** unbenutzt

**Damit ist das Design bootfähig.**

---

## 14. Empfohlene KiCad-Hierarchie

```text
ROOT
├── Power.kicad_sch
├── ESP32.kicad_sch
├── TMC2209.kicad_sch
├── AS5600.kicad_sch
├── FeetechBus.kicad_sch
└── Connectors.kicad_sch
```

---

## 15. Footprint-Empfehlungen

### Passiv

| Typ | Footprint |
| :--- | :--- |
| 100nF | 0603 |
| 4.7kΩ | 0603 |
| 220Ω | 0603 |
| 1kΩ | 0603 |
| 2.2kΩ | 0603 |
| 10kΩ | 0603 |
| 10µF | 0805 |
| 47µF X7R | 1206 |
| 470µF Elko | Radial D10 |

---

## 16. Wichtigste elektrische Freigabe

Dieses Design ist konsistent bezüglich:

- ✅ Power Domains
- ✅ Boot-Verhalten
- ✅ UART (TMC2209 Two-Wire)
- ✅ I2C (AS5600)
- ✅ Half-Duplex Bus (Feetech via BSS138)
- ✅ ESP32-C3 Bootstrapping
- ✅ 3.3V / 5V Pegel
- ✅ EMV-Basismaßnahmen
