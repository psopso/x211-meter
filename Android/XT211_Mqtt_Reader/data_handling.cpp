#pragma once
#include "config.h"

void addToQueue(const MeterData& data);
void pruneOldData();
void publishQueue();

/* data_handling.cpp */
#include <ArduinoJson.h>
#include <time.h> // Přidáno pro práci s časem (strftime)
#include "data_handling.h"
#include "connections.h"

// Odkaz na globální proměnné z hlavního souboru
extern MeterData dataQueue[MAX_QUEUE_SIZE];
extern int queueItemCount;

void addToQueue(const MeterData& data) {
    if (queueItemCount < MAX_QUEUE_SIZE) {
        dataQueue[queueItemCount] = data;
        queueItemCount++;
        Serial.println("Data přidána do fronty.");
    } else {
        Serial.println("Chyba: Fronta je plná. Nejstarší záznam bude přepsán.");
        // Implementace cyklického bufferu - posuneme všechny prvky a nový vložíme na konec
        for(int i = 0; i < MAX_QUEUE_SIZE - 1; i++) {
            dataQueue[i] = dataQueue[i+1];
        }
        dataQueue[MAX_QUEUE_SIZE - 1] = data;
    }
}

void pruneOldData() {
    time_t now;
    time(&now);

    int originalCount = queueItemCount;
    int newCount = 0;
    MeterData newQueue[MAX_QUEUE_SIZE];

    for (int i = 0; i < queueItemCount; i++) {
        // Ponecháme pouze data, která nejsou starší než DATA_RETENTION_S
        if ((now - dataQueue[i].timestamp) < DATA_RETENTION_S) {
            if (newCount < MAX_QUEUE_SIZE) {
                newQueue[newCount] = dataQueue[i];
                newCount++;
            }
        }
    }

    if (newCount < originalCount) {
        Serial.printf("Promazáno %d starých záznamů.\n", originalCount - newCount);
        queueItemCount = newCount;
        // Zkopírujeme pročištěnou frontu zpět
        for (int i = 0; i < queueItemCount; i++) {
            dataQueue[i] = newQueue[i];
        }
    }
}

/**
 * @brief Odešle obsah fronty na MQTT.
 * Každý záznam je odeslán jako samostatná MQTT zpráva.
 * Časová značka je formátována do čitelného řetězce.
 */
void publishQueue() {
    if (queueItemCount == 0) {
        Serial.println("Fronta je prázdná, není co odesílat.");
        return;
    }

    Serial.printf("Odesílám %d záznamů z fronty (každý jako samostatnou zprávu)...\n", queueItemCount);

    for (int i = 0; i < queueItemCount; i++) {
        JsonDocument doc; // Malý dokument pro jeden záznam

        // --- ZMĚNA FORMÁTU ČASU ---
        time_t timestamp = dataQueue[i].timestamp;
        struct tm * timeinfo = localtime(&timestamp);
        char timeString[25];
        // Formátování času do řetězce "YYYY-MM-DD HH:MM:SS"
        strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        doc["datetime"] = timeString; // Místo "timestamp" používáme "datetime" s formátovaným časem
        doc["first_after_restart"] = dataQueue[i].firstAfterRestart;

        JsonObject values = doc.createNestedObject("values");
        for (int j = 0; j < OBIS_CODE_COUNT; j++) {
            // Formátování na 2 desetinná místa
            values[OBIS_CODES_TO_PARSE[j]] = serialized(String(dataQueue[i].values[j], 2));
        }

        String output;
        serializeJson(doc, output);
        
        Serial.printf("Odesílám záznam %d/%d (%s)...\n", i + 1, queueItemCount, timeString);
        publishData(output.c_str());

        delay(50); // Krátká pauza, aby se nezahltil broker/síťový stack
    }
    Serial.println("Všechny záznamy z fronty byly odeslány.");
}