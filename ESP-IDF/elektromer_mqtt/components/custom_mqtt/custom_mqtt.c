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

static const char *TAG = "CUSTOM_MQTT";
//static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static esp_mqtt_client_handle_t client = NULL;
static int successful_sends = 0;
static int failed_sends = 0;

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

            cJSON_AddStringToObject(root, "datetime", buf);
            cJSON_AddBoolToObject(root, "first_boot", entry.first_after_restart);
			cJSON_AddItemToObject(root, "values", values);    

            cJSON_AddNumberToObject(values, "1.8.0", entry.data.obis_1_8_0);
            cJSON_AddNumberToObject(values, "1.8.1", entry.data.obis_1_8_1);
            cJSON_AddNumberToObject(values, "1.8.2", entry.data.obis_1_8_2);
            cJSON_AddNumberToObject(values, "1.8.3", entry.data.obis_1_8_3);
            cJSON_AddNumberToObject(values, "1.8.4", entry.data.obis_1_8_4);

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

void custom_mqtt_send_status_old(float voltage, float soc) {
    // TODO: Implementace stavové zprávy - velmi podobná send_data,
    // ale vytvoří jiný JSON a pošle na MQTT_STATUS_TOPIC
    ESP_LOGI(TAG, "Sending status message... (Voltage: %.2fV, SoC: %.1f%%)", voltage, soc);

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
	cJSON *battery = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "battery", battery);    
    
    cJSON_AddNumberToObject(battery, "Voltage", voltage);
    cJSON_AddNumberToObject(battery, "SOC", soc);

    char *json_string = cJSON_PrintUnformatted(root);
    if (esp_mqtt_client_publish(client, MQTT_STATUS_TOPIC, json_string, 0, 1, 0) > 0) successful_sends++; else failed_sends++;
    cJSON_Delete(root);
    free(json_string);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Data sending finished. Success: %d, Failed: %d", successful_sends, failed_sends);

    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
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
    char *json_string = NULL;
    
    switch (mqtt_type) {
        case CMD_BATTERY: {
			double voltage = va_arg(args, double);
            double soc = va_arg(args, double);
		    ESP_LOGI(TAG, "Sending status message... (Voltage: %.2fV, SoC: %.1f%%)", voltage, soc);
		    
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
			cJSON_AddItemToObject(root, "Status", statustype);    
		    cJSON_AddStringToObject(statustype, "Status", status);
		    cJSON_AddStringToObject(statustype, "StatusText", status1);
		    cJSON_AddNumberToObject(statustype, "Resets", resets);
   			cJSON_AddNumberToObject(statustype, "Wakeups", wakeups);	
   			json_string = strdup(cJSON_PrintUnformatted(root));		
			break;
		}
    }

    if (esp_mqtt_client_publish(client, MQTT_STATUS_TOPIC, json_string, 0, 1, 0) > 0) successful_sends++; else failed_sends++;
    cJSON_Delete(root);
    free(json_string);

    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Data sending finished. Success: %d, Failed: %d", successful_sends, failed_sends);

    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
}
