#include <Arduino.h>
#include <Wire.h>
#include <AS5600.h>
#include <TMCStepper.h>
#include <PID_v1.h>
#include <SoftwareSerial.h>
#include "config.h"
#include "FeetechProtocol.h"

// --- Global Objects ---
AS5600 encoder(&Wire);
SoftwareSerial tmcSerial(PIN_TMC_UART, PIN_TMC_UART);
TMC2209Stepper driver(&tmcSerial, 0.11f, 0);
FeetechProtocol protocol(Serial1, SERVO_ID);

// PID Variables
double setpoint = 2048; // Middle of 4096
double input = 0;
double output = 0;
PID myPID(&input, &output, &setpoint, PID_KP, PID_KI, PID_KD, DIRECT);

// --- Callbacks ---
void handleGoalPos(int16_t pos, uint16_t time, uint16_t speed) {
    setpoint = pos;
    Serial.print("New Setpoint: ");
    Serial.println(setpoint);
}

void setup() {
    Serial.begin(115200);
    
    // Feetech Bus (UART1)
    Serial1.begin(BAUDRATE, SERIAL_8N1, PIN_FT_RX, PIN_FT_TX);
    protocol.onWritePos(handleGoalPos);

    // I2C for AS5600
    Wire.begin(PIN_SDA, PIN_SCL);
    encoder.begin();

    // TMC2209
    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_DIR, OUTPUT);
    digitalWrite(PIN_EN, HIGH);

    tmcSerial.begin(115200);
    driver.begin();
    driver.toff(4);
    driver.rms_current(600);
    driver.microsteps(MICROSTEPS);
    
    digitalWrite(PIN_EN, LOW);

    // PID Setup
    myPID.SetMode(AUTOMATIC);
    myPID.SetOutputLimits(-1000, 1000); // Speed/Step frequency
    myPID.SetSampleTime(10); // 10ms = 100Hz loop

    Serial.println("Smart Servo Stepper V2 Initialized.");
}

void loop() {
    protocol.update();
    
    // Update PID
    input = encoder.readAngle();
    myPID.Compute();
    
    // Apply Output to Motor
    if (abs(output) > 5) {
        digitalWrite(PIN_DIR, output > 0 ? HIGH : LOW);
        // Simple step generation (blocking for now, should use Timer/Interrupt)
        digitalWrite(PIN_STEP, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_STEP, LOW);
    }
}
