#include "custom_comm.h"
#include "esp_err.h"
#include "esp_log.h"
#include "custom_ota.h"
#include "custom_time.h"
#include "custom_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Tady vložíme závislosti na WiFi a MQTT jen pro tento soubor
#include "custom_config.h" // Pro SSID, hesla atd.
#include "custom_wifi.h"
#include "include/custom_comm.h"

#ifdef USE_COMM_WIFI

#include "custom_mqtt.h"
#else
    #include "custom_lora.h"
	void convert_comm_data_to_lora_data(comm_msg_type_t type, comm_data_t *commdata, lora_universal_packet_t *packet, int *items_sent, size_t *out_size);
#endif

static const char *TAG = "COMM";
bool wifi_is_initialized = false;

esp_err_t comm_init(void) {
#ifdef USE_COMM_WIFI
    ESP_LOGI(TAG, "Inicializace komunikace: WIFI + MQTT");
	wifi_is_initialized = true;
    return custom_wifi_init(WIFI_SSID, WIFI_PASSWORD, WIFI_IP, WIFI_GW, WIFI_NETMASK);
#else
    ESP_LOGI(TAG, "Inicializace komunikace: LORA (zatim neni implementovano)");
    return custom_lora_init();
    //return ESP_OK;
#endif
}

bool comm_is_connected(void) {
#ifdef USE_COMM_WIFI
    return custom_wifi_is_connected();
#else
    // LoRa nevyžaduje "připojení" k AP, je vždy připravena vysílat
    return true; 
#endif
}

