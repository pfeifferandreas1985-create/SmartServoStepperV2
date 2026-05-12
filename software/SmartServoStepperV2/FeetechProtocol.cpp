#include "FeetechProtocol.h"

FeetechProtocol::FeetechProtocol(HardwareSerial& serial, uint8_t id) : _serial(serial), _id(id), _index(0) {}

void FeetechProtocol::begin(long baud) {
    _serial.begin(baud);
}

void FeetechProtocol::update() {
    while (_serial.available()) {
        uint8_t b = _serial.read();
        if (_index == 0 && b != 0xFF) continue;
        if (_index == 1 && b != 0xFF) { _index = 0; continue; }
        
        _buffer[_index++] = b;
        
        if (_index > 3) {
            uint8_t len = _buffer[3];
            if (_index == len + 4) {
                processPacket();
                _index = 0;
            }
        }
        if (_index >= sizeof(_buffer)) _index = 0;
    }
}

void FeetechProtocol::processPacket() {
    uint8_t id = _buffer[2];
    if (id != _id && id != 0xFE) return; // FE is broadcast
    
    uint8_t len = _buffer[3];
    uint8_t instr = _buffer[4];
    uint8_t checksum = _buffer[len + 3];
    
    if (calculateChecksum(&_buffer[2], len + 1) != checksum) return;
    
    if (instr == 0x03) { // WRITE_DATA
        uint8_t addr = _buffer[5];
        if (addr == 0x2A) { // Goal Position
            int16_t pos = (_buffer[6] << 8) | _buffer[7];
            if (_writePosCallback) _writePosCallback(pos, 0, 0);
        }
    }
}

uint8_t FeetechProtocol::calculateChecksum(uint8_t* data, uint8_t length) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < length; i++) sum += data[i];
    return ~(sum & 0xFF);
}

void FeetechProtocol::onWritePos(void (*callback)(int16_t pos, uint16_t time, uint16_t speed)) {
    _writePosCallback = callback;
}
