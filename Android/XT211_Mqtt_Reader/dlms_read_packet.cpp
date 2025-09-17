#include <Arduino.h>
#include <vector>
#include "dlms_read_packet.h"

// Čte paket začínající 0x0F, končí při timeoutu (v ms)
std::vector<uint8_t> read_packet(HardwareSerial &serial, uint32_t timeout_ms) {
    std::vector<uint8_t> packet;
    uint32_t start = millis();
    bool started = false;

    while (true) {
        if (serial.available()) {
            uint8_t b = serial.read();

            if (!started) {
                if (b == 0x0F) {          // začátek paketu
                    started = true;
                    packet.push_back(b);
                    start = millis();     // reset timeoutu
                }
            } else {
                packet.push_back(b);
                start = millis();         // reset timeoutu při každém bajtu
            }
        } else {
            if (started && (millis() - start > timeout_ms)) {
                // konec paketu
                break;
            }
        }
    }
    return packet;
}
