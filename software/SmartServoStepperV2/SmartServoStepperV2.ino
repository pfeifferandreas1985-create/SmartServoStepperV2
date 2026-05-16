// ═══════════════════════════════════════════════════════════
// SmartServoStepperV2 - FINAL PRODUCTION v2.9.2
// ═══════════════════════════════════════════════════════════
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <TMCStepper.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <AS5600.h>

// --- PINS (v2.5 Stable) ---
#define PIN_TMC_RX      5   
#define PIN_TMC_TX      6   
#define PIN_EN          3   
#define PIN_SDA         0   
#define PIN_SCL         10  
#define PIN_LED         8   

// --- WIFI (FritzBox Hybrid) ---
const char* st_ssid = "FRITZ!Box 6660 Cable UE";
const char* st_pass = "28370714691864306613";
IPAddress local_ip(192, 168, 178, 78);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

WebSocketsServer webSocket = WebSocketsServer(81);
SoftwareSerial tmcSerial(PIN_TMC_RX, PIN_TMC_TX);
TMC2209Stepper driver(&tmcSerial, 0.11f, 0);
AS5600 as5600;

float Kp = 5.0, Ki = 0.2, Kd = 0.5; 
float integral = 0, lastError = 0;
long targetPos = 2048;
long currentSpeed = 0;
String controlMode = "pos"; 
bool isKilled = true;
float currentRPS = 0;
float lastAngle = 0;
unsigned long lastTime = 0;

void handleWebSocket(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        JsonDocument doc; 
        deserializeJson(doc, payload);
        if (doc["mode"].is<const char*>()) {
            controlMode = doc["mode"].as<String>();
            if (doc["val"].is<long>()) {
                if (controlMode == "pos") targetPos = doc["val"];
                else currentSpeed = doc["val"];
            }
        }
        if (doc["cmd"].is<const char*>()) {
            String cmd = doc["cmd"].as<String>();
            if (cmd == "kill") { isKilled = true; digitalWrite(PIN_EN, HIGH); }
            else if (cmd == "enable") { isKilled = false; digitalWrite(PIN_EN, LOW); }
            else if (cmd == "set_pid") { Kp = doc["kp"]; Ki = doc["ki"]; Kd = doc["kd"]; integral = 0; }
            else if (cmd == "set_microstep") { driver.microsteps(1 << doc["value"].as<int>()); }
            else if (cmd == "tare") { targetPos = as5600.readAngle(); }
        }
    }
}

void setup() {
    delay(3000);
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.config(local_ip, gateway, subnet);
    WiFi.begin(st_ssid, st_pass);
    
    tmcSerial.begin(19200);
    Wire.begin(PIN_SDA, PIN_SCL);
    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_EN, HIGH);
    
    driver.begin();
    driver.pdn_disable(true);
    driver.mstep_reg_select(true);
    driver.rms_current(1000); 
    driver.en_spreadCycle(true);
    driver.microsteps(16);
    
    webSocket.begin();
    webSocket.onEvent(handleWebSocket);
    lastAngle = as5600.readAngle();
    lastTime = millis();
}

void loop() {
    webSocket.loop();
    unsigned long now = millis();
    float dt = (now - lastTime) / 1000.0;
    
    if (dt >= 0.05) {
        float currAngle = as5600.readAngle();
        float diff = currAngle - lastAngle;
        if (diff > 2048) diff -= 4096;
        if (diff < -2048) diff += 4096;
        currentRPS = (diff / 4096.0) / dt;
        lastAngle = currAngle;
        lastTime = now;
    }

    if (!isKilled) {
        if (controlMode == "speed") {
            driver.VACTUAL(currentSpeed * 2); 
        } else {
            float currentPos = as5600.readAngle();
            float error = targetPos - currentPos;
            if (error > 2048) error -= 4096;
            if (error < -2048) error += 4096;
            
            integral += error * dt;
            integral = constrain(integral, -1000, 1000); 
            float derivative = (error - lastError) / dt;
            float output = (error * Kp) + (integral * Ki) + (derivative * Kd);
            lastError = error;
            
            if (abs(error) < 5) {
                driver.VACTUAL(0);
            } else {
                driver.VACTUAL(constrain(output * 400, -120000, 120000));
            }
        }
    } else {
        driver.VACTUAL(0);
        integral = 0;
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
        out["err"] = lastError;
        out["pid"] = (int)(lastError * Kp);
        
        String json;
        serializeJson(out, json);
        webSocket.broadcastTXT(json);
        digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    }
}
