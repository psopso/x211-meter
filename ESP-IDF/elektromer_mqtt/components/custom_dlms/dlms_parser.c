#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "dlms_parser.h"

#define MAX_VALUE_LEN  64       // maximální délka hodnoty

static const char *TAG = "DLMS";

/// pomocná funkce: formát OBIS do "A-B:C.D.E.F"
static void format_obis(char *out, size_t out_size,
                        uint8_t a, uint8_t b, uint8_t c,
                        uint8_t d, uint8_t e, uint8_t f)
{
//    snprintf(out, out_size, "%u-%u:%u.%u.%u.%u", a, b, c, d, e, f);
    snprintf(out, out_size, "%u.%u.%u", d, e, f);
}

/// pomocná funkce: převod pole na hex string
static void bytes_to_hex(char *out, size_t out_size,
                         const uint8_t *data, size_t len)
{
    size_t pos = 0;
    for (size_t i = 0; i < len && pos + 2 < out_size; i++) {
        pos += snprintf(out + pos, out_size - pos, "%02x", data[i]);
    }
}

dlms_result_t result;
/// hlavní DLMS parser
dlms_result_t parse_dlms(const uint8_t *buf, size_t len)
{
//    dlms_result_t result;
    memset(&result, 0, sizeof(result));
    bool requiredOBIS = false;
    
    for (size_t i = 0; i + 6 < len; i++) {
        // hledáme xx xx xx xx xx xx 0xFF
        if (i + 7 < len && buf[i+6] == 0xFF) {
            if (result.count >= DLMS_ITEMS_EXP) break;

            dlms_item_t *item = &result.items[result.count];
            if (buf[i+3] == 0x01)
              requiredOBIS = true;
            else
              requiredOBIS = false;
            
            format_obis(item->obis, sizeof(item->obis),
                        buf[i], buf[i+1], buf[i+2],
                        buf[i+3], buf[i+4], buf[i+5]);

            size_t p = i + 7;
            if (p + 1 >= len) continue;

            uint8_t dtype = buf[p+1];  // typ dat
            size_t pos = p + 2;

            switch (dtype) {
                case 0x09: { // Octet string (text)
                    if (pos < len) {
                        uint8_t slen = buf[pos++];
                        if (pos + slen <= len && slen < MAX_VALUE_LEN) {
                            memcpy(item->value, buf + pos, slen);
                            item->value[slen] = 0;
                            pos += slen;
                        }
                    }
                    break;
                }
                case 0x06: { // 32bit signed
                    if (pos + 4 <= len) {
                        int32_t v = ((int32_t)buf[pos] << 24) |
                                    ((int32_t)buf[pos+1] << 16) |
                                    ((int32_t)buf[pos+2] << 8) |
                                    (int32_t)buf[pos+3];
                        snprintf(item->value, sizeof(item->value), "%ld", (long)v);
                        pos += 4;
                    }
                    break;
                }
                case 0x12: { // 16bit unsigned
                    if (pos + 2 <= len) {
                        uint16_t v = ((uint16_t)buf[pos] << 8) | buf[pos+1];
                        snprintf(item->value, sizeof(item->value), "%u", v);
                        pos += 2;
                    }
                    break;
                }
                case 0x10: { // 8bit unsigned
                    if (pos < len) {
                        snprintf(item->value, sizeof(item->value), "%u", buf[pos]);
                        pos++;
                    }
                    break;
                }
                case 0x0A: { // Octet string (čas/datum)
                    if (pos < len) {
                        uint8_t slen = buf[pos++];
                        if (pos + slen <= len) {
                            bytes_to_hex(item->value, sizeof(item->value),
                                         buf + pos, slen);
                            pos += slen;
                        }
                    }
                    break;
                }
                default: {
                    if (pos < len) {
                        snprintf(item->value, sizeof(item->value),
                                 "0x%02x", buf[pos]);
                        pos++;
                    }
                    break;
                }
            }
            if (item->value[0] != 0) {
				if (requiredOBIS)
                	result.count++;
            }
        }
    }

    return result;
}

/// příklad výpisu výsledků
void dlms_print_result(const dlms_result_t *res)
{
    for (size_t i = 0; i < res->count; i++) {
        ESP_LOGI(TAG, "OBIS %s => %s", res->items[i].obis, res->items[i].value);
    }
}
