/**
 * @file XT211_Mqtt_Reader.ino
 * @brief Hlavní soubor pro čtení dat z elektroměru XT211 a odesílání na MQTT.
 *
 * Tento soubor řídí hlavní logiku zařízení:
 * 1. Rozhoduje o úkolech po probuzení (první start vs. běžné probuzení).
 * 2. Volá funkce pro čtení a parsování dat.
 * 3. Spravuje časování pro odesílání dat a promazávání fronty.
 * 4. Uvádí zařízení do hlubokého spánku pro úsporu energie.
 */

//mosquitto_sub -h 192.168.143.151 -u admin -P Bubak,3390 -v -t 'elektromer/#'

#include "config.h"
#include "connections.h"
#include "data_handling.h"
#include "dlms_parser.h"
#include "power_management.h"

// Definice pole OBIS kódů, které je deklarováno jako extern v config.h
const char* OBIS_CODES_TO_PARSE[OBIS_CODE_COUNT] = {
    "1.8.0", // Celková spotřeba energie (A+)
    "1.8.1", // Spotřeba energie v tarifu 1 (T1)
    "1.8.2", // Spotřeba energie v tarifu 2 (T2)
    "1.8.3", // Spotřeba energie v tarifu 3 (T3)
    "1.8.4"  // Spotřeba energie v tarifu 4 (T4)
};

// Globální proměnné uložené v RTC paměti, které přežijí deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int wakeUpCycleCounter = 0;
RTC_DATA_ATTR MeterData dataQueue[MAX_QUEUE_SIZE];
RTC_DATA_ATTR int queueItemCount = 0;
RTC_DATA_ATTR int failedReadAttempts = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n===================================");
    Serial.println("Zařízení se probudilo.");


    bootCount++;
    wakeUpCycleCounter++;

    // Výpočet, po kolika cyklech probuzení se mají odesílat data
    const int sendIntervalCycles = SEND_INTERVAL_S / SLEEP_DURATION_S;
    time_t frameArrivalTime = 0; // Zde uložíme čas příchodu rámce
    bool frameReadSuccess = false;

    // Zjištění důvodu probuzení
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    // První spuštění po zapnutí (ne po probuzení z deep sleep)
    if (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("První spuštění po zapnutí/restartu.");
        bootCount = 1;
        wakeUpCycleCounter = 1;
        queueItemCount = 0; // Vynulujeme frontu
        failedReadAttempts = 0;

        connectWiFi();
        syncNTP();

        Serial.printf("Čekám na první platný rámec (max %lus)...\n", FRAME_WAIT_TIMEOUT_MS / 1000);
        MeterData firstData;
        frameReadSuccess = readAndParseFrame(firstData, FRAME_WAIT_TIMEOUT_MS, frameArrivalTime);
        if (frameReadSuccess) {
            firstData.firstAfterRestart = true;
            addToQueue(firstData);

            Serial.println("První data načtena, odesílám na MQTT.");
            connectMQTT();
            publishQueue();
            disconnectMQTT();
        } else {
            Serial.println("Nepodařilo se načíst první rámec.");
            connectMQTT();
            publishStatus("Error: Initial frame read failed.");
            disconnectMQTT();
        }
        //disconnectWiFi();
    } else { // Běžné probuzení časovačem
        Serial.printf("Běžné probuzení č. %d\n", wakeUpCycleCounter);

        MeterData newData;
        // Čekáme na rámec, timeout je kratší než interval spánku
        frameReadSuccess = readAndParseFrame(newData, 30000, frameArrivalTime);
        if (frameReadSuccess) {
             newData.firstAfterRestart = false;
             addToQueue(newData);
             failedReadAttempts = 0; // Resetujeme počítadlo chyb
        } else {
            Serial.println("Nepodařilo se načíst data po probuzení.");
            failedReadAttempts++;
        }
    }

    // Kontrola, zda je čas odeslat data nebo chybovou zprávu
    bool shouldConnect = (wakeUpCycleCounter % sendIntervalCycles == 0) || (failedReadAttempts >= FRAME_READ_ATTEMPTS);

    if (shouldConnect) {
        connectWiFi();
        syncNTP(); // Pravidelná synchronizace času
        connectMQTT();

        if (failedReadAttempts >= FRAME_READ_ATTEMPTS) {
            Serial.printf("Překročen počet neúspěšných pokusů (%d). Odesílám chybovou zprávu.\n", failedReadAttempts);
            publishStatus("Error: Failed to read frame in multiple cycles.");
            failedReadAttempts = 0; // Reset po odeslání chyby
        } else {
            Serial.println("Čas pro odeslání dat na MQTT.");
            publishStatus("OK: Hourly data send.");
            publishQueue();
        }

        disconnectMQTT();
//        disconnectWiFi();
    }

    // Jednou za den promaž stará data
    pruneOldData();
    delay(5000);
    disconnectWiFi();
    delay(1000);

    // --- DYNAMICKÝ VÝPOČET DOBY SPÁNKU ---
    uint64_t sleepDuration = SLEEP_DURATION_S; // Výchozí doba spánku

    if (frameArrivalTime > 0) {
        time_t currentTime;
        time(&currentTime);
        long workDuration = currentTime - frameArrivalTime;

        if (workDuration >= 0 && workDuration < SLEEP_DURATION_S) {
            sleepDuration = SLEEP_DURATION_S - workDuration;
            Serial.printf("Dynamický spánek: Cílový interval %ds, práce trvala %lds.\n", SLEEP_DURATION_S, workDuration);
        } else {
            Serial.printf("Doba práce (%lds) je neplatná nebo delší než interval spánku. Používám výchozí dobu spánku.\n", workDuration);
        }
    } else {
        Serial.println("Platný rámec nebyl přijat. Používám výchozí dobu spánku.");
    }
    
    goToSleep(sleepDuration);
}

void loop() {
    // Tento kód se nikdy nevykoná, protože ESP32 je v deep sleep.
}
