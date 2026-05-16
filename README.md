# 🦾 SmartServoStepperV2

A high-performance Smart Stepper Servo based on the ESP32-C3 Super Mini, TMC2209 driver, and AS5600 magnetic encoder.

## 🚀 Key Features (v3.0.0 Stable)
- **Direct Frequency Positioning:** Oscillation-free, ultra-precise CNC-style open-loop positioning via `FastAccelStepper`.
- **Dual Communication:** Hybrid WiFi (FritzBox Station Mode + Fallback Access Point).
- **Web Dashboard:** Interactive HTML/JS GUI with real-time telemetry (RPS, Angle).
- **Hybrid TMC2209 Control:** Hardware timers for perfect Step/Dir movement, paired with UART for smart driver configuration (SpreadCycle, current limits).

## 📌 Hardware Configuration
| Component | ESP32-C3 GPIO | Function |
| :--- | :--- | :--- |
| UART RX | 5 | Data from TMC2209 |
| UART TX | 6 | Data to TMC2209 |
| ENABLE | 3 | Driver Stage Control |
| STEP | 4 | Hardware Timer Steps |
| DIR | 2 | Hardware Timer Direction |
| I2C SDA | 0 | AS5600 Data |
| I2C SCL | 10 | AS5600 Clock |
| Status LED| 8 | System Heartbeat |

## 📦 Software Setup
1. **Arduino IDE Settings:**
   - Board: `ESP32C3 Dev Module`
   - USB CDC On Boot: **ENABLED** (Required for Serial Monitor)
   - Flash Mode: **DIO**
2. **Libraries Required:**
   - `TMCStepper`
   - `FastAccelStepper`
   - `ArduinoJson (v7)`
   - `WebSocketsServer`
   - `AS5600`
3. **Usage:**
   - Upload `SmartServoStepperV2.ino`.
   - Access the dashboard at `192.168.178.78` (FritzBox) or `192.168.4.1` (AP).

## 🛠 Stability Fixes
- **Startup Delay:** A 3-second delay in `setup()` ensures the USB-CDC port enumerates correctly.
- **Pin Mapping:** Re-routed signals away from sensitive strap pins (GPIO 9) to ensure reliable boots.
- **Encoder Filtering:** 20ms RPS window to prevent aliasing at high speeds.

---
*Maintained by pfeifferandreas1985*
