// ═══════════════════════════════════════════════════════════
// SmartServoStepperV2 – config.h  (v1.6 Final)
// Hardware-Spezifikation abgeglichen mit PCB-Layout v1.6
// Board: ESP32-C3 Super Mini
// ═══════════════════════════════════════════════════════════
#ifndef CONFIG_H
#define CONFIG_H

// ─── Pin-Mapping (ESP32-C3 Super Mini, v1.6 Final) ───
//
// Feetech TTL Bus (Half-Duplex, 1 Mbps)
//   TX-Pfad: GPIO 1 → BSS138 Level Shifter (3.3V→5V) → 5V Bus
//   RX-Pfad: 5V Bus → Spannungsteiler (4.7kΩ Serie + 10kΩ→GND) → GPIO 7
//   Bus-Terminierung: 2.2kΩ Pull-Up gegen 5V am Bus-Ausgang
#define PIN_FT_TX       1
#define PIN_FT_RX       7

// TMC2209 DIAG (StallGuard4 / Diagnose)
//   Open-Drain Ausgang, externer 4.7kΩ Pull-Up gegen 3.3V
#define PIN_DIAG        2

// TMC2209 ENABLE (Active LOW)
//   10kΩ Pull-Up gegen 3.3V → Boot = Motor AUS
#define PIN_EN          3

// TMC2209 Step/Dir
#define PIN_STEP        18    // Direkt
#define PIN_DIR         19    // Direkt

// TMC2209 UART (Two-Wire SoftwareSerial)
//   RX: GPIO 5 (Direct)
//   TX: GPIO 6 (via 1kΩ resistor)
#define PIN_TMC_RX      5
#define PIN_TMC_TX      6

// Status-LED (On-Board, Blau, Active LOW)
#define PIN_LED         8

// I2C → AS5600 Magnetic Encoder
//   GPIO 0 (SDA) und GPIO 9 (SCL)
#define PIN_SDA         0
#define PIN_SCL         9

// USB/JTAG: GPIO 18/19 (D-/D+) → physisch unbeschaltet lassen!

// ─── Motor Constants ───
#define STEPS_PER_REV      200     // NEMA17 = 200 full steps/rev
#define DEFAULT_MICROSTEPS  16     // TMC2209: 1,2,4,8,16,32,64,128,256
#define ENCODER_RES        4096    // AS5600: 12-bit absolute
#define ENCODER_CPR        4096.0f

// ─── PID Defaults ───
#define DEFAULT_KP     2.0f
#define DEFAULT_KI     0.1f
#define DEFAULT_KD     0.05f
#define PID_SAMPLE_MS  5          // 200 Hz control loop
#define PID_OUT_MIN   -1000.0f
#define PID_OUT_MAX    1000.0f

// ─── TMC2209 Defaults ───
#define TMC_RMS_CURRENT    800    // mA – safe starting point for NEMA17
#define TMC_SENSE_RESISTOR 0.11f  // Rsense on typical TMC2209 breakout
#define TMC_DRIVER_ADDR    0      // MS1=LOW, MS2=LOW → address 0
#define TMC_UART_BAUD      115200
#define TMC_STALL_THRESHOLD 50    // StallGuard4 threshold (0–255)
#define TMC_COOLSTEP_LOWER  2     // CoolStep lower threshold
#define TMC_COOLSTEP_UPPER  8     // CoolStep upper threshold
#define TMC_TPWM_THRESHOLD  100   // stealthChop→spreadCycle velocity threshold

// ─── Feetech Protocol ───
#define SERVO_ID       1
#define FT_BAUDRATE    1000000    // 1 Mbps (STS/SCS standard)

// ─── WebSocket / WiFi ───
#define WIFI_SSID      "FRITZ!Box 6660 Cable UE"
#define WIFI_PASS      "28370714691864306613"
#define WS_PORT        81
#define TELEMETRY_INTERVAL_MS  10  // 100 Hz telemetry push

// ─── Safety ───
#define WATCHDOG_TIMEOUT_MS 5000   // Motor kill if no WS data for 5s
#define MAX_STEP_FREQ       20000  // Absolute max step frequency (Hz)
#define POSITION_DEADBAND   3      // encoder ticks – stops jitter

#endif // CONFIG_H
