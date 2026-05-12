#ifndef FEETECH_PROTOCOL_H
#define FEETECH_PROTOCOL_H

#include <Arduino.h>

class FeetechProtocol {
public:
    FeetechProtocol(HardwareSerial& serial, uint8_t id);
    void begin(long baud);
    void update();
    
    // Callbacks for the main app to handle commands
    void onWritePos(void (*callback)(int16_t pos, uint16_t time, uint16_t speed));
    
private:
    HardwareSerial& _serial;
    uint8_t _id;
    void (*_writePosCallback)(int16_t pos, uint16_t time, uint16_t speed);
    
    uint8_t _buffer[64];
    uint8_t _index;
    
    void processPacket();
    uint8_t calculateChecksum(uint8_t* data, uint8_t length);
};

#endif
