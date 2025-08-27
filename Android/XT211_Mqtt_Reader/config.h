#pragma once
#include <ctime>
#include <Arduino.h>

/**
 * @file config.h
 * @brief Centrální konfigurační soubor pro celý projekt.
 *
 * Zde se nastavují všechny parametry, jako jsou přihlašovací údaje,
 * adresy serverů, časové intervaly a hardwarové piny.
 */

// -- KONFIGURACE WiFi --
#define WIFI_SSID "KaiserData"
#define WIFI_PASSWORD "Tamade69"

// -- KONFIGURACE MQTT --
#define MQTT_BROKER "192.168.143.151"
#define MQTT_PORT 1883
#define MQTT_USER "admin" // Může být prázdné, pokud není vyžadováno
#define MQTT_PASSWORD "Bubak,3390" // Může být prázdné, pokud není vyžadováno
#define MQTT_CLIENT_ID "elektromerXT211-ESP32"
#define MQTT_TOPIC_DATA "elektromer/xt211/data"
#define MQTT_TOPIC_STATUS "elektromer/xt211/status"

// -- KONFIGURACE NTP --
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600   // Posun pro CET (UTC+1)
#define DAYLIGHT_OFFSET_SEC 3600 // Posun pro letní čas (CEST)

// -- KONFIGURACE ČASOVÁNÍ (v sekundách) --
#define SLEEP_DURATION_S 885             // 15 minut: Jak často se zařízení probouzí pro čtení
#define SEND_INTERVAL_S 3600             // 1 hodina: Jak často se odesílají data na MQTT
#define DATA_RETENTION_S 86400           // 24 hodin: Jak stará data se mají uchovávat

// -- KONFIGURACE ČTENÍ RÁMCE --
#define FRAME_WAIT_TIMEOUT_MS 70000      // 70s: Max. čas čekání na PRVNÍ rámec po restartu
#define FRAME_READ_ATTEMPTS 3            // Po kolika neúspěšných cyklech čtení odeslat chybu

// -- KONFIGURACE RS485 --
#define RS485_RX_PIN 16 // GPIO pin připojený na RX pin ESP32 (z pohledu ESP32)
#define RS485_TX_PIN 17 // GPIO pin připojený na TX pin ESP32 (pro RS485 modul)
#define RS485_BAUDRATE 9600 // Baudrate pro XT211 je typicky 9600

// -- KONFIGURACE APLIKACE --
#define MAX_QUEUE_SIZE 150 // Max. počet záznamů ve frontě (24h * 6 záznamů/h = 144)

// Automatický výpočet počtu kódů
const int OBIS_CODE_COUNT = 5;

// Seznam požadovaných OBIS kódů pro parsování
extern const char* OBIS_CODES_TO_PARSE[];

// Struktura pro uložení jednoho záznamu měření
struct MeterData {
    time_t timestamp;
    bool firstAfterRestart;
    float values[OBIS_CODE_COUNT]; // Hodnoty odpovídají pořadí v OBIS_CODES_TO_PARSE
};
