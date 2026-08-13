// ==============================================================================
// KNIHOVNY A ZÁVISLOSTI
// ==============================================================================
#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "custom_config.h"
#include "custom_rs485.h"
#include "custom_dlms.h"
#include "custom_storage.h"
#include "custom_comm.h"
#include "custom_battery.h"
#include "custom_led.h"
//#include "custom_wifi.h"
#include "custom_ota.h"
#include "custom_time.h"

#include "my_debug.h"

// ==============================================================================
// GLOBÁLNÍ PROMĚNNÉ A KONSTANTY
// ==============================================================================
static const char *TAG = "MAIN";

// --- RTC paměť (uchovává data mezi hlubokými spánky) ---
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int wakeUpCycleCounter = 0;
RTC_DATA_ATTR int badFrameCounter = 0;
RTC_DATA_ATTR int mqttWakeUpCycleCounter = 0;
RTC_DATA_ATTR time_t firstBootTime = 0;
RTC_DATA_ATTR int32_t adaptive_offset = 0; 
RTC_DATA_ATTR unsigned char waitTimeMin = 0;
RTC_DATA_ATTR unsigned char waitTimeMax = 0;
RTC_DATA_ATTR char next_wakeup_str_pers[64] = {0};
RTC_DATA_ATTR int64_t last_planned_wakeup_usec = 0;

// --- Běžné proměnné (nulují se při každém startu) ---
bool forceOTA = false;
bool wifiInitialized = false;
bool resetWaitTime = false;
unsigned char diff = 0;
int waitTime = 0;
double drift = 0;
double network_delay_sec = 0;

// ==============================================================================
// PROTOTYPY FUNKCÍ
// ==============================================================================
const char *get_build_datetime(void);
void sendBatteryStatus();
void enter_aligned_deep_sleep(void);
void formatDatetime(time_t frametime, char *s);
void formatDatetimeShort(time_t frametime, char *s);
void time_sync_from_ntp_with_drift(bool first_boot, double *drift, double *network_delay_sec);
void ota_update(time_t startTime, bool first_boot);

