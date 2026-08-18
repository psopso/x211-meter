#pragma once
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "custom_comm.h"
#include "lora.h"

esp_err_t custom_lora_init();
esp_err_t custom_lora_send_packet(uint8_t *buf, int len );
esp_err_t custom_lora_receive_packet();
