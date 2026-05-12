#ifndef CONFIG_H
#define CONFIG_H

// --- Pin Definitions (ESP32-C3 Super Mini) ---

// I2C for AS5600 Encoder
#define PIN_SDA 4
#define PIN_SCL 5

// TMC2209 Stepper Driver
#define PIN_STEP 0
#define PIN_DIR  1
#define PIN_EN   3
#define PIN_TMC_UART 6

// Feetech Servo Bus (Half-Duplex via Level Shifter)
#define PIN_FT_TX 21
#define PIN_FT_RX 20

// --- Motor & Control Constants ---
#define STEPS_PER_REV 200
#define MICROSTEPS 16
#define ENCODER_RES 4096 // 12-bit

// PID Constants (Initial Tuning)
#define PID_KP 2.0
#define PID_KI 0.1
#define PID_KD 0.05

// Feetech Protocol Constants
#define SERVO_ID 1
#define BAUDRATE 1000000 // 1Mbps for Feetech

#endif
