// ═══════════════════════════════════════════════════════════════════════
// SmartServoStepperV2.ino – v2.0 Production Firmware
// ESP32-C3 Super Mini · TMC2209 (UART) · AS5600 Encoder · Feetech Bus
//
// Features:
//   ✓ Closed-Loop PID Position Control (200Hz)
//   ✓ Speed Mode (open-loop step generation)
//   ✓ TMC2209 UART: stealthChop, spreadCycle, CoolStep, StallGuard4
//   ✓ Live microstepping change (1–256)
//   ✓ AS5600 12-bit absolute encoder (I2C 400kHz)
//   ✓ Feetech STS/SCS Protocol Emulation (TTL half-duplex bus)
//   ✓ WebSocket JSON telemetry (100Hz) + command interface
//   ✓ WiFi AP fallback + mDNS (servo.local)
//   ✓ Runtime PID tuning, TMC current adjustment
//   ✓ Watchdog safety timeout
//   ✓ Non-blocking step generation via hardware timer
// ═══════════════════════════════════════════════════════════════════════
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <AS5600.h>
#include <TMCStepper.h>
#include <ArduinoJson.h>
#include "config.h"
#include "FeetechProtocol.h"

// ═══ OBJECTS ═══
AS5600 encoder(&Wire);

// TMC2209: SoftwareSerial auf PIN_TMC_UART
#include <SoftwareSerial.h>
SoftwareSerial tmcSerial;
TMC2209Stepper tmc(&tmcSerial, TMC_SENSE_RESISTOR, TMC_DRIVER_ADDR);

// Feetech Bus: UART1 → GPIO1(TX) + GPIO7(RX), 1 Mbps
FeetechProtocol feetech(Serial1, SERVO_ID);
WebSocketsServer webSocket(WS_PORT);

// ═══ STATE ═══
enum ControlMode { MODE_POSITION, MODE_SPEED };
volatile ControlMode ctrlMode = MODE_POSITION;
volatile bool motorEnabled = false;
volatile int32_t stepFreqHz = 0;         // For speed mode
volatile int32_t targetStepFreq = 0;     // Ramped target

// PID state (manual implementation – no library dependency)
float pidKp = DEFAULT_KP;
float pidKi = DEFAULT_KI;
float pidKd = DEFAULT_KD;
float pidSetpoint = 2048.0f;
float pidIntegral = 0.0f;
float pidPrevError = 0.0f;
float pidOutput = 0.0f;

// Encoder state
volatile uint16_t rawAngle = 0;
volatile float angleRps = 0.0f;  // rotations per second
uint16_t prevAngle = 0;
uint32_t prevAngleTime = 0;
int32_t totalTicks = 0;           // cumulative for multi-turn

// TMC2209 diagnostics
bool tmcConnected = false;
volatile bool tmcStall = false;  // Set by DIAG interrupt
uint8_t tmcOTWarning = 0;

// Microstep config
uint16_t microSteps = DEFAULT_MICROSTEPS;
uint32_t stepsPerRev = STEPS_PER_REV * DEFAULT_MICROSTEPS;

// Timing
uint32_t lastPidMs = 0;
uint32_t lastTeleMs = 0;
uint32_t lastWsDataMs = 0;
uint32_t lastStepUs = 0;

// ═══ STEP GENERATION (Non-blocking) ═══
// Uses micros() based timing for precise step pulses
void generateSteps() {
    if (!motorEnabled) return;

    int32_t freq = abs(stepFreqHz);
    if (freq < 5) return;  // Dead zone

    uint32_t now = micros();
    uint32_t period = 1000000UL / (uint32_t)freq;  // Full period in µs
    uint32_t halfPeriod = period / 2;

    if ((now - lastStepUs) >= halfPeriod) {
        lastStepUs = now;
        digitalWrite(PIN_STEP, !digitalRead(PIN_STEP));
    }
}

