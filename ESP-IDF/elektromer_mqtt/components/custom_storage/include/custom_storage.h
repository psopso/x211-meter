#pragma once
#include <time.h>
#include <stdbool.h>
#include "custom_config.h"
#include "custom_dlms.h"

typedef struct {
    dlms_data_t data;
    time_t timestamp;
    bool first_after_restart;
} data_queue_entry_t;

void storage_init(void);
void storage_add_to_queue(const dlms_data_t *data, time_t timestamp, bool is_first_boot);
void storage_cleanup_old_data(void);
int storage_get_queue_count(void);
bool storage_get_queue_entry(int index, data_queue_entry_t *entry);
