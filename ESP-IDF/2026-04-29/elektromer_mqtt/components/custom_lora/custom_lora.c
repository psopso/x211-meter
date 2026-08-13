#include "esp_log.h"
#include "esp_err.h"
#include "custom_lora.h"
#include "lora.h"
#include "driver/spi_master.h"
#include "esp_timer.h"

static const char *TAG = "CUSTOM_LORA";

//int lorainitialized = 0;

esp_err_t custom_lora_init() {
  return ESP_OK;
}

bool lora_ready = false;

esp_err_t safe_lora_init() {
    if (lora_ready) return ESP_OK;
    int ret = lora_init();
    ESP_LOGI(TAG, "Po LoraInit");
    if (ret != 0) {
        lora_ready = true;
        lora_set_frequency(433e6); // 433MHz
        lora_set_tx_power(0);
        lora_dump_registers();
        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "LoraInit failed");
        // Knihovna selhala, ale mohla nechat SPI inicializované.
        // Zkusíme ho uvolnit "naslepo".
        spi_bus_free(SPI2_HOST); 
        return ESP_FAIL;
    }
}
//type, data, sizeof(comm_data_t)
esp_err_t custom_lora_send_packet(uint8_t *buf, int len ) {
	ESP_LOGI(TAG, "Inicializuji LoRa.");
    if (safe_lora_init() != ESP_OK)
      return ESP_FAIL;

	ESP_LOGI(TAG, "Odesilam LoRa paket. Len: %d", len);
    lora_send_packet(buf, len);
	return ESP_OK;
}

bool wait_for_lora(uint32_t timeout_ms)
{
    int64_t start = esp_timer_get_time();
    int64_t timeout_us = (int64_t)timeout_ms * 1000;

    while (lora_received() == 0) {
        if ((esp_timer_get_time() - start) > timeout_us) {
            return false; // timeout
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // uvolní CPU
    }

    return true; // data dorazila
}

//int lora_receive_packet(uint8_t *buf, int size);
//int lora_received(void);
esp_err_t custom_lora_receive_packet() {
	lora_receive();
	if (!wait_for_lora(5000)) {
	    ESP_LOGI(TAG, "Timeout pri prijmu.");
	    return ESP_ERR_TIMEOUT;
	} else {
		uint8_t buf[255];
		int rxlen = lora_receive_packet(buf, 255);
	    ESP_LOGI(TAG,"Data prijata: %d RSSI: %d", rxlen, lora_packet_rssi());	    
	}
	return ESP_OK;
}
