#pragma once
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

typedef enum {
    CMD_BATTERY,
    CMD_STATUS,
    CMD_DEBUG
} mqtt_type_t;


void custom_mqtt_send_data(void);
//void custom_mqtt_send_status_old(float voltage, float soc);
void custom_mqtt_send_status(mqtt_type_t mqtt_type, ...);

void mqtt_send_debug(const char *topic, const char *payload);
void custom_mqtt_send_debug_queue_data(void);
