#pragma once
#include "esp_err.h"

#define MAX17048_DEFAULT_ADDR 0x36

void battery_init(void);
esp_err_t battery_get_status(float *voltage, float *soc);
