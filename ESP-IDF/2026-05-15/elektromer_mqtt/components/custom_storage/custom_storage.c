#include "custom_storage.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include <string.h>

static const char *TAG = "STORAGE";

// Uložení fronty do RTC paměti, aby přežila deep sleep
RTC_DATA_ATTR data_queue_entry_t data_queue[QUEUE_SIZE];
RTC_DATA_ATTR int queue_head = 0;
RTC_DATA_ATTR int queue_tail = 0;
RTC_DATA_ATTR int queue_count = 0;

void storage_init(void) {
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        memset(data_queue, 0, sizeof(data_queue));
        queue_head = 0;
        queue_tail = 0;
        queue_count = 0;
        ESP_LOGI(TAG, "Storage queue initialized (cleared).");
    } else {
        ESP_LOGI(TAG, "Storage restored from RTC. Items in queue: %d", queue_count);
    }
}

void storage_add_to_queue(const dlms_data_t *data, time_t timestamp, bool is_first_boot, int waittime) {
    if (queue_count >= QUEUE_SIZE) {
        // Fronta je plná, přepíšeme nejstarší záznam
        ESP_LOGW(TAG, "Queue is full. Overwriting the oldest entry.");
        queue_tail = (queue_tail + 1) % QUEUE_SIZE;
        queue_count--;
    }

    data_queue[queue_head].data = *data;
    data_queue[queue_head].timestamp = timestamp;
    data_queue[queue_head].first_after_restart = is_first_boot;
    data_queue[queue_head].waittime = waittime;
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count++;
    ESP_LOGI(TAG, "Data added to queue. : %d", queue_count);
}

void storage_cleanup_old_data(void) {
    time_t now;
    time(&now);
    time_t retention_limit = now - (DATA_RETENTION_HOURS * 3600);
    
    int items_removed = 0;
    while (queue_count > 0 && data_queue[queue_tail].timestamp < retention_limit) {
        queue_tail = (queue_tail + 1) % QUEUE_SIZE;
        queue_count--;
        items_removed++;
    }
    if (items_removed > 0) {
        ESP_LOGI(TAG, "Removed %d old data entries from queue. New count: %d", items_removed, queue_count);
    }
}

int storage_get_queue_count(void) {
    return queue_count;
}

bool storage_get_queue_entry(int index, data_queue_entry_t *entry) {
    if (index < 0 || index >= queue_count) {
        return false;
    }
    int internal_index = (queue_tail + index) % QUEUE_SIZE;
    *entry = data_queue[internal_index];
    return true;
}

/**
 * Odstraní prvních n nejstarších záznamů z fronty.
 * Používá se po úspěšném potvrzení přijetí balíčku LoRa bránou.
 */
void storage_remove_entries(int n) {
    if (n <= 0) {
        return;
    }

    // Pokud bychom chtěli smazat víc, než máme, smažeme prostě všechno
    if (n > queue_count) {
        n = queue_count;
    }

    // Posuneme tail (konec fronty) o n pozic dopředu s ohledem na modulo
    queue_tail = (queue_tail + n) % QUEUE_SIZE;
    
    // Snížíme celkový počet prvků ve frontě
    queue_count -= n;

    ESP_LOGI(TAG, "Removed %d entries from queue. Remaining: %d", n, queue_count);

    // Pokud je fronta prázdná, pro jistotu srovnáme head a tail (kosmetická úprava)
    if (queue_count == 0) {
        queue_head = 0;
        queue_tail = 0;
    }
}
