// ═══════════════════════════════════════════════════════════
// SmartServoStepperV2 - PIN CONFIGURATION v2.5 STABLE
// ═══════════════════════════════════════════════════════════
#ifndef CONFIG_H
#define CONFIG_H

// --- TMC2209 PINOUT ---
#define PIN_TMC_RX      5   // UART RX (Connect via 1k Resistor to PDN_UART)
#define PIN_TMC_TX      6   // UART TX (Direct to PDN_UART)
#define PIN_EN          3   // ENABLE (Active LOW)
#define PIN_STEP        4   // STEP (Optional, using UART VACTUAL for primary move)
#define PIN_DIR         2   // DIR (Optional)

// --- AS5600 I2C PINOUT ---
#define PIN_SDA         0   // I2C Data
#define PIN_SCL         10  // I2C Clock

// --- STATUS ---
#define PIN_LED         8   // Onboard Blue LED

// --- PARAMETERS ---
#define TMC_RMS_CURRENT 1000 // 1A
#define TMC_DRIVER_ADDR 0b00
#define TMC_SENSE_RES   0.11f

#endif
