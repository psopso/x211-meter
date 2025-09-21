#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "custom_config.h"
#include "custom_time.h"
#include "custom_rs485.h"
#include "custom_dlms.h"
#include "custom_storage.h"
#include "custom_mqtt.h"
#include "custom_battery.h"
#include "custom_led.h"

#include "custom_wifi.h"
#include "custom_ota.h"

static const char *TAG = "MAIN";

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int wakeUpCycleCounter = 0;
RTC_DATA_ATTR int badFrameCounter = 0;
RTC_DATA_ATTR int mqttWakeUpCycleCounter = 0;


bool wifiInitialized = false;

void app_main(void) {
    // Inicializace NVS - nezbytné pro Wi-Fi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Zapnutí napájení pro periferie
    gpio_set_direction(POWER_ENABLE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_ENABLE_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500)); // Počkat na stabilizaci napájení

    // Inicializace komponent
    storage_init();
    battery_init();

    // Zjistit důvod probuzení
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    bool first_boot = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED);
    if (first_boot)
      switchLED(true,  5, 0, 0);
    else
      switchLED(true, 0,0,5); // int red, int green, int blue);

    if (first_boot) {
		bootCount += 1;
        ESP_LOGI(TAG, "První spuštění po restartu.");
        // TODO: Pro první spuštění může být nutné se připojit k WiFi a synchronizovat čas
        custom_wifi_init("KaiserData", "Tamade69", "192.168.143.153", "192.168.143.250", "255.255.255.0");
		        
        if (custom_wifi_is_connected()) {
			wifiInitialized = true;
            ESP_LOGI("MAIN", "Wi-Fi je připojeno, můžu posílat data.");
            // ✅ OTA teď v samostatném modulu
        	simple_ota_check_and_do_update(OTA_URL);
	        esp_err_t ntp_esp_status = time_sync_from_ntp();
	        if (ntp_esp_status == ESP_FAIL) {
				custom_mqtt_send_status(CMD_STATUS, "FAILED", "NTP failed", bootCount, wakeUpCycleCounter);
				uint64_t sleep_duration_us = (uint64_t)DEEP_SLEEP_INTERVAL_MIN_NTP * 60 * 1000000;
				ESP_LOGI(TAG, "NTP Failed - Vstupuji do deep sleep na %llu sekund.", sleep_duration_us / 1000000);
				esp_sleep_enable_timer_wakeup(sleep_duration_us);
				// Vypnutí napájení periferií před spánkem
    			gpio_set_level(POWER_ENABLE_GPIO, 0); 
    			esp_deep_sleep_start();
			}
//	        ESP_ERROR_CHECK(ntp_esp_status);
//           custom_wifi_disconnect();
			custom_mqtt_send_status(CMD_STATUS, "OK", "First start", bootCount, wakeUpCycleCounter);

        } else {
            ESP_LOGW("MAIN", "Wi-Fi je odpojeno, čekám...");
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        
    } else {
		wakeUpCycleCounter += 1;
		mqttWakeUpCycleCounter += 1;
	};

    rs485_init();

    uint8_t buffer[UART_BUFFER_SIZE];
    
    time_t frame_reception_time;
    
    int length = rs485_read_frame(buffer, UART_BUFFER_SIZE, &frame_reception_time);
    esp_log_buffer_hex(TAG, buffer, length);
    
//    frame_reception_time = time(NULL);
    
    char s[50];
 	formatDatetime(frame_reception_time, s);
    
    if (length > 0) {
        ESP_LOGI(TAG, "Prijat platny ramec, delka: %d", length);
        ESP_LOGI(TAG, "Datum a cas prijeti: %s", s);
        
        dlms_data_t parsed_data;
        if (dlms_parse_frame(buffer, length, &parsed_data) == ESP_OK) {
            storage_add_to_queue(&parsed_data, frame_reception_time, first_boot);
        } else {
            ESP_LOGE(TAG, "Chyba při parsování DLMS rámce.");
        }
    } else {
        ESP_LOGE(TAG, "Nepodařilo se přijmout platný rámec v časovém limitu.");
        // TODO: Zde implementovat logiku pro opakované pokusy
        badFrameCounter +=1;
    }

    // Kontrola, zda je čas odeslat data a synchronizovat čas
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Odesíláme vždy na začátku hodiny (např. v prvních 15 minutách)
    if ((first_boot) || (mqttWakeUpCycleCounter > 4)) {
		mqttWakeUpCycleCounter = 0;
		ESP_LOGI(TAG, "wifiInitialized: %b ", wifiInitialized);
	   if (wifiInitialized) {
		   ESP_LOGI(TAG, "CustomWifiConnected: %b", custom_wifi_is_connected());
		    if (!custom_wifi_is_connected()) {
    		    ESP_LOGI(TAG, "Zkusím WIFI po novu.");
        		vTaskDelay(pdMS_TO_TICKS(5000));
        		custom_wifi_init(WIFI_SSID, WIFI_PASSWORD,  WIFI_IP, WIFI_GW, WIFI_NETMASK);
        		vTaskDelay(pdMS_TO_TICKS(5000));
			} 
		} else {
    	    ESP_LOGI(TAG, "Zkusím WIFI po novu.");
        	vTaskDelay(pdMS_TO_TICKS(5000));
        	custom_wifi_init("KaiserData", "Tamade69",  "192.168.143.153", "192.168.143.250", "255.255.255.0");
        	vTaskDelay(pdMS_TO_TICKS(5000));		
		}
		
        ESP_LOGI(TAG, "Čas pro odeslání dat na MQTT a synchronizaci času.");
        custom_mqtt_send_data();
        if (!first_boot)
    	    custom_mqtt_send_status(CMD_STATUS, "OK", "After wakeup", bootCount, wakeUpCycleCounter);
 

                
        float voltage, soc;
        if (battery_get_status(&voltage, &soc) == ESP_OK) {
			ESP_LOGI(TAG, "Battery status: %f  %f", voltage, soc);
//            custom_mqtt_send_status(voltage, soc);
            custom_mqtt_send_status(CMD_BATTERY, voltage, soc);
        }
        if (!first_boot) {
          esp_err_t ntp_esp_status = time_sync_from_ntp(); // Opravit čas
         }
    }

    // Vyčistit stará data z fronty
    storage_cleanup_old_data();

    // Příprava na deep sleep
    uint64_t time_to_enter_sleep_us = (uint64_t)(time(NULL) - frame_reception_time) * 1000000;
    uint64_t sleep_duration_us = ((uint64_t)DEEP_SLEEP_INTERVAL_MIN * 60 * 1000000) - time_to_enter_sleep_us - ((uint64_t)DEEP_SLEEP_OFFSET_S * 1000000);
    
    if (sleep_duration_us > ((uint64_t)DEEP_SLEEP_INTERVAL_MIN * 60 * 1000000)) {
        // Pojistka pro případ, že výpočet nevyjde (např. při neúspěšném čtení)
        sleep_duration_us = (uint64_t)DEEP_SLEEP_INTERVAL_MIN * 60 * 1000000;
    }

    esp_sleep_enable_timer_wakeup(sleep_duration_us);
    ESP_LOGI(TAG, "Boot statistika: Resets: %i  Wakeups: %i Bad frames: %i", bootCount, wakeUpCycleCounter, badFrameCounter);
    ESP_LOGI(TAG, "Vstupuji do deep sleep na %llu sekund.", sleep_duration_us / 1000000);
    
    // Vypnutí napájení periferií před spánkem
    gpio_set_level(POWER_ENABLE_GPIO, 0); 
    
    esp_deep_sleep_start();
}

void formatDatetime(time_t frametime, char *s) {
	struct tm *local_time_info;
	local_time_info = localtime(&frametime); // Convert to local time
	strftime(s, 50, "%c", local_time_info);
}
