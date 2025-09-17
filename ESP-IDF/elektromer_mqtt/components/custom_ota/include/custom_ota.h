#pragma once

#include "esp_err.h"

/**
 * @brief Spustí OTA update z dané URL
 *
 * @param url  URL (HTTP) k binárce firmware
 * @return
 *  - ESP_OK při úspěšném OTA (ESP se restartuje)
 *  - ESP_FAIL při selhání OTA (ESP pokračuje normálně)
 */
esp_err_t simple_ota_check_and_do_update(const char *url);
