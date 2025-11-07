#include "custom_ota.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_log.h"

static const char *TAG = "CUSTOM_OTA";
uint8_t buf[4096];

esp_err_t simple_ota_check_and_do_update(const char *url)
{
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP client init failed");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HTTP open failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    int status = esp_http_client_fetch_headers(client);
    if (status < 0) {
        ESP_LOGW(TAG, "Fetch headers failed");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    int content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "Content_length: %i", content_length);
    
    if (esp_http_client_get_status_code(client) != 200 || content_length <= 0) {
        ESP_LOGI(TAG, "No new firmware found (status=%d)",
                 esp_http_client_get_status_code(client));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
    if (ota_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found!");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_ota_handle_t ota_handle;
    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, content_length, &ota_handle));
    
    int read_len, total = 0;
    while ((read_len = esp_http_client_read(client, (char *)buf, sizeof(buf))) > 0) {
        ESP_ERROR_CHECK(esp_ota_write(ota_handle, buf, read_len));
        total += read_len;
        ESP_LOGI(TAG, "Written %d/%d bytes...", total, content_length);
    }

    ESP_ERROR_CHECK(esp_ota_end(ota_handle));
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(ota_partition));

    ESP_LOGI(TAG, "OTA update successful (%d bytes), rebooting...", total);
    esp_http_client_cleanup(client);
//    ESP_LOGI(TAG, "Waiting 5s then restart");
//    vTaskDelay(pdMS_TO_TICKS(5000));
//    esp_restart();

    return ESP_OK; // nedostane se sem, protože se restartuje
}
