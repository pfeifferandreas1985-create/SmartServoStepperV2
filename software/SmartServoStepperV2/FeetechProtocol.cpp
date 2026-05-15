// ═══════════════════════════════════════════════════════════
// SmartServoStepperV2 – FeetechProtocol.cpp
// Full STS/SCS Protocol with PING, READ, WRITE, ID change
// ═══════════════════════════════════════════════════════════
#include "FeetechProtocol.h"

FeetechProtocol::FeetechProtocol(HardwareSerial& serial, uint8_t id)
    : _serial(serial), _id(id) {}

void FeetechProtocol::setId(uint8_t id) { _id = id; }

void FeetechProtocol::update() {
    while (_serial.available()) {
        uint8_t b = _serial.read();
        uint32_t now = millis();

        // Reset buffer if >10ms gap between bytes (new packet)
        if (_index > 0 && (now - _lastByteTime) > 10) { _index = 0; }
        _lastByteTime = now;

        // Header sync: expect 0xFF 0xFF
        if (_index == 0 && b != 0xFF) continue;
        if (_index == 1 && b != 0xFF) { _index = 0; continue; }

        _buffer[_index++] = b;

        // Check if packet is complete: [0xFF][0xFF][ID][LEN][...][CHK]
        if (_index > 3) {
            uint8_t len = _buffer[3]; // Length = num params + 2
            if (_index == (uint8_t)(len + 4)) {
                processPacket();
                _index = 0;
            }
        }
        // Buffer overflow protection
        if (_index >= sizeof(_buffer)) _index = 0;
    }
}

void FeetechProtocol::processPacket() {
    uint8_t pktId   = _buffer[2];
    uint8_t len     = _buffer[3];
    uint8_t instr   = _buffer[4];
    uint8_t chkRecv = _buffer[len + 3];

    // Verify checksum (over ID + LEN + params, NOT header)
    uint8_t chkCalc = calcChecksum(&_buffer[2], len + 1);
    if (chkCalc != chkRecv) return;

    // Only respond if addressed to us or broadcast (0xFE)
    if (pktId != _id && pktId != 0xFE) return;

    switch (instr) {
        case FT_INST_PING: {
            // Respond with empty status packet (error = 0)
            sendStatusPacket(0x00, nullptr, 0);
            break;
        }

        case FT_INST_READ: {
            if (len < 4) break; // Need addr + read_length
            uint8_t addr    = _buffer[5];
            uint8_t readLen = _buffer[6];
            uint8_t resp[8] = {0};
            uint8_t respLen = 0;

            // Build response data based on address
            if (addr == FT_ADDR_PRESENT_POS_H && readLen >= 2) {
                resp[0] = (_presentPos >> 8) & 0xFF;
                resp[1] = _presentPos & 0xFF;
                respLen = 2;
            } else if (addr == FT_ADDR_PRESENT_SPEED_H && readLen >= 2) {
                resp[0] = (_presentSpeed >> 8) & 0xFF;
                resp[1] = _presentSpeed & 0xFF;
                respLen = 2;
            } else if (addr == FT_ADDR_PRESENT_LOAD_H && readLen >= 2) {
                resp[0] = (_presentLoad >> 8) & 0xFF;
                resp[1] = _presentLoad & 0xFF;
                respLen = 2;
            } else if (addr == FT_ADDR_PRESENT_VOLTAGE) {
                resp[0] = _presentVoltage;
                respLen = 1;
            } else if (addr == FT_ADDR_PRESENT_TEMP) {
                resp[0] = _presentTemp;
                respLen = 1;
            } else if (addr == FT_ADDR_ID) {
                resp[0] = _id;
                respLen = 1;
            } else {
                // Read from addr block for position+speed+load+volt+temp
                if (addr == FT_ADDR_PRESENT_POS_H && readLen >= 8) {
                    resp[0] = (_presentPos >> 8) & 0xFF;
                    resp[1] = _presentPos & 0xFF;
                    resp[2] = (_presentSpeed >> 8) & 0xFF;
                    resp[3] = _presentSpeed & 0xFF;
                    resp[4] = (_presentLoad >> 8) & 0xFF;
                    resp[5] = _presentLoad & 0xFF;
                    resp[6] = _presentVoltage;
                    resp[7] = _presentTemp;
                    respLen = 8;
                } else {
                    respLen = readLen;  // Return zeros
                }
            }

            if (pktId != 0xFE) { // Don't respond to broadcast READs
                sendStatusPacket(0x00, resp, respLen);
            }
            break;
        }

        case FT_INST_WRITE: {
            if (len < 3) break;
            uint8_t addr = _buffer[5];

            if (addr == FT_ADDR_GOAL_POS_H) {
                int16_t pos = ((int16_t)_buffer[6] << 8) | _buffer[7];
                uint16_t time  = (len >= 6) ? ((_buffer[8] << 8) | _buffer[9]) : 0;
                uint16_t speed = (len >= 8) ? ((_buffer[10] << 8) | _buffer[11]) : 0;
                if (_writePosCallback) _writePosCallback(pos, time, speed);
            }
            else if (addr == FT_ADDR_TORQUE_ENABLE) {
                bool enable = (_buffer[6] != 0);
                if (_torqueCallback) _torqueCallback(enable);
            }
            else if (addr == FT_ADDR_ID) {
                uint8_t newId = _buffer[6];
                if (newId >= 1 && newId <= 253) {
                    _id = newId;
                    if (_idChangeCallback) _idChangeCallback(newId);
                }
            }

            // ACK
            if (pktId != 0xFE) {
                sendStatusPacket(0x00, nullptr, 0);
            }
            break;
        }

        default:
            break;
    }
}

void FeetechProtocol::sendStatusPacket(uint8_t error, const uint8_t* data, uint8_t dataLen) {
    // Status packet: [0xFF][0xFF][ID][LEN][ERROR][...DATA...][CHK]
    uint8_t pktLen = dataLen + 2; // error + params + chk counted differently
    uint8_t pkt[64];
    uint8_t idx = 0;

    pkt[idx++] = 0xFF;
    pkt[idx++] = 0xFF;
    pkt[idx++] = _id;
    pkt[idx++] = pktLen;
    pkt[idx++] = error;

    for (uint8_t i = 0; i < dataLen; i++) {
        pkt[idx++] = data[i];
    }

    // Checksum over [ID + LEN + ERROR + DATA]
    pkt[idx] = calcChecksum(&pkt[2], pktLen + 1);
    idx++;

    _serial.write(pkt, idx);
    _serial.flush();
}

uint8_t FeetechProtocol::calcChecksum(const uint8_t* data, uint8_t length) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < length; i++) sum += data[i];
    return ~(sum & 0xFF);
}
