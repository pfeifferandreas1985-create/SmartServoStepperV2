# TMC2209 UART Troubleshooting & Solution Guide (ESP32-C3)

This document describes the successful resolution of UART communication issues between an ESP32-C3 and a TMC2209 motor driver.

## 1. The Core Problem
The initial setup failed with `0x00` (no communication) or caused system brownouts (ESP32 disconnecting from USB).
- **USB-Serial Conflict:** GPIO 20 and 21 on the ESP32-C3 are natively tied to the internal USB-Serial/JTAG unit. Using them for secondary UART communication caused stability issues and crashes.
- **Signal Integrity:** The TMC2209 requires a specific single-wire UART configuration with a pull-up and a series resistor to separate the RX and TX paths of the MCU.

## 2. The Proven Solution (The "Safe Mode")
To achieve stable 0x21 version handshakes and prevent hardware damage, the following configuration was implemented:

### Hardware Wiring
| ESP32-C3 Pin | TMC2209 Pin | Component |
| :--- | :--- | :--- |
| **GPIO 5 (RX)** | PDN_UART | Direct connection |
| **GPIO 6 (TX)** | PDN_UART | **1kΩ Resistor in series** |
| **3.3V (VCC)** | PDN_UART | **4.7kΩ Pull-up Resistor** |
| **GPIO 3 (EN)** | EN | Grounded or controlled by MCU |
| **GND** | GND | Common ground is essential |

### Software Configuration
- **Library:** `TMCStepper`
- **Driver:** `SoftwareSerial` (highly recommended for C3 to avoid hardware remapping issues)
- **Baudrate:** 19200 Baud (proven stability)
- **Critical Register Flags:**
  - `pdn_disable(true)`
  - `mstep_reg_select(true)`

## 3. Implementation Steps
1. **Reset State:** Flash a blank sketch first to ensure the board is electrically stable.
2. **Sequential Power-up:** Connect Logic Power (3.3V/GND) first, verify USB stability, then connect Motor Power (12V).
3. **UART Handshake:** Use the diagnostic script to poll the version register. `0x21` confirms success.

## 4. Final Pin Mapping
```cpp
#define UART_RX_PIN 5
#define UART_TX_PIN 6
#define EN_PIN      3
#define STEP_PIN    18
#define DIR_PIN     19
```

---
*Status: Verified and Operational (2026-05-15)*
