#include "custom_mqtt.h"
#include "custom_config.h"
#include "custom_storage.h"
#include "esp_log.h"
//#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "custom_ota.h"
#include "custom_wifi.h"
#include "my_debug.h"

static const char *TAG = "CUSTOM_MQTT";
//static EventGroupHandle_t s_wifi_event_group;

extern const char *get_build_datetime(void);

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static esp_mqtt_client_handle_t client = NULL;
static int successful_sends = 0;
static int failed_sends = 0;

char * getCurrentDatetime();

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    // Zjednodušený handler pro logování
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%i", base, (int)event_id);
}

void custom_mqtt_send_data(void) {   
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    vTaskDelay(pdMS_TO_TICKS(2000));

    successful_sends = 0; failed_sends = 0;

    for (int i = 0; i < storage_get_queue_count(); i++) {
        data_queue_entry_t entry;
        if (storage_get_queue_entry(i, &entry)) {
			char buf[80];
			struct tm  ts;
		    ts = *localtime(&entry.timestamp);
    		strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);

            cJSON *root = cJSON_CreateObject();
           	cJSON *values = cJSON_CreateObject();
           	cJSON *data = cJSON_CreateObject();

            cJSON_AddStringToObject(root, "datetime", getCurrentDatetime());
			cJSON_AddItemToObject(root, "data", data);    
            cJSON_AddStringToObject(data, "reading_datetime", buf);
            cJSON_AddBoolToObject(data, "first_boot", entry.first_after_restart);
			cJSON_AddItemToObject(data, "values", values);    
			cJSON_AddNumberToObject(data, "waittime", entry.waittime);    

            cJSON_AddNumberToObject(values, "1.8.0", entry.data.obis_1_8_0);
            cJSON_AddNumberToObject(values, "1.8.1", entry.data.obis_1_8_1);
            cJSON_AddNumberToObject(values, "1.8.2", entry.data.obis_1_8_2);
            cJSON_AddNumberToObject(values, "1.8.3", entry.data.obis_1_8_3);
            cJSON_AddNumberToObject(values, "1.8.4", entry.data.obis_1_8_4);
			cJSON_AddStringToObject(values, "96.1.1", entry.data.obis_96_1_1);

            char *json_string = cJSON_PrintUnformatted(root);
            if (esp_mqtt_client_publish(client, MQTT_DATA_TOPIC, json_string, 0, 1, 0) > 0) successful_sends++; else failed_sends++;
            cJSON_Delete(root);
            free(json_string);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    ESP_LOGI(TAG, "Data sending finished. Success: %d, Failed: %d", successful_sends, failed_sends);

    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
    ESP_LOGI(TAG, "MQTT disconnected.");
}

void custom_mqtt_send_status(mqtt_type_t mqtt_type, ...) {
	va_list args;
    va_start(args, mqtt_type);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    vTaskDelay(pdMS_TO_TICKS(2000));

    successful_sends = 0; failed_sends = 0;


    cJSON *root = cJSON_CreateObject();
	cJSON *statustype = cJSON_CreateObject();
	cJSON *battery = cJSON_CreateObject();
    char *json_string = NULL;
    
    switch (mqtt_type) {
        case CMD_BATTERY: {
			double voltage = va_arg(args, double);
            double soc = va_arg(args, double);
		    ESP_LOGI(TAG, "Sending status message... (Voltage: %.2fV, SoC: %.1f%%)", voltage, soc);
		    
            cJSON_AddStringToObject(root, "datetime", getCurrentDatetime());
			cJSON_AddItemToObject(root, "battery", statustype);    
		    cJSON_AddNumberToObject(statustype, "Voltage", voltage);
    		cJSON_AddNumberToObject(statustype, "SOC", soc);

    		json_string = strdup(cJSON_PrintUnformatted(root));
    		break;
		}
        case CMD_STATUS: {
			json_string = strdup("");
			char * status = va_arg(args, char *);
			char * status1 = va_arg(args, char *);
   	        int resets = va_arg(args, int);
   	        int wakeups = va_arg(args, int);
   	        time_t firstBootTime = va_arg(args, time_t);
//   	        int lastWait = va_arg(args, int);
//   	        int lastWaitMin = va_arg(args, int);
//   	        int lastWaitMax = va_arg(args, int);
            int wifiSignal = va_arg(args, int);
   	        int lastAdaptive = va_arg(args, int);
   	        char * startTime = va_arg(args, char *);
   	        char * nextStartTime = va_arg(args, char *);
			double drift = va_arg(args, double);
			double voltage = va_arg(args, double);
			double soc = va_arg(args, double);
			
			char wifiSignalS[10];
			snprintf(wifiSignalS, sizeof(wifiSignalS), "%d", wifiSignal);			
			
			char buf[80];
			char buf1[40];
			struct tm  ts;
		    ts = *localtime(&firstBootTime);
    		strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
   	        
   	        snprintf(buf1, sizeof(buf1), "%.2f", drift);
   	        
            cJSON_AddStringToObject(root, "datetime", getCurrentDatetime());
			cJSON_AddItemToObject(root, "Status", statustype);    
			cJSON_AddItemToObject(root, "Battery", battery);    

		    cJSON_AddStringToObject(statustype, "Status", status);
		    cJSON_AddStringToObject(statustype, "StatusText", status1);
		    cJSON_AddNumberToObject(statustype, "Resets", resets);
   			cJSON_AddNumberToObject(statustype, "Wakeups", wakeups);	
//   			cJSON_AddNumberToObject(statustype, "LastWait", lastWait);	
//   			cJSON_AddNumberToObject(statustype, "LastWaitMin", lastWaitMin);	
//   			cJSON_AddNumberToObject(statustype, "LastWaitMax", lastWaitMax);	
   			cJSON_AddNumberToObject(statustype, "LastAdaptive", lastAdaptive);	
   			cJSON_AddStringToObject(statustype, "FirstBootTime", (const char *)&buf);	
   			cJSON_AddStringToObject(statustype, "BuildDatetime", get_build_datetime());	
   			cJSON_AddStringToObject(statustype, "Wifi", wifiSignalS);
   			cJSON_AddStringToObject(statustype, "NTPDrift", buf1);
   			
			cJSON_AddNumberToObject(battery, "Voltage", voltage);   			
			cJSON_AddNumberToObject(battery, "SOC", soc);   			
   			//Debug

   			if (nextStartTime) {
//				   ESP_LOGI(TAG, "DEBUG: laststarttime is not NULL");
				   if (nextStartTime[0] != '\0') {
//					   ESP_LOGI(TAG, "DEBUG: laststarttime is not \\0");
//					   ESP_LOGI(TAG, "DEBUG string: %s", lastStartTime);
					   cJSON_AddStringToObject(statustype, "PlannedStartTime", nextStartTime);
				   } else {
//					   ESP_LOGI(TAG, "DEBUG: laststarttime is \\0");
				   }
			}

   			if (startTime) {
//				   ESP_LOGI(TAG, "DEBUG: starttime is not NULL");
				   if (startTime[0] != '\0') {
//					   ESP_LOGI(TAG, "DEBUG: starttime is not \\0");
//					   ESP_LOGI(TAG, "DEBUG string: %s", startTime);
					   cJSON_AddStringToObject(statustype, "RealStartTime", startTime);
				   } else {
//					   ESP_LOGI(TAG, "DEBUG: starttime is \\0");
				   }
			}

   			//
//			if (startTime) {
//				if (startTime[0] != '\0')
//					cJSON_AddStringToObject(statustype, "StartTime", startTime);
//    		}
    		
//			if (lastStartTime[0] != '\0')    			
//   				cJSON_AddStringToObject(statustype, "LastStartTime", lastStartTime);
//			else
//    			cJSON_AddNullToObject(statustype, "LastStartTime");	
   					
   			json_string = strdup(cJSON_PrintUnformatted(root));		
			break;
		}
        case CMD_DEBUG: {
			char *debug = va_arg(args, char *);
            char *debug1 = va_arg(args, char *);
		    ESP_LOGI(TAG, "Sending status message... (Debug: %s, Debug1: %s)", debug, debug1);
		    
            cJSON_AddStringToObject(root, "datetime", getCurrentDatetime());
			cJSON_AddItemToObject(root, "debug", statustype);    
		    cJSON_AddStringToObject(statustype, "Debug", debug);
    		cJSON_AddStringToObject(statustype, "Debug1", debug1);

    		json_string = strdup(cJSON_PrintUnformatted(root));
    		break;
		}
		
    }

    if (esp_mqtt_client_publish(client, MQTT_STATUS_TOPIC, json_string, 0, 1, 0) > 0) successful_sends++; else failed_sends++;
    cJSON_Delete(root);
    free(json_string);

    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Status sending finished. Success: %d, Failed: %d", successful_sends, failed_sends);

    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
}

void mqtt_send_debug(const char *topic, const char *payload) {
	custom_mqtt_send_status(2, topic, payload);	
}

char strftime_buf[64];
char * getCurrentDatetime() {
	time_t now = 0;
    time(&now);
    struct tm  ts = *localtime(&now);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &ts);
	return strftime_buf;
}

void custom_mqtt_send_debug_queue_data(void) { 
	char s[20];
    for (int i = 0; i < storage_get_queue_count(); i++) {
        data_queue_entry_t entry;
        if (storage_get_queue_entry(i, &entry)) {
			//entry.data.obis_1_8_0)
			snprintf(s,20,"%lf", entry.data.obis_1_8_0);
			my_debug_log("MQTTDEBUG", s);
		}
	}	
}
