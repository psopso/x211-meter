#pragma once

std::vector<uint8_t> read_packet(HardwareSerial &serial, uint32_t timeout_ms);
