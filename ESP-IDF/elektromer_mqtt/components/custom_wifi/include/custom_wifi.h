#pragma once

#include "esp_err.h"
#include <stdbool.h>

// Inicializace Wi-Fi, automaticky se připojí k síti
esp_err_t custom_wifi_init(const char *ssid, const char *password,
   const char *ip, const char *gateway, const char *netmask);

// Funkce pro ruční reconnect (např. po vypnutí AP)
void custom_wifi_reconnect(void);

// Funkce pro odpojení
void custom_wifi_disconnect(void);

// Funkce pro zjištění, zda je připojeno
bool custom_wifi_is_connected(void);
