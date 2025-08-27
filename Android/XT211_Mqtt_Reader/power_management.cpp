#include <esp_sleep.h>
#include <time.h> // Přidáno pro práci s časem
#include "config.h"
#include "power_management.h"

void goToSleep(uint64_t sleepSeconds) {
    // Bezpečnostní pojistka proti příliš krátkým cyklům spánku
    if (sleepSeconds < 10) {
        Serial.printf("Vypočtená doba spánku (%llu s) je příliš krátká, nastavuji na 10s.\n", sleepSeconds);
        sleepSeconds = 10;
    }

    // --- VÝPOČET ČASU PROBUZENÍ ---
    time_t now;
    time(&now);
    time_t wakeupTime = now + sleepSeconds;
    struct tm * timeinfo = localtime(&wakeupTime);
    char timeString[25];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", timeinfo);

    Serial.printf("Uspávám zařízení na %llu sekund.\n", sleepSeconds);
    Serial.printf("Probuzení v %s\n", timeString);
    Serial.println("===================================\n");
    Serial.flush();
    
    esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000ULL);
    esp_deep_sleep_start();
}