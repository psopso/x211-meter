// dlms_parser.h
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DLMS_ITEMS 20
#define DLMS_ITEMS_EXP 10
#define MAX_OBIS_LEN   10
#define MAX_VALUE_LEN  64

typedef struct {
    char obis[MAX_OBIS_LEN];
    char value[MAX_VALUE_LEN];
} dlms_item_t;

typedef struct {
    dlms_item_t items[DLMS_ITEMS_EXP];
    size_t count;
} dlms_result_t;

/**
 * @brief Parse DLMS frame into OBIS-value pairs
 *
 * @param buf Pointer to input DLMS frame
 * @param len Length of input buffer
 * @return dlms_result_t Structure with parsed items
 */
dlms_result_t parse_dlms(const uint8_t *buf, size_t len);

/**
 * @brief Print parsed result to ESP-IDF log
 *
 * @param res Pointer to result from parse_dlms()
 */
void dlms_print_result(const dlms_result_t *res);

#ifdef __cplusplus
}
#endif