void send_data(const char *status, const char *reason, time_t startTime, bool first_boot, bool status_only);
// ==============================================================================
// HLAVNÍ BĚH PROGRAMU
// ==============================================================================
void app_main(void) {
	// 0. Inicializace detekce USB kabelu
    gpio_reset_pin(USB_SENSE_PIN);
    gpio_set_direction(USB_SENSE_PIN, GPIO_MODE_INPUT);
    // Hardwarový pulldown už dělá spodní rezistor děliče, ale jistota je jistota:
    gpio_pulldown_en(USB_SENSE_PIN);
    
    
    // --- 1. OKAMŽITÝ ODBĚR ČASU PŘI STARTU ---
    struct timeval tv_start;
    gettimeofday(&tv_start, NULL);
    time_t startTime = tv_start.tv_sec;

    #if CONFIG_MY_DEBUG_ENABLED
        my_debug_register_sender(mqtt_send_debug);
//        custom_wifi_init(WIFI_SSID, WIFI_PASSWORD, WIFI_IP, WIFI_GW, WIFI_NETMASK);
	comm_init();
        vTaskDelay(pdMS_TO_TICKS(3000));
        ota_update(startTime, first_boot);
    #endif

    // --- 2. VÝPIS STATISTIKY PROBUZENÍ ---
    char s_now[64] = {0};
    formatDatetime(startTime, s_now);

    ESP_LOGI(TAG, "================ START STATISTIKA ================");
    ESP_LOGI(TAG, "Aktualni cas startu: %s (UNIX: %lld.%06d)", s_now, (long long)tv_start.tv_sec, (int)tv_start.tv_usec);
    
    if (last_planned_wakeup_usec > 0) {
        int64_t actual_now_usec = (int64_t)tv_start.tv_sec * 1000000LL + tv_start.tv_usec;
        double diff_from_plan = (double)(actual_now_usec - last_planned_wakeup_usec) / 1000000.0;
        ESP_LOGI(TAG, "Posledni plan startu: %s (UNIX: %lld.%06d)", next_wakeup_str_pers, (long long)(last_planned_wakeup_usec / 1000000LL), (int)(last_planned_wakeup_usec % 1000000LL));
        ESP_LOGI(TAG, "HW Chyba casovace (Drift): %.6f s", diff_from_plan);
    } else {
        ESP_LOGI(TAG, "Posledni plan startu: N/A (Prvni boot)");
    }
    ESP_LOGI(TAG, "Firmware build UTC: %s", get_build_datetime());
//	ESP_LOGI(TAG, "Target: %s", IDF_TARGET)
    ESP_LOGI(TAG, "==================================================");

    // --- 3. HARDWAROVÁ INICIALIZACE ---
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    #ifndef ENV_TEST
        gpio_set_direction(POWER_ENABLE_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(POWER_ENABLE_GPIO, 1);
        gpio_set_direction(RS485_ENABLE_RX_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(RS485_ENABLE_RX_PIN, 0);
    #endif

    vTaskDelay(pdMS_TO_TICKS(500)); 
    storage_init();
    
    #ifndef ENV_TEST
        battery_init();
    #endif

    // --- 4. IDENTIFIKACE TYPU STARTU (First Boot vs Wakeup) ---
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    bool first_boot = (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED);
    
    if (first_boot) switchLED(true, 5, 0, 0);
    else switchLED(true, 0, 0, 5);

    if (first_boot) {
		forceOTA = true;
        bootCount++;
        adaptive_offset = 0;
        ESP_LOGI(TAG, "Prvni spusteni po restartu.");
        
        #ifndef CONFIG_MY_DEBUG_ENABLED
//            custom_wifi_init(WIFI_SSID, WIFI_PASSWORD, WIFI_IP, WIFI_GW, WIFI_NETMASK);
	      comm_init();
        #endif
        
        if (comm_is_connected()) {
            wifiInitialized = true;
            ota_update(startTime, first_boot);
            if (time_sync_from_ntp() == ESP_FAIL) {
                time(&firstBootTime);
//mqtt                custom_mqtt_send_status(CMD_STATUS, "FAILED", "NTP failed", bootCount, wakeUpCycleCounter, firstBootTime, 0, 0, 0, check_signal_strength(), 0, "", "", 0);
				send_data("OK", "First boot", startTime, first_boot, true);
                esp_sleep_enable_timer_wakeup((uint64_t)DEEP_SLEEP_INTERVAL_MIN_NTP * 60 * 1000000);
                gpio_set_level(POWER_ENABLE_GPIO, 0); 
                ESP_LOGI(TAG, "Going into deep sleep mode for %llu", (unsigned long long)DEEP_SLEEP_INTERVAL_MIN_NTP);
                esp_deep_sleep_start();
            }
            time(&firstBootTime);
//mqtt            custom_mqtt_send_status(CMD_STATUS, "OK", "First start", bootCount, wakeUpCycleCounter, firstBootTime, 0, 0, 0, check_signal_strength(), 0, "", "", 0);
			send_data("OK", "First boot", startTime, first_boot, true);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
        mqttWakeUpCycleCounter++;        
    } else {
        wakeUpCycleCounter++;
        mqttWakeUpCycleCounter++;
    }

    // --- 5. KOMUNIKACE RS485 A ZPRACOVÁNÍ DAT ---
    rs485_init();
    uint8_t buffer[UART_BUFFER_SIZE];
    time_t frame_reception_time;
    bool rs485completed = false;

    while (!rs485completed) {    
        int length = rs485_read_frame(buffer, UART_BUFFER_SIZE, &frame_reception_time);
        
        if (length > 0) {
            if (first_boot) {
                diff = 0;
                waitTimeMin = 250;
                waitTimeMax = 0;
            } else {
                double real_diff = difftime(frame_reception_time, startTime);
                diff = (real_diff < 0) ? 0 : (real_diff > 255 ? 255 : (unsigned char)real_diff);
            }
            
            waitTime = (int)diff;
            if (waitTime < 65) {
                if (waitTime < waitTimeMin) waitTimeMin = waitTime;
                if (waitTime > waitTimeMax) waitTimeMax = waitTime;
            }
			
            dlms_data_t parsed_data;
            if (dlms_parse_frame(buffer, length, &parsed_data) == ESP_OK) {
                storage_add_to_queue(&parsed_data, frame_reception_time, first_boot, waitTime);
                rs485completed = true;
                
                if (!first_boot) {
                    diff += DEEP_SLEEP_OFFSET_S; 
                    adaptive_offset += (int32_t)diff;
                    ESP_LOGI(TAG, "Adaptivni offset: %ld s (waittime: %d s)", (long)adaptive_offset, waitTime);
                }
            } else {
                badFrameCounter++;
            }
        } else {
            badFrameCounter++;
        }

        // Záchranný mechanismus při selhání RS485
        if (!rs485completed && badFrameCounter > RS485_BAD_FRAMES_MAX) {
            if (!comm_is_connected()) comm_init(); //custom_wifi_init(WIFI_SSID, WIFI_PASSWORD, WIFI_IP, WIFI_GW, WIFI_NETMASK);
            vTaskDelay(pdMS_TO_TICKS(5000));
            ota_update(startTime, first_boot);
            sendBatteryStatus();
//mqtt            custom_mqtt_send_status(CMD_STATUS, "FAILED", "rs485 failed", bootCount, wakeUpCycleCounter, firstBootTime, 0, 0, 0, check_signal_strength(), 0, "", "", 0);
			send_data("FAILED", "rs485 failed", startTime, first_boot, true);
            esp_sleep_enable_timer_wakeup((uint64_t)DEEP_SLEEP_INTERVAL_MIN_NTP * 60 * 1000000);
            gpio_set_level(POWER_ENABLE_GPIO, 0); 
	    ESP_LOGI(TAG, "Going into deep sleep mode for %llu", (unsigned long long)DEEP_SLEEP_INTERVAL_MIN_NTP);
            esp_deep_sleep_start();                
        }
    }
    
	ESP_LOGI(TAG, "Wakeup cycle counter: %ld(%ld)", (long)mqttWakeUpCycleCounter, (long)wakeUpCycleCounter);
    // --- 6. ODESLÁNÍ DAT (MQTT & SYNC) ---
    if (first_boot || (mqttWakeUpCycleCounter >= 4)) {
        mqttWakeUpCycleCounter = 0;
        
        if (!comm_is_connected()) {
//            custom_wifi_init(WIFI_SSID, WIFI_PASSWORD, WIFI_IP, WIFI_GW, WIFI_NETMASK);
              comm_init();
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
        
//mqtt        custom_mqtt_send_data();
		if (first_boot)
			send_data("OK", "First boot", startTime, first_boot, false);
		else
			send_data("OK", "After wakeup", startTime, first_boot, false);
        time_sync_from_ntp_with_drift(first_boot, &drift, &network_delay_sec);
        
//        if (!first_boot) {
//            char s_short[64];
//            formatDatetimeShort(startTime, s_short);
//mqtt            custom_mqtt_send_status(CMD_STATUS, "OK", "After wakeup", bootCount, wakeUpCycleCounter, firstBootTime, waitTime, waitTimeMin, waitTimeMax, check_signal_strength(), adaptive_offset, s_short, next_wakeup_str_pers, drift);
//        }
        
        sendBatteryStatus();
        ota_update(startTime, first_boot);
        resetWaitTime = true;         
    }

    // --- 7. ÚKLID A SPÁNEK ---
    storage_cleanup_old_data();
    enter_aligned_deep_sleep();
}

// ==============================================================================
// POMOCNÉ FUNKCE
// ==============================================================================

void enter_aligned_deep_sleep(void) {
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL); 
    
    uint32_t interval_sec = DEEP_SLEEP_INTERVAL_MIN * 60;
    uint32_t sec_past_anchor = (uint32_t)tv_now.tv_sec % interval_sec;
    uint32_t usec_past_anchor = (uint32_t)tv_now.tv_usec;
    
    int64_t usec_to_sleep = (int64_t)(interval_sec - sec_past_anchor) * 1000000LL - usec_past_anchor;
    
    if (adaptive_offset > 120 || adaptive_offset < -120) adaptive_offset = 0;
    
    int32_t final_offset = adaptive_offset;
    if (final_offset > 30) final_offset -= 60;
    else if (final_offset < -30) final_offset += 60;
    adaptive_offset = final_offset;  // Vrátíme přepočet adaptive offsetu
    
    usec_to_sleep += (int64_t)(final_offset + DEEP_SLEEP_OFFSET_S) * 1000000LL;

    if (usec_to_sleep < 10000000LL) {
        usec_to_sleep += (int64_t)interval_sec * 1000000LL;
    }

    // Uložení plánu pro příští kontrolu po probuzení
    last_planned_wakeup_usec = (int64_t)tv_now.tv_sec * 1000000LL + tv_now.tv_usec + usec_to_sleep;
    time_t next_wakeup_sec = last_planned_wakeup_usec / 1000000LL;
    
    struct tm next_wakeup_tm;
    localtime_r(&next_wakeup_sec, &next_wakeup_tm);
    strftime(next_wakeup_str_pers, sizeof(next_wakeup_str_pers), "%H:%M:%S", &next_wakeup_tm);

    time_t now_unix = tv_now.tv_sec;
    struct tm now_tm;
    char now_str[64];
    localtime_r(&now_unix, &now_tm);
    strftime(now_str, sizeof(now_str), "%H:%M:%S", &now_tm);
    
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "1. Aktualni cas:   %s (UNIX: %lld.%06d)", now_str, (long long)tv_now.tv_sec, (int)tv_now.tv_usec);    
    ESP_LOGI(TAG, "2. Doba spanku:    %.3f sekund", (double)usec_to_sleep / 1000000.0);
    ESP_LOGI(TAG, "3. Plan probuzeni: %s (UNIX: %lld.%06lld)", next_wakeup_str_pers, (long long)next_wakeup_sec, last_planned_wakeup_usec % 1000000LL);
//    SMART_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "==========================================");
    
    if (resetWaitTime) {
        waitTimeMin = 250;
        waitTimeMax = 0;
    }
    ESP_LOGI(TAG, "Going into deep sleep mode ...");
    gpio_set_level(POWER_ENABLE_GPIO, 0); 
    esp_sleep_enable_timer_wakeup(usec_to_sleep);
    esp_deep_sleep_start();
}

void time_sync_from_ntp_with_drift(bool first_boot, double *drift_out, double *delay_out) {
    if (first_boot) return;

    struct timeval tv_before, tv_after;
    gettimeofday(&tv_before, NULL);
    int64_t t_start = esp_timer_get_time();
    
    comm_time_sync_from_ntp(); 
    
    int64_t t_end = esp_timer_get_time();
    gettimeofday(&tv_after, NULL);
    
    double time_b_sec = tv_before.tv_sec + (tv_before.tv_usec / 1000000.0);
    double time_a_sec = tv_after.tv_sec + (tv_after.tv_usec / 1000000.0);
    *delay_out = (double)(t_end - t_start) / 1000000.0;
    *drift_out = (time_a_sec - time_b_sec) - *delay_out;
}

void sendBatteryStatus() {
    float voltage, soc;
    if (battery_get_status(&voltage, &soc) == ESP_OK) {
//mqtt        custom_mqtt_send_status(CMD_BATTERY, voltage, soc);
    }
}

void formatDatetime(time_t frametime, char *s) {
    struct tm *local_time_info = localtime(&frametime); 
    strftime(s, 64, "%c", local_time_info);
}

void formatDatetimeShort(time_t frametime, char *s) {
    struct tm *local_time_info = localtime(&frametime); 
    strftime(s, 64, "%Y-%m-%d %H:%M:%S", local_time_info);
}

const char *get_build_datetime(void) { return BUILD_DATETIME; }

void send_data(const char *status, const char *reason, time_t startTime, bool first_boot, bool status_only) {
	ESP_LOGI(TAG, "Odesílám data a status: first_boot %d, status_only: %d", first_boot, status_only);
	comm_data_t out_data = {0}; // Vynuluje strukturu
//custom_mqtt(CMD_STATUS, "OK", "After wakeup", bootCount, 
//   wakeUpCycleCounter, firstBootTime, waitTime, waitTimeMin, waitTimeMax, 
//   check_signal_strength(), adaptive_offset, s_short, next_wakeup_str_pers, drift);
//	
//    state, reason,boot_count,wakeup_cycle_count,first_boot_time,wait_time,
//    wait_time_min,wait_time_max,RSSI,adaptive_offset,startTime,next_wakeup_str,drift;

//    out_data.rssi = check_signal_strength(); // Bude definováno jinde, nebo přesunuto do comm

//    char s_short[64];
//    formatDatetimeShort(startTime, s_short);
//    strncpy(out_data.start_time_str, s_short, sizeof(out_data.start_time_str) - 1);
//    strncpy(out_data.next_wakeup_str, next_wakeup_str_pers, sizeof(out_data.next_wakeup_str) - 1);


    char s_short[64];

    out_data.state = status;
    out_data.reason = reason;
    out_data.boot_count = bootCount;
    out_data.wakeup_cycle_count = wakeUpCycleCounter;
    out_data.first_boot_time = firstBootTime;
    out_data.adaptive_offset = adaptive_offset;
    out_data.ntp_drift = drift;
    out_data.rssi = check_signal_strength(); // Bude definováno jinde, nebo přesunuto do comm
    formatDatetimeShort(startTime, s_short);
    strncpy(out_data.start_time_str, s_short, sizeof(out_data.start_time_str) - 1);
    strncpy(out_data.next_wakeup_str, next_wakeup_str_pers, sizeof(out_data.next_wakeup_str) - 1);

    if (comm_is_connected()) {
		if (!status_only) {
			ESP_LOGI(TAG,"Pokus o odeslani dat.");
			comm_send(MSG_TYPE_METER_DATA, &out_data); // Odešle hlavní data
		}
		if (!first_boot) {
			ESP_LOGI(TAG, "Pokus o odeslani statusu");
			comm_send(MSG_TYPE_STATUS, &out_data);     // Odešle status
		}
    } else
		ESP_LOGI(TAG, "comm is not connected yet.");
}

void ota_update(time_t startTime, bool first_boot) {
    if (comm_ota_check_and_do_update(OTA_URL, forceOTA) == ESP_OK) {
//mqtt        custom_mqtt_send_status(CMD_STATUS, "OK", "OTA-OK", bootCount, wakeUpCycleCounter, firstBootTime, 0, 0, 0, 0, 0, "", "", 0);      
		send_data("OK", "OTA OK", startTime, first_boot, true);
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }
//	forceOTA = false;
}
