// ═══════════════════════════════════════════════════════════
// SmartServoStepperV2 – FeetechProtocol.h
// Feetech STS/SCS Half-Duplex Servo Bus Protocol Emulation
// Supports: PING, READ, WRITE, REG_WRITE, ACTION, SYNC_WRITE
// ═══════════════════════════════════════════════════════════
#ifndef FEETECH_PROTOCOL_H
#define FEETECH_PROTOCOL_H

#include <Arduino.h>

// ─── Instruction Set ───
#define FT_INST_PING       0x01
#define FT_INST_READ       0x02
#define FT_INST_WRITE      0x03
#define FT_INST_REG_WRITE  0x04
#define FT_INST_ACTION     0x05
#define FT_INST_RESET      0x06
#define FT_INST_SYNC_WRITE 0x83

// ─── Control Table Addresses (STS/SCS compatible) ───
#define FT_ADDR_ID              0x05
#define FT_ADDR_BAUD            0x06
#define FT_ADDR_GOAL_POS_H      0x2A
#define FT_ADDR_GOAL_POS_L      0x2B
#define FT_ADDR_GOAL_TIME_H     0x2C
#define FT_ADDR_GOAL_TIME_L     0x2D
#define FT_ADDR_GOAL_SPEED_H    0x2E
#define FT_ADDR_GOAL_SPEED_L    0x2F
#define FT_ADDR_TORQUE_ENABLE   0x28
#define FT_ADDR_PRESENT_POS_H   0x38
#define FT_ADDR_PRESENT_POS_L   0x39
#define FT_ADDR_PRESENT_SPEED_H 0x3A
#define FT_ADDR_PRESENT_SPEED_L 0x3B
#define FT_ADDR_PRESENT_LOAD_H  0x3C
#define FT_ADDR_PRESENT_LOAD_L  0x3D
#define FT_ADDR_PRESENT_VOLTAGE 0x3E
#define FT_ADDR_PRESENT_TEMP    0x3F
#define FT_ADDR_LOCK            0x30

// ─── Callback Types ───
typedef void (*WritePosCallback)(int16_t pos, uint16_t time, uint16_t speed);
typedef void (*TorqueCallback)(bool enable);
typedef void (*IdChangeCallback)(uint8_t newId);

class FeetechProtocol {
public:
    FeetechProtocol(HardwareSerial& serial, uint8_t id);

    void update();
    void setId(uint8_t id);
    uint8_t getId() const { return _id; }

    // Register callbacks
    void onWritePos(WritePosCallback cb)   { _writePosCallback = cb; }
    void onTorque(TorqueCallback cb)       { _torqueCallback = cb; }
    void onIdChange(IdChangeCallback cb)   { _idChangeCallback = cb; }

    // Set present values for READ responses
    void setPresentPosition(int16_t pos)   { _presentPos = pos; }
    void setPresentSpeed(int16_t speed)     { _presentSpeed = speed; }
    void setPresentLoad(int16_t load)       { _presentLoad = load; }
    void setPresentVoltage(uint8_t voltage) { _presentVoltage = voltage; }
    void setPresentTemp(uint8_t temp)       { _presentTemp = temp; }

private:
    HardwareSerial& _serial;
    uint8_t _id;

    // Callbacks
    WritePosCallback _writePosCallback = nullptr;
    TorqueCallback   _torqueCallback   = nullptr;
    IdChangeCallback _idChangeCallback = nullptr;

    // Present values for READ responses
    int16_t _presentPos   = 0;
    int16_t _presentSpeed = 0;
    int16_t _presentLoad  = 0;
    uint8_t _presentVoltage = 120; // 12.0V
    uint8_t _presentTemp  = 25;

    // RX buffer
    uint8_t _buffer[128];
    uint8_t _index = 0;
    uint32_t _lastByteTime = 0;

    void processPacket();
    void sendStatusPacket(uint8_t error, const uint8_t* data, uint8_t dataLen);
    uint8_t calcChecksum(const uint8_t* data, uint8_t length);
};

#endif // FEETECH_PROTOCOL_H
