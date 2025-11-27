#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/ip_addr.h"
#include "nvs_flash.h"
#include "custom_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "custom_wifi";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static TaskHandle_t reconnect_task_handle = NULL;
static char saved_ssid[32];
static char saved_password[64];

// === Forward deklarace ===
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
static void wifi_reconnect_task(void *pv);

static esp_netif_t *wifi_netif = NULL;

esp_err_t custom_wifi_init(const char *ssid, const char *password,
       const char *ip, const char *gateway, const char *netmask)
{
    strncpy(saved_ssid, ssid, sizeof(saved_ssid));
    strncpy(saved_password, password, sizeof(saved_password));

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
	wifi_netif = esp_netif_create_default_wifi_sta();
    if (!wifi_netif) {
        ESP_LOGE(TAG, "Failed to create default wifi netif!");
        return ESP_FAIL;
    }    
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// Pokud jsou zadány IP parametry, použijeme statickou IP
    if (ip && *ip && gateway && *gateway && netmask && *netmask) {
        ESP_LOGI(TAG, "Using static IP config: %s, GW: %s, NM: %s", ip, gateway, netmask);
        esp_err_t err = esp_netif_dhcpc_stop(wifi_netif);
        if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
            ESP_LOGE(TAG, "Failed to stop DHCP client: %s", esp_err_to_name(err));
            return err;
        }

        esp_netif_ip_info_t ip_info = { 0 };
        ip_info.ip.addr = esp_ip4addr_aton(ip);
        ip_info.gw.addr = esp_ip4addr_aton(gateway);
        ip_info.netmask.addr = esp_ip4addr_aton(netmask);
        ESP_ERROR_CHECK(esp_netif_set_ip_info(wifi_netif, &ip_info));
    }

	// Po nastavení IP:
	esp_netif_dns_info_t dns;
	dns.ip.u_addr.ip4.addr = esp_ip4addr_aton(gateway);
	dns.ip.type = IPADDR_TYPE_V4;
	ESP_ERROR_CHECK(esp_netif_set_dns_info(wifi_netif, ESP_NETIF_DNS_MAIN, &dns));
	

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, saved_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, saved_password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Spustíme separátní task pro reconnect
    xTaskCreate(wifi_reconnect_task, "wifi_reconnect_task", 4096, NULL, 5, &reconnect_task_handle);

    bool bcon = custom_wifi_is_connected();
    int concounter = 20;
    while(!bcon && (concounter > 0)) {
		concounter -= 1;
		ESP_LOGI(TAG, "Waiting for Wifi ... %i/%i", 20-concounter, 20);
    	vTaskDelay(pdMS_TO_TICKS(1000));
    	bcon = custom_wifi_is_connected();
    };
    if (bcon)
    	return ESP_OK;
    else
    	return ESP_FAIL;
}

void custom_wifi_reconnect(void)
{
    if (reconnect_task_handle) {
        xTaskNotifyGive(reconnect_task_handle);
    }
}

void custom_wifi_disconnect(void)
{
    esp_wifi_disconnect();
}

bool custom_wifi_is_connected(void)
{
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}

static void wifi_reconnect_task(void *pv)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "Reconnecting Wi-Fi...");
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_wifi_connect();
    }
}

/**
 * @brief Získá aktuální RSSI (sílu signálu) připojené Wi-Fi.
 */
esp_err_t custom_wifi_get_rssi(int8_t *rssi_out)
{
    if (rssi_out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Struktura pro uložení informací o AP
    wifi_ap_record_t ap_info;

    // Zavoláme API funkci pro získání informací o připojeném AP
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);

    if (err == ESP_OK) {
        // Povedlo se, uložíme RSSI
        *rssi_out = ap_info.rssi;
        return ESP_OK;
    } else {
        // Nepovedlo se (pravděpodobně nejsme připojeni)
        // Vrátíme chybový kód, který jsme dostali
        *rssi_out = 0; // Pro jistotu nastavíme výstup na 0
        return err;
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Disconnected, scheduling reconnect...");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        custom_wifi_reconnect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

#include "esp_log.h"
#include "custom_wifi.h" // Ujistěte se, že máte tento include

// ... v nějakém tasku nebo funkci:

int8_t check_signal_strength(void)
{
    int8_t current_rssi = 0;
    esp_err_t ret = custom_wifi_get_rssi(&current_rssi);

    if (ret == ESP_OK) {
        ESP_LOGI("WIFI_RSSI", "Aktuální síla signálu: %d dBm", current_rssi);
        
        // Hodnoty RSSI jsou záporné:
        // -30 dBm: Vynikající (stojíte vedle routeru)
        // -60 dBm: Velmi dobrý
        // -70 dBm: Dobrý, spolehlivý
        // -80 dBm: Slabý, může vypadávat
        // -90 dBm: Velmi špatný
    } else if (ret == ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGW("WIFI_RSSI", "Nelze získat RSSI, Wi-Fi není připojena.");
    } else {
        ESP_LOGE("WIFI_RSSI", "Chyba při získávání RSSI: %s", esp_err_to_name(ret));
    }
    return current_rssi;
}