esp_err_t comm_send(comm_msg_type_t type, comm_data_t *data) {
	ESP_LOGI(TAG, "Comm Send: %d", type);
    if (data == NULL) return ESP_ERR_INVALID_ARG;

#ifdef USE_COMM_WIFI
    if (!custom_wifi_is_connected()) return ESP_FAIL;

    switch (type) {
        case MSG_TYPE_METER_DATA:
			ESP_LOGI(TAG, "Odeslani DATA do mqtt");
            // Tady pošleme JEN data z elektroměru
            // Předpokládám, že vaše funkce custom_mqtt_send_data 
            // interně bere data z 'parsed_data'. 
            // Můžete ji upravit, aby brala 'data->meter_data'.
//            return custom_mqtt_send_data(&(data->meter_data)); 
            custom_mqtt_send_data();
			return ESP_OK;

        case MSG_TYPE_STATUS:
			ESP_LOGI(TAG, "Odeslani STATUS do mqtt");
            // Tady pošleme diagnostický balík proměnných
            /*return*/ custom_mqtt_send_status(
                CMD_STATUS, 
                data->state, 
                data->reason, 
                data->boot_count, 
                data->wakeup_cycle_count,
				data->first_boot_time,
//				data->wait_time,
//                data->wait_time_min,
//				data->wait_time_max,
				data->rssi,
				data->adaptive_offset,
				data->start_time_str,
				data->next_wakeup_str,
				data->ntp_drift,
				(double)data->battery_voltage,
				(double)data->battery_soc
                // ... a tak dále (všechny ty diagnostické proměnné)
//custom_mqtt_send_status(CMD_STATUS, "OK", "After wakeup", bootCount, 
//   wakeUpCycleCounter, firstBootTime, waitTime, waitTimeMin, waitTimeMax, 
//   check_signal_strength(), adaptive_offset, s_short, next_wakeup_str_pers, drift);
//	
//    state, reason,boot_count,wakeup_cycle_count,first_boot_time,wait_time,
//    wait_time_min,wait_time_max,RSSI,adaptive_offset,startTime,next_wakeup_str,drift;
				
            );
			return ESP_OK;

        case MSG_TYPE_BATTERY:
			ESP_LOGI(TAG, "Odeslani BATTERY do mqtt");
            // Tady pošleme jen info o baterce
            /*return*/// custom_mqtt_send_battery(data->battery_voltage, data->battery_soc);
            custom_mqtt_send_status(CMD_BATTERY, 
              data->battery_voltage, 
              data->battery_soc
			);
			return ESP_OK;
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
#else
    // LoRa sekce uvnitř comm_send
    lora_universal_packet_t lora_packet = {0};
    lora_packet.network_id = MY_SECRET_NETWORK_ID;
    lora_packet.sender_id = SENDER_ID_METER;

    int items_sent = 0;
    size_t payload_size = 0;

    // Tahle jedna funkce obslouží DATA, STATUS i BATTERY!
    convert_comm_data_to_lora_data(type, data, &lora_packet, &items_sent, &payload_size);

    if (payload_size > 0) {


//Následující řádky bude možno po odladění smazat
//        ESP_LOGI(TAG, "Odesilam LoRa paket. Typ: %d, Polozek: %d, Velikost: %zu B", 
//                 type, items_sent, payload_size);
//                 
//        esp_err_t espstatus = custom_lora_send_packet((uint8_t *)&lora_packet, payload_size); 
//        // ... timeout, cekani na ACK ...
//        if (espstatus == ESP_OK)
//        	espstatus = custom_lora_receive_packet();
//        return espstatus;

		ESP_LOGI(TAG, "Odesilam LoRa paket. Typ: %d, Polozek: %d, Velikost: %zu B", 
           type, items_sent, payload_size);

		esp_err_t espstatus = ESP_FAIL;
		
		for (int attempt = 1; attempt <= LORA_MAX_RETRIES; attempt++) {
		    ESP_LOGI(TAG, "--- Pokus o odeslani: %d/%d ---", attempt, LORA_MAX_RETRIES);
		    
		    // 1. Odeslání paketu
		    espstatus = custom_lora_send_packet((uint8_t *)&lora_packet, payload_size); 
		    
		    if (espstatus == ESP_OK) {
		        // 2. Vysílání proběhlo, jdeme poslouchat odpověď (ACK)
		        espstatus = custom_lora_receive_packet();
		        
		        if (espstatus == ESP_OK) {
		            ESP_LOGI(TAG, "ACK uspesne prijato! Attempt: %d", attempt);
		            break; // ÚSPĚCH! Vyskakujeme z for-smyčky a jdeme dál
		            
		        } else if (espstatus == ESP_ERR_TIMEOUT) {
		            ESP_LOGW(TAG, "Vyprsel timeout pro prijem ACK (pokus %d).", attempt);
		        } else {
		            ESP_LOGE(TAG, "Jina chyba pri prijmu: %s", esp_err_to_name(espstatus));
		        }
		    } else {
		        ESP_LOGE(TAG, "Chyba pri samotnem vysilani: %s", esp_err_to_name(espstatus));
		        // Pokud selže už samotná komunikace s čipem (SPI), nemá smysl hned opakovat
		    }
		
		    // 3. Pokud to nebyl poslední pokus a nedopadlo to dobře, chvíli počkáme
		    if (attempt < LORA_MAX_RETRIES) {
		        ESP_LOGI(TAG, "Cekam %d ms pred dalsim pokusem...", LORA_RETRY_DELAY_MS);
		        vTaskDelay(pdMS_TO_TICKS(LORA_RETRY_DELAY_MS));
		    }
		}

		// Vrátí ESP_OK (pokud se to v nějakém pokusu povedlo), 
		// nebo poslední chybový kód (pokud selhaly všechny pokusy)
		if (espstatus == ESP_OK) {
		    //Tady bychom měli smazat něco z počátku fronty
		    //Možná budeme mazat pouze, pokud fronta bude delší, než 8 (=>)  LORA_MAX_ITEMS_PER_PACKET=8
		    //a smažeme právě 8 záznamů
		    if (storage_get_queue_count() >= LORA_MAX_ITEMS_PER_PACKET) {
		      ESP_LOGI(TAG,"Mazu %d zaznamů z fronty.", LORA_MAX_ITEMS_PER_PACKET);
                      storage_remove_entries(LORA_MAX_ITEMS_PER_PACKET);
		    }
		}
		return espstatus;
    }
    
    return ESP_FAIL;
#endif
}

int8_t check_signal_strength() {
#ifdef USE_COMM_WIFI
	return check_wifi_signal_strength();
#else
	return 0;
#endif

}

esp_err_t comm_ota_check_and_do_update(const char *url, bool forceota) {
#ifdef USE_COMM_WIFI
	return simple_ota_check_and_do_update(url);
#else
	if (forceota) {
		if (!wifi_is_initialized) {
			custom_wifi_init(WIFI_SSID, WIFI_PASSWORD, WIFI_IP, WIFI_GW, WIFI_NETMASK);
			wifi_is_initialized = true;
		}
		return simple_ota_check_and_do_update(url);
	} else {
		return ESP_ERR_NOT_SUPPORTED; //ESP_OK;
	}
#endif
}

void comm_time_sync_from_ntp() {
#ifdef USE_COMM_WIFI
    ESP_LOGI(TAG, "Debug: time_sync_from_ntp");
	time_sync_from_ntp();
#else
    ESP_LOGI(TAG, "Debug: ---");
#endif
}

 //Queue size 100
//[LORA_MAX_ITEMS_PER_PACKET]; // 4 * 17 = 68 bytes  8 * 17 = 136  100/8 = 12 paketů = celá fronta
//když odešlu 3 pakety, tak to mám celou frontu na 4x
//100 byte 300ms, 8 paketů = 140 bytů, tedy asi 400ms, mohu poslat 16 paketů do 1 sekundy. Tedy celou frontu pošlu na 6x
#ifndef USE_COMM_WIFI
// Pomocná funkce pro převod stringu na číslo (upravte podle vašich reálných stavů)
uint8_t map_state_to_code(const char* state) {
    if (state == NULL) return 0;
    if (strcmp(state, "OK") == 0) return 0;
    if (strcmp(state, "SLEEP") == 0) return 1;
    if (strcmp(state, "ERROR") == 0) return 99;
    return 255; // Neznámý
}

//void convert_comm_data_to_lora_data(comm_msg_type_t type, comm_data_t *commdata, lora_data_payload_t *payload, int *items_sent) {
void convert_comm_data_to_lora_data(comm_msg_type_t type, comm_data_t *commdata, lora_universal_packet_t *packet, int *items_sent, size_t *out_size) {	
	packet->packet_type = (uint8_t)type; // Zajištění, že paket má správný typ
	
    switch (type) {
        case MSG_TYPE_METER_DATA:
		  ESP_LOGI(TAG, "Odeslani DATA do mqtt");
		  *items_sent = 0;
	      for (int i = 0; i < storage_get_queue_count() && i < LORA_MAX_ITEMS_PER_PACKET; i++) {
	        data_queue_entry_t entry;
	        if (storage_get_queue_entry(i, &entry)) {
	            data_queue_entry_t *src = &entry;
	            lora_queue_item_t *dst = &packet->payload.items[i];
	            
	            dst->timestamp = (uint32_t)src->timestamp;
	            // Převod kWh (double) na Wh (uint32)
	            dst->obis_1_8_0_Wh = (uint32_t)(src->data.obis_1_8_0 * 1000.0);
	            dst->obis_1_8_1_Wh = (uint32_t)(src->data.obis_1_8_1 * 1000.0);
	            dst->obis_1_8_2_Wh = (uint32_t)(src->data.obis_1_8_2 * 1000.0);
	            dst->first_after_restart = src->first_after_restart ? 1 : 0;
	            if (src->waittime > 255)
	              dst->waittime = 255;
	            else
	            	dst->waittime = src->waittime;	
	            *items_sent += 1;
	      	}
      	  }
      	  packet->item_count = (uint8_t)*items_sent;
      	  // Spočítáme přesnou velikost paketu (Hlavička + počet záznamů)
          *out_size = offsetof(lora_universal_packet_t, payload) + (*items_sent * sizeof(lora_queue_item_t));
      	  return;
      	case MSG_TYPE_STATUS:
  		    ESP_LOGI(TAG, "Priprava STATUS pro LoRa");
            if (commdata == NULL) return; // Ochrana
            
            *items_sent = 1;
            packet->item_count = 1;
            
            lora_status_item_t *dst = &packet->payload.status;
            
            // Konverze textů na čísla
            dst->state_code = map_state_to_code(commdata->state);
            dst->reason_code = 0; // Tady si můžete udělat podobnou mapovací funkci
            
            // Přímé přiřazení a oříznutí velikostí
            dst->boot_count = (uint32_t)commdata->boot_count;
            dst->wakeup_cycle_count = (uint32_t)commdata->wakeup_cycle_count;
            dst->first_boot_time = (uint32_t)commdata->first_boot_time;
            dst->adaptive_offset = commdata->adaptive_offset;
            
            // Konverze zlomků na celá čísla (drift z double na int16 v ms)
            dst->ntp_drift_ms = (int16_t)(commdata->ntp_drift * 1000.0);
            
            dst->rssi = (int8_t)commdata->rssi;
            dst->batt_voltage_mv = (uint16_t)(commdata->battery_voltage * 1000.0);
            dst->batt_soc = (uint8_t)commdata->battery_soc;

            // Řetězce start_time_str a next_wakeup_str NEPOSÍLÁME!
            // ESPHome si je na straně serveru případně vypočítá z času a wait_time.

            *out_size = offsetof(lora_universal_packet_t, payload) + sizeof(lora_status_item_t);
      	    return;
      	case MSG_TYPE_BATTERY:
      	  return;
      }
}
#endif