// ═══ ENCODER READING ═══
void readEncoder() {
    rawAngle = encoder.readAngle();  // 0–4095

    // Calculate velocity (RPS)
    uint32_t now = millis();
    uint32_t dt = now - prevAngleTime;
    if (dt >= 20) {  // 50Hz velocity update
        int32_t diff = (int32_t)rawAngle - (int32_t)prevAngle;
        // Handle wrap-around
        if (diff > 2048) diff -= 4096;
        if (diff < -2048) diff += 4096;

        totalTicks += diff;
        angleRps = (float)diff / ENCODER_CPR / ((float)dt / 1000.0f);

        prevAngle = rawAngle;
        prevAngleTime = now;
    }
}

// ═══ PID CONTROLLER ═══
void runPID() {
    if (ctrlMode != MODE_POSITION || !motorEnabled) {
        pidIntegral = 0;
        pidPrevError = 0;
        return;
    }

    float error = pidSetpoint - (float)rawAngle;

    // Handle wrap-around for shortest path
    if (error > 2048.0f) error -= 4096.0f;
    if (error < -2048.0f) error += 4096.0f;

    // Dead band to prevent jitter
    if (abs(error) < POSITION_DEADBAND) {
        stepFreqHz = 0;
        pidIntegral = 0;
        return;
    }

    pidIntegral += error * (PID_SAMPLE_MS / 1000.0f);
    // Anti-windup
    pidIntegral = constrain(pidIntegral, PID_OUT_MIN / pidKi, PID_OUT_MAX / pidKi);

    float derivative = (error - pidPrevError) / (PID_SAMPLE_MS / 1000.0f);
    pidPrevError = error;

    pidOutput = (pidKp * error) + (pidKi * pidIntegral) + (pidKd * derivative);
    pidOutput = constrain(pidOutput, PID_OUT_MIN, PID_OUT_MAX);

    // Convert PID output to step frequency
    stepFreqHz = (int32_t)pidOutput;
    digitalWrite(PIN_DIR, stepFreqHz >= 0 ? HIGH : LOW);
    stepFreqHz = abs(stepFreqHz);
}

// ═══ STALLGUARD DIAG INTERRUPT ═══
// DIAG-Pin (GPIO2) geht HIGH wenn TMC2209 einen Stall erkennt
void IRAM_ATTR onStallDetected() {
    tmcStall = true;
}

