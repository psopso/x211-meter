#include "custom_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    custom_wifi_init("MojeSSID", "MojeHeslo");

    while (1) {
        if (custom_wifi_is_connected()) {
            ESP_LOGI("MAIN", "Wi-Fi je připojeno, můžu posílat data.");
        } else {
            ESP_LOGW("MAIN", "Wi-Fi je odpojeno, čekám...");
        }

        // --- LOGOVÁNÍ STACKU HLAVNÍHO TASKU ---
        UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI("MAIN", "Main task stack usage: used ~%u bytes, free ~%u bytes",
                 (CONFIG_ESP_MAIN_TASK_STACK_SIZE - watermark * sizeof(StackType_t)),
                 watermark * sizeof(StackType_t));

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
