// ═══════════════════════════════════════════════════════════
// SmartServoStepperV2 - OPTIMAL STEPPER DRIVER (DIRECT FREQUENCY / CNC)
// ═══════════════════════════════════════════════════════════
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <TMCStepper.h>
#include <Wire.h>
#include <AS5600.h>
#include "FastAccelStepper.h"

// --- PINS ---
#define PIN_TMC_RX      5   
#define PIN_TMC_TX      6   
#define PIN_EN          3   
#define PIN_STEP        4   
#define PIN_DIR         2   
#define PIN_SDA         0   
#define PIN_SCL         10  
#define PIN_LED         8   

const char* st_ssid = "FRITZ!Box 6660 Cable UE";
const char* st_pass = "28370714691864306613";
IPAddress local_ip(192, 168, 178, 78);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

WebSocketsServer webSocket = WebSocketsServer(81);
HardwareSerial tmcSerial(1);
TMC2209Stepper driver(&tmcSerial, 0.11f, 0);
AS5600 as5600;

FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

long targetPos = 2048;
long currentSpeed = 0;
String controlMode = "pos"; 
bool isKilled = true;
float currentRPS = 0;
float lastAngle = 0;
unsigned long lastTime = 0;

// Konstanten zur Umrechnung (AS5600 -> Motor-Schritte)
const float STEPS_PER_REV = 3200.0; // 200 Vollschritte * 16 Microsteps
const float AS5600_PER_REV = 4096.0;
const float RATIO = STEPS_PER_REV / AS5600_PER_REV;

void handleWebSocket(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        JsonDocument doc; 
        deserializeJson(doc, payload);
        
        if (doc["mode"].is<const char*>()) {
            controlMode = doc["mode"].as<String>();
        }
        
        if (doc["val"].is<long>()) {
            long val = doc["val"];
            if (controlMode == "pos") {
                targetPos = val;
                
                // Berechne den kürzesten Weg zum neuen Ziel
                float currentPos = as5600.readAngle();
                float diff = targetPos - currentPos;
                if (diff > 2048) diff -= 4096;
                if (diff < -2048) diff += 4096;
                
                // Rechne in Schritte um und adde zum aktuellen STEPPER-Ziel
                long stepsToMove = diff * RATIO;
                if (stepper && !isKilled) {
                    stepper->move(stepsToMove); 
                }
            }
            else {
                currentSpeed = val;
                if (stepper && !isKilled) {
                    if (currentSpeed == 0) {
                        stepper->stopMove();
                    } else {
                        stepper->setSpeedInHz(abs(currentSpeed * 20));
                        if (currentSpeed > 0) stepper->runForward();
                        else stepper->runBackward();
                    }
                }
            }
        }
        
        if (doc["cmd"].is<const char*>()) {
            String cmd = doc["cmd"].as<String>();
            if (cmd == "kill") { 
                isKilled = true; 
                if (stepper) stepper->stopMove(); 
                digitalWrite(PIN_EN, HIGH); 
            }
            else if (cmd == "enable") { 
                isKilled = false; 
                digitalWrite(PIN_EN, LOW); 
                if(stepper) stepper->setCurrentPosition(0); 
            }
            else if (cmd == "set_microstep") { driver.microsteps(1 << doc["value"].as<int>()); }
            else if (cmd == "tare") { targetPos = as5600.readAngle(); }
        }
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.config(local_ip, gateway, subnet);
    WiFi.begin(st_ssid, st_pass);
    
    // Hardware Serial auf Pins 5 und 6
    tmcSerial.begin(115200, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
    
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000); 
    
    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_EN, HIGH);
    
    // TMC2209 Optimal Setup
    driver.begin();
    driver.pdn_disable(true);
    driver.I_scale_analog(false);
    driver.mstep_reg_select(true);
    driver.rms_current(1000); 
    driver.en_spreadCycle(true);
    driver.microsteps(16);
    driver.blank_time(24);
    driver.toff(4);
    
    // Hardware Timer initialisieren für Stepper
    engine.init();
    stepper = engine.stepperConnectToPin(PIN_STEP);
    if (stepper) {
        stepper->setDirectionPin(PIN_DIR);
        stepper->setAutoEnable(false); 
        
        // Direkte Frequenz-Methode Parameter:
        stepper->setSpeedInHz(40000); // Maximale Reisegeschwindigkeit
        stepper->setAcceleration(25000); // Saubere Beschleunigungsrampe (Anti-Stall)
    }
    
    webSocket.begin();
    webSocket.onEvent(handleWebSocket);
    lastAngle = as5600.readAngle();
    lastTime = micros();
}

void loop() {
    webSocket.loop();
    unsigned long now = micros();
    float dt = (now - lastTime) / 1000000.0;
    
    // Sensor nur noch für Telemetrie auslesen
    if (dt >= 0.02) { 
        float currentPos = as5600.readAngle();
        float diff = currentPos - lastAngle;
        if (diff > 2048) diff -= 4096;
        if (diff < -2048) diff += 4096;
        currentRPS = (diff / 4096.0) / dt;
        lastAngle = currentPos;
        lastTime = now;
    }
    
    static uint32_t lastTele = 0;
    if (millis() - lastTele > 50) { 
        lastTele = millis();
        JsonDocument out; 
        out["pos"] = as5600.readAngle();
        out["rps"] = abs(currentRPS); 
        out["tmc"] = (driver.version() == 0x21) ? "OK" : "ERR";
        out["k"] = isKilled ? 1 : 0;
        out["sp"] = targetPos;
        out["err"] = 0; // Fehler gibts nicht mehr, da offener Kreis
        out["pid"] = 0;
        
        String json;
        serializeJson(out, json);
        webSocket.broadcastTXT(json);
        digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    }
}