// ═══ TMC2209 SETUP ═══
void setupTMC() {
    Serial.println("[TMC] Initialisiere SoftwareSerial (RX:5, TX:6)...");
    tmcSerial.begin(19200, SWSERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
    
    // Kurzes Blinken um Aktivität zu zeigen
    digitalWrite(PIN_LED, LOW); delay(50); digitalWrite(PIN_LED, HIGH);

    Serial.println("[TMC] Teste Verbindung (Adresse 0)...");
    uint8_t ver = 0;
    
    // Versuche Version zu lesen
    for (int i = 0; i < 5; i++) {
        while(tmcSerial.available()) tmcSerial.read();
        ver = tmc.version();
        if (ver == 0x21) {
            tmcConnected = true;
            break;
        }
        delay(50);
    }

    if (!tmcConnected) {
        Serial.println("[TMC] OFFLINE. Prüfe PDN_UART Pin am TMC.");
        return;
    }
    
    tmc.begin();
    tmc.pdn_disable(true);      // Aktiviert UART-Modus
    tmc.mstep_reg_select(true); // Nutzt Register für Microsteps
    
    Serial.println("[TMC] Verbunden! Version: 0x" + String(ver, HEX));

    // ─ Basic Configuration ─
    tmc.toff(4);                        // Enable driver (toff > 0)
    tmc.rms_current(TMC_RMS_CURRENT);   // Set motor current
    tmc.microsteps(microSteps);         // Microstepping resolution
    tmc.blank_time(24);                 // Comparator blank time

    // ─ StealthChop2 (ultra-quiet at low speeds) ─
    tmc.en_spreadCycle(false);          // stealthChop als Default
    tmc.pwm_autoscale(true);            // Auto-Tune PWM Amplitude
    tmc.pwm_autograd(true);             // Auto-Tune PWM Gradient
    tmc.TPWMTHRS(TMC_TPWM_THRESHOLD);  // Umschaltung zu spreadCycle bei hoher Geschwindigkeit

    // ─ StallGuard4 (sensorlose Blockadeerkennung) ─
    tmc.SGTHRS(TMC_STALL_THRESHOLD);    // Schwellwert (niedriger = empfindlicher)
    tmc.TCOOLTHRS(0xFFFFF);             // SG aktiv oberhalb dieser Geschwindigkeit

    // ─ DIAG Pin: Hardware-Interrupt für Echtzeit-Stallerkennung ─
    pinMode(PIN_DIAG, INPUT);
    // attachInterrupt(digitalPinToInterrupt(PIN_DIAG), onStallDetected, RISING);

    // ─ CoolStep (dynamische Stromreduzierung nach Last) ─
    tmc.semin(TMC_COOLSTEP_LOWER);      // Untere CoolStep-Schwelle
    tmc.semax(TMC_COOLSTEP_UPPER);      // Obere CoolStep-Schwelle
    tmc.sedn(0b01);                     // Strom-Dekrement-Geschwindigkeit

    // ─ MicroPlyer Interpolation ─
    tmc.intpol(true);                   // Intern auf 256 µSteps interpolieren

    Serial.println("[TMC] Konfiguriert: " + String(TMC_RMS_CURRENT) + "mA, " +
                   String(microSteps) + " µSteps, stealthChop+CoolStep+StallGuard aktiv");
}

// ═══ TMC2209 DIAGNOSTICS ═══
void readTMCDiag() {
    if (!tmcConnected) return;

    // StallGuard result
    uint16_t sgResult = tmc.SG_RESULT();
    tmcStall = (sgResult < 10);  // Very low = stall detected

    // Thermal flags
    tmcOTWarning = tmc.otpw() ? 1 : 0;
    if (tmc.ot()) tmcOTWarning = 2;  // Over-temperature shutdown

    // Auto-disable on overtemp
    if (tmcOTWarning >= 2 && motorEnabled) {
        motorEnabled = false;
        digitalWrite(PIN_EN, HIGH);
        Serial.println("[TMC] ÜBERTEMPERATUR! Motor deaktiviert.");
    }
}

// ═══ WEBSOCKET HANDLER ═══
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
        Serial.println("[WS] Client #" + String(num) + " verbunden");
    }
    else if (type == WStype_DISCONNECTED) {
        Serial.println("[WS] Client #" + String(num) + " getrennt");
    }
    else if (type == WStype_TEXT) {
        lastWsDataMs = millis();

        JsonDocument doc;
        if (deserializeJson(doc, payload, length)) return;

        // ── Mode & Value commands ──
        if (doc.containsKey("mode")) {
            const char* mode = doc["mode"];
            if (strcmp(mode, "pos") == 0) {
                ctrlMode = MODE_POSITION;
                pidSetpoint = doc["val"].as<float>();
                pidIntegral = 0;
            } else if (strcmp(mode, "speed") == 0) {
                ctrlMode = MODE_SPEED;
                int32_t hz = doc["val"].as<int32_t>();
                hz = constrain(hz, -MAX_STEP_FREQ, MAX_STEP_FREQ);
                stepFreqHz = abs(hz);
                digitalWrite(PIN_DIR, hz >= 0 ? HIGH : LOW);
            }
        }

        // ── Direct commands ──
        if (doc.containsKey("cmd")) {
            String cmd = doc["cmd"].as<String>();

            if (cmd == "enable") {
                motorEnabled = true;
                digitalWrite(PIN_EN, LOW);
            }
            else if (cmd == "kill") {
                motorEnabled = false;
                stepFreqHz = 0;
                pidIntegral = 0;
                digitalWrite(PIN_EN, HIGH);
            }
            else if (cmd == "tare") {
                totalTicks = 0;
            }
            else if (cmd == "set_microstep") {
                uint8_t val = doc["value"].as<uint8_t>();
                uint16_t msMap[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
                if (val < 9) {
                    microSteps = msMap[val];
                    stepsPerRev = STEPS_PER_REV * microSteps;
                    if (tmcConnected) {
                        tmc.microsteps(microSteps);
                    }
                }
            }
            else if (cmd == "set_current") {
                uint16_t mA = doc["value"].as<uint16_t>();
                mA = constrain(mA, 100, 2000);
                if (tmcConnected) tmc.rms_current(mA);
            }
            else if (cmd == "set_pid") {
                pidKp = doc["kp"].as<float>();
                pidKi = doc["ki"].as<float>();
                pidKd = doc["kd"].as<float>();
                pidIntegral = 0;
                pidPrevError = 0;
            }
            else if (cmd == "set_stealthchop") {
                bool sc = doc["value"].as<bool>();
                if (tmcConnected) tmc.en_spreadCycle(!sc);
            }
            else if (cmd == "set_stallguard") {
                uint8_t thr = doc["value"].as<uint8_t>();
                if (tmcConnected) tmc.SGTHRS(thr);
            }
            else if (cmd == "set_ttl_id") {
                uint8_t newId = doc["value"].as<uint8_t>();
                if (newId >= 1 && newId <= 253) {
                    feetech.setId(newId);
                    // Send ACK back
                    String ack = "{\"ttl_ack\":" + String(newId) + "}";
                    webSocket.sendTXT(num, ack);
                }
            }
            else if (cmd == "get_ttl_id") {
                String resp = "{\"ttl_id\":" + String(feetech.getId()) + "}";
                webSocket.sendTXT(num, resp);
            }
        }
    }
}

