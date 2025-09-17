#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "string.h"

// Struktura pro uložení naparsovaných dat
typedef struct {
    double obis_1_8_0; // Active energy+ total
    double obis_1_8_1; // Active energy+ in T1
    double obis_1_8_2; // Active energy+ in T2
    double obis_1_8_3; // Active energy+ in T3
    double obis_1_8_4; // Active energy+ in T4
    // Zde můžete přidat další OBIS kódy podle potřeby
} dlms_data_t;

/**
 * @brief Parsování DLMS/COSEM datového rámce z elektroměru.
 * * @param buffer Ukazatel na surová data.
 * @param length Délka dat.
 * @param data Ukazatel na strukturu, kam se uloží naparsovaná data.
 * @return ESP_OK při úspěchu, jinak ESP_FAIL.
 */
esp_err_t dlms_parse_frame(const uint8_t *buffer, int length, dlms_data_t *data);
