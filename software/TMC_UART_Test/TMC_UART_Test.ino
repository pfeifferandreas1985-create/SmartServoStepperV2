#include <Arduino.h>
#include <TMCStepper.h>
#include <SoftwareSerial.h>

// NEUE SICHERE PINS
#define STEP_PIN    18
#define DIR_PIN     19
#define EN_PIN      3       
#define RX_PIN      5       
#define TX_PIN      6       
#define R_SENSE     0.11f   
#define DRIVER_ADDRESS 0b00 
#define LED_PIN     8 

SoftwareSerial tmcSerial;
TMC2209Stepper driver(&tmcSerial, R_SENSE, DRIVER_ADDRESS);

void setup() {
    Serial.begin(115200);
    delay(2000);
    pinMode(LED_PIN, OUTPUT);
    
    Serial.println("\n\n=== TMC2209 SICHERER TEST (GPIO 5/6) ===");
    
    tmcSerial.begin(19200, SWSERIAL_8N1, RX_PIN, TX_PIN);
    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, LOW);
    
    driver.begin();
    driver.pdn_disable(true);
    driver.mstep_reg_select(true);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    
    uint8_t v = driver.version();
    Serial.print("TMC Version: 0x");
    if (v < 0x10) Serial.print("0");
    Serial.println(v, HEX);
    
    if (v == 0x21) {
        Serial.println(">>> ERFOLG! CHIP AN PINS 5/6 GEFUNDEN! <<<");
    }
    
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
}