// ═══ TELEMETRY ═══
void sendTelemetry() {
    if (webSocket.connectedClients() == 0) return;

    float degrees = rawAngle / 4096.0f * 360.0f;

    JsonDocument doc;
    doc["pos"]  = rawAngle;
    doc["deg"]  = (int)degrees;
    doc["rps"]  = angleRps;
    doc["pid"]  = (int)pidOutput;
    doc["sp"]   = (int)pidSetpoint;
    doc["err"]  = (int)(pidSetpoint - rawAngle);
    doc["k"]    = motorEnabled ? 0 : 1;
    doc["hz"]   = stepFreqHz;
    doc["mode"] = (ctrlMode == MODE_POSITION) ? "pos" : "speed";

    // TMC diagnostics
    if (tmcConnected) {
        doc["tmc"]  = (tmcOTWarning == 0) ? "OK" : (tmcOTWarning == 1 ? "WARM" : "OT!");
        doc["sg"]   = tmc.SG_RESULT();
        doc["cs"]   = tmc.cs_actual();
        doc["stall"] = tmcStall ? 1 : 0;
    } else {
        doc["tmc"] = "N/A";
    }

    String json;
    serializeJson(doc, json);
    webSocket.broadcastTXT(json);
}

// ═══ FEETECH CALLBACKS ═══
void onFeetechGoalPos(int16_t pos, uint16_t time, uint16_t speed) {
    ctrlMode = MODE_POSITION;
    pidSetpoint = constrain((float)pos, 0.0f, 4095.0f);
    pidIntegral = 0;
    Serial.println("[FT] GoalPos=" + String(pos));
}

void onFeetechTorque(bool enable) {
    motorEnabled = enable;
    digitalWrite(PIN_EN, enable ? LOW : HIGH);
    if (!enable) { stepFreqHz = 0; pidIntegral = 0; }
    Serial.println("[FT] Torque=" + String(enable));
}

void onFeetechIdChange(uint8_t newId) {
    Serial.println("[FT] ID geändert → " + String(newId));
}

