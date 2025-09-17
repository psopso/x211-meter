#include "custom_time.h"
#include "esp_log.h"
#include "esp_sntp.h" // Použití správné hlavičky pro aktuální API
#include <time.h>
#include <sys/time.h>

static const char *TAG = "CUSTOM_TIME";

// Globální proměnná pro signalizaci dokončení synchronizace
volatile bool time_sync_successful = false;

// Callback funkce pro oznámení o dokončení synchronizace
static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized from NTP server.");
    time_sync_successful = true;
}

esp_err_t time_sync_from_ntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    // Počkat na dokončení synchronizace
    int retry = 0;
    const int retry_count = 15;
    while (!time_sync_successful && retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for time synchronization... (%d/%d)", retry + 1, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        retry++;
    }
    
    if (time_sync_successful) {
        ESP_LOGI(TAG, "Time synchronization finished.");
        // Ověření času po synchronizaci
        time_t now = 0;
        struct tm timeinfo = { 0 };
        char strftime_buf[64];
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Time synchronization failed after %d retries.", retry_count);
        return ESP_FAIL;
    }
}