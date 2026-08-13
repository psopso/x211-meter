#ifndef CUSTOM_COMM_H
#define CUSTOM_COMM_H

#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "custom_dlms.h" 
#include "custom_storage.h"

// PŘEPÍNAČ TECHNOLOGIÍ (Zatím natvrdo WiFi, později přidáme LoRa)
//#define USE_COMM_WIFI 

#define MY_SECRET_NETWORK_ID 		0xA1B2 // Vymyslete si jakékoliv 16bitové číslo
#define SENDER_ID_METER      		0x01   // ID vašeho elektroměru
#define LORA_MAX_RETRIES     		5	//Počet pokusů o vysílání paketu LoRa
#define LORA_RETRY_DELAY_MS 		2000 // Počkáme 2 vteřiny před dalším pokusem
#define LORA_MAX_ITEMS_PER_PACKET 	8

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
    uint8_t waittime;
} lora_queue_item_t;          // CELKEM: Jen 17 bajtů na záznam!
#pragma pack(pop)

// 2. Zkomprimovaný záznam pro STATUS (Nově přidáno!)
#pragma pack(push, 1)
typedef struct {
    uint8_t state_code;        // 1B (Např. 0 = OK, 1 = INIT, 2 = ERROR) místo textu
    uint8_t reason_code;       // 1B (Např. 0 = WAKEUP, 1 = REBOOT) místo textu
    uint32_t boot_count;       // 4B
    uint32_t wakeup_cycle_count;// 4B
    uint32_t first_boot_time;  // 4B
    int32_t adaptive_offset;   // 4B
    int16_t ntp_drift_ms;      // 2B (ntp_drift * 1000)
    int8_t rssi;               // 1B
    uint16_t batt_voltage_mv;  // 2B (battery_voltage * 1000)
    uint8_t batt_soc;          // 1B (0-100 %)
} lora_status_item_t;          // CELKEM: 28 bajtů (místo původních stovek!)
#pragma pack(pop)

// 3. Struktura samotného odesílaného LoRa paketu
#pragma pack(push, 1)
/*typedef struct {
	uint16_t network_id;      // 2 bajty: Zabezpečení sítě (např. 0xA1B2)
    uint8_t sender_id;        // 1 bajt: Kdo to posílá
    uint8_t packet_type;      // Např. MSG_TYPE_METER_DATA (1 byt)
    uint8_t item_count;       // Kolik záznamů paket reálně obsahuje (1 byt)
    lora_queue_item_t items[LORA_MAX_ITEMS_PER_PACKET]; // 4 * 17 = 68 bajtů  8*17+5=141
} lora_data_payload_t;        // CELÝ PAKET: 70?? bajtů (Ideální pro LoRa!)
*/
typedef struct {
    uint16_t network_id;      // 2 bajty
    uint8_t sender_id;        // 1 bajt
    uint8_t packet_type;      // 1 bajt (MSG_TYPE_...)
    uint8_t item_count;       // 1 bajt (Pro data 1-8, pro status vždy 1)
    
    // Union znamená, že v paměti bude buď 'items' NEBO 'status'. 
    // Šetří to paměť a sjednocuje odesílání!
    union {
        lora_queue_item_t items[LORA_MAX_ITEMS_PER_PACKET]; 
        lora_status_item_t status;                          
    } payload;
} lora_universal_packet_t;
#pragma pack(pop)

#endif // CUSTOM_COMM_H
