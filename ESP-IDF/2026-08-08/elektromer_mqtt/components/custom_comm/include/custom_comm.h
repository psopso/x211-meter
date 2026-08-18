#ifndef CUSTOM_COMM_H
#define CUSTOM_COMM_H

#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "custom_dlms.h" 

// PŘEPÍNAČ TECHNOLOGIÍ (Zatím natvrdo WiFi, později přidáme LoRa)
#define USE_COMM_WIFI 

typedef enum {
    MSG_TYPE_METER_DATA, 
    MSG_TYPE_STATUS,     
    MSG_TYPE_BATTERY     
} comm_msg_type_t;

typedef struct {
    const char *state;          
    const char *reason;         
    int boot_count;
    int wakeup_cycle_count;
    time_t first_boot_time;
    
    int wait_time;
    unsigned char wait_time_min;
    unsigned char wait_time_max;
    int32_t adaptive_offset;
    double ntp_drift;
    
    char start_time_str[64];
    char next_wakeup_str[64];
    
    int rssi;                   
    
    float battery_voltage;
    float battery_soc;
    
    dlms_data_t meter_data;     
} comm_data_t;

// API pro hlavní program
esp_err_t comm_init(void);
bool comm_is_connected(void);
esp_err_t comm_send(comm_msg_type_t type, comm_data_t *data);
int8_t check_signal_strength();
esp_err_t comm_ota_check_and_do_update(const char *url, bool forceota);
void comm_time_sync_from_ntp();

// 1. Zkomprimovaný JEDEN záznam pro LoRa
#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp;       // 4 byty (UNIX čas)
    uint32_t obis_1_8_0_Wh;   // 4 byty (Total Wh)
    uint32_t obis_1_8_1_Wh;   // 4 byty (T1 Wh)
    uint32_t obis_1_8_2_Wh;   // 4 byty (T2 Wh)
    uint8_t first_after_restart; // 1 byt (bool)
} lora_queue_item_t;          // CELKEM: Jen 17 bajtů na záznam!
#pragma pack(pop)

// 2. Struktura samotného odesílaného LoRa paketu
#define LORA_MAX_ITEMS_PER_PACKET 4

#pragma pack(push, 1)
typedef struct {
    uint8_t packet_type;      // Např. MSG_TYPE_METER_DATA (1 byt)
    uint8_t item_count;       // Kolik záznamů paket reálně obsahuje (1 byt)
    lora_queue_item_t items[LORA_MAX_ITEMS_PER_PACKET]; // 4 * 17 = 68 bajtů
} lora_data_payload_t;        // CELÝ PAKET: 70 bajtů (Ideální pro LoRa!)
#pragma pack(pop)

#endif // CUSTOM_COMM_H