// ═══ SETUP ═══
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n═══ SmartServoStepperV2 v2.0 ═══");

    // ── GPIO ──
    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_DIR, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_EN, HIGH);   // Motor OFF at boot (R3 10kΩ Pull-Up)
    digitalWrite(PIN_STEP, LOW);
    digitalWrite(PIN_DIR, HIGH);
    digitalWrite(PIN_LED, HIGH);  // LED off (Active LOW)

    // ── I2C → AS5600 ──
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);  // Fast mode for quick encoder reads
    encoder.begin();
    if (encoder.isConnected()) {
        Serial.println("[AS5600] Verbunden (Magnet " +
                       String(encoder.magnetTooStrong() ? "STARK" :
                              encoder.magnetTooWeak() ? "SCHWACH" : "OK") + ")");
    } else {
        Serial.println("[AS5600] WARNUNG: Nicht gefunden!");
    }
    rawAngle = encoder.readAngle();
    prevAngle = rawAngle;
    pidSetpoint = rawAngle;  // Start at current position
    prevAngleTime = millis();

    // ── TMC2209 ──
    setupTMC();

    // ── Feetech Bus (Half-Duplex, 1 Mbps) ──
    Serial.println("[FT] Initialisiere UART1 (GPIO1/7)...");
    Serial1.begin(FT_BAUDRATE, SERIAL_8N1, PIN_FT_RX, PIN_FT_TX);
    feetech.onWritePos(onFeetechGoalPos);
    feetech.onTorque(onFeetechTorque);
    feetech.onIdChange(onFeetechIdChange);

    // ── WiFi ──
    Serial.println("[WiFi] Starte WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("[WiFi] Verbinde");
    uint32_t wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart) < 10000) {
        delay(250);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Verbunden: " + WiFi.localIP().toString());
    } else {
        // Fallback: Start AP
        Serial.println("\n[WiFi] STA fehlgeschlagen → starte AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("SmartServoV2", "12345678");
        Serial.println("[WiFi] AP: SmartServoV2 @ " + WiFi.softAPIP().toString());
    }

    // ── mDNS ──
    if (MDNS.begin("servo")) {
        Serial.println("[mDNS] http://servo.local");
    }

    // ── WebSocket ──
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
    Serial.println("[WS] Port " + String(WS_PORT) + " bereit");

    lastWsDataMs = millis();
    Serial.println("═══ System bereit ═══\n");
}

// ═══ MAIN LOOP ═══
void loop() {
    uint32_t now = millis();

    // ── WebSocket ──
    webSocket.loop();

    // ── Feetech Bus ──
    feetech.update();
    feetech.setPresentPosition(rawAngle);
    feetech.setPresentSpeed((int16_t)(angleRps * 100));

    // ── Encoder ──
    readEncoder();

    // ── PID (200 Hz) ──
    if ((now - lastPidMs) >= PID_SAMPLE_MS) {
        lastPidMs = now;
        runPID();
    }

    // ── Step Generation (non-blocking) ──
    generateSteps();

    // ── Telemetry (100 Hz) ──
    if ((now - lastTeleMs) >= TELEMETRY_INTERVAL_MS) {
        lastTeleMs = now;
        sendTelemetry();
    }

    // ── TMC Diagnostics (every 500ms) ──
    static uint32_t lastDiagMs = 0;
    if ((now - lastDiagMs) >= 500) {
        lastDiagMs = now;
        readTMCDiag();
    }

    // ── Watchdog: Kill motor if no WS commands for timeout ──
    if (motorEnabled && (now - lastWsDataMs) > WATCHDOG_TIMEOUT_MS) {
        // Only kill if in speed mode (position mode should hold)
        if (ctrlMode == MODE_SPEED) {
            stepFreqHz = 0;
            Serial.println("[WDG] Speed-Mode Timeout → Motor gestoppt");
        }
    }

    // ── Status-LED Heartbeat (GPIO8, Active LOW) ──
    static uint32_t lastLedMs = 0;
    if ((now - lastLedMs) >= 500) {
        lastLedMs = now;
        digitalWrite(PIN_LED, !digitalRead(PIN_LED));
    }

    // ── Echtzeit-Diagnose via Serial (alle 2s) ──
    static uint32_t lastDbgMs = 0;
    if ((now - lastDbgMs) >= 2000) {
        lastDbgMs = now;
        Serial.print("[DIAG] TMC: ");
        Serial.print(tmcConnected ? "VERBUNDEN" : "OFFLINE!");
        Serial.print(" | Magnet: ");
        Serial.print(rawAngle);
        Serial.print(" | Motor: ");
        Serial.println(motorEnabled ? "AKTIV" : "AUS");
    }
}
