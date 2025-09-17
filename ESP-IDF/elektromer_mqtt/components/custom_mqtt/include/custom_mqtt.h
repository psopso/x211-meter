#pragma once
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

typedef enum {
    CMD_BATTERY,
    CMD_STATUS
} mqtt_type_t;


void custom_mqtt_send_data(void);
//void custom_mqtt_send_status_old(float voltage, float soc);
void custom_mqtt_send_status(mqtt_type_t mqtt_type, ...);
