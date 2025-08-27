/* dlms_parser.cpp */
#include <HardwareSerial.h>
#include "dlms_parser.h"

// Inicializace druhého sériového portu pro RS485
HardwareSerial RS485Serial(2);

/**
 * @brief Simulovaná funkce pro parsování DLMS rámce.
 * @warning TOTO JE POUZE SIMULACE. Musíte ji nahradit reálným kódem,
 * který bude dekódovat binární data z elektroměru podle
 * specifikace DLMS/COSEM.
 * @param buffer Ukazatel na buffer s přijatými daty.
 * @param length Délka přijatých dat.
 * @param outData Struktura, do které se uloží naparsované hodnoty.
 * @return true, pokud bylo parsování úspěšné, jinak false.
 */
bool parseDlmsFrame(byte* buffer, size_t length, MeterData& outData) {
    Serial.println("--- SIMULACE PARSOVÁNÍ DLMS RÁMCE ---");

    // Zde by byl komplexní kód pro dekódování HDLC rámce a extrakci hodnot.
    // Pro demonstrační účely naplníme strukturu náhodnými daty.
    for (int i = 0; i < OBIS_CODE_COUNT; i++) {
        outData.values[i] = 10000.0 + (esp_random() % 50000) / 100.0;
    }

    // Označíme záznam aktuálním časem
    time(&outData.timestamp);

    Serial.println("Parsování úspěšné (simulováno).");
    return true;
}

bool readAndParseFrame(MeterData& outData, unsigned long timeoutMs, time_t& frameTimestamp) {
    // Nastavení sériové komunikace: 9600 baud, 8 datových bitů, sudá parita (Even), 1 stop bit
    RS485Serial.begin(RS485_BAUDRATE, SERIAL_8E1, RS485_RX_PIN, RS485_TX_PIN);

    Serial.println("Čekám na data z RS485...");

    unsigned long startTime = millis();
    byte buffer[512]; // Buffer pro příjem dat
    size_t bytesRead = 0;
    bool frameStarted = false;
    const byte FRAME_FLAG = 0x7E; // HDLC start/end flag

    while (millis() - startTime < timeoutMs) {
        if (RS485Serial.available()) {
            byte incomingByte = RS485Serial.read();

            if (incomingByte == FRAME_FLAG) {
                if (!frameStarted) {
                    // Začátek rámce
                    frameStarted = true;
                    time(&frameTimestamp); // << KLÍČOVÁ ZMĚNA: Uložíme čas příchodu rámce
                    bytesRead = 0;
                    buffer[bytesRead++] = incomingByte;
                } else {
                    // Konec rámce
                    buffer[bytesRead++] = incomingByte;
                    Serial.printf("Přijat potenciální rámec o délce %d bytů.\n", bytesRead);
                    RS485Serial.end(); // Ukončíme sériový port
                    return parseDlmsFrame(buffer, bytesRead, outData);
                }
            } else {
                if (frameStarted && bytesRead < sizeof(buffer)) {
                    buffer[bytesRead++] = incomingByte;
                }
            }
        }
        delay(5); // Krátká pauza, aby se nezahltil procesor
    }

    Serial.println("Timeout při čekání na data z RS485.");
    RS485Serial.end();
    return false;
}
