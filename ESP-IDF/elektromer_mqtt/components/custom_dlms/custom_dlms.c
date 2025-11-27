#include "custom_dlms.h"
#include "esp_log.h"
#include "dlms_parser.h"
#include "my_debug.h"
#include <stdio.h>

static const char *TAG = "DLMS_PARSER";
char s[77];
   	
void storeOBISToData(dlms_data_t *data, char *obis, char *svalue){
	const char obis0[] = "1.8.0";
	const char obis1[] = "1.8.1";
	const char obis2[] = "1.8.2";
	const char obis3[] = "1.8.3";
	const char obis4[] = "1.8.4";

    char *endptr;
    double value = strtod(svalue, &endptr);
    if (endptr == svalue)
      value = 0;
	value = value / 1000;
	if (strcmp(obis, obis0) == 0) {  //string are the same
		data->obis_1_8_0 = value;		
	}
	if (strcmp(obis, obis1) == 0) {  //string are the same
		data->obis_1_8_1 = value;				
	}
	if (strcmp(obis, obis2) == 0) {  //string are the same
		data->obis_1_8_2 = value;		
	}
	if (strcmp(obis, obis3) == 0) {  //string are the same
		data->obis_1_8_3 = value;		
	}
	if (strcmp(obis, obis4) == 0) {  //string are the same
		data->obis_1_8_4 = value;				
	}	
}

esp_err_t dlms_parse_frame(const uint8_t *buffer, int length, dlms_data_t *data) {
    ESP_LOGI(TAG, "Parsing DLMS frame (length: %d)...", length);
    
    // TODO: Zde je potřeba implementovat vaši logiku pro parsování DLMS/COSEM rámce.
    // Budete muset procházet `buffer` a hledat sekvence odpovídající OBIS kódům
    // a následně extrahovat hodnoty (např. 4-bajtové integer hodnoty) a přepočítat je
    // na reálné hodnoty (kWh) podle specifikace elektroměru.
    
    // Pro ukázku a testování jsou zde vloženy fiktivní hodnoty:
	data->obis_1_8_0 = 0.0;
    data->obis_1_8_1 = 0.0;
    data->obis_1_8_2 = 0.0;
    data->obis_1_8_3 = 0.0;
    data->obis_1_8_4 = 0.0;

/*
	typedef struct {
    	char obis[MAX_OBIS_LEN];
    	char value[MAX_VALUE_LEN];
	} dlms_item_t;

	typedef struct {
    	dlms_item_t items[MAX_DLMS_ITEMS];
    	size_t count;
	} dlms_result_t;
*/

    my_debug_log_hex("RS485", buffer);
    
	dlms_result_t dlmsresult = parse_dlms(buffer, length);
	snprintf(s, 77, "Delka: %i/%i", length, dlmsresult.count);
	my_debug_log("DLMS", s);
	
	for (size_t i = 0; i < dlmsresult.count; i++) {
    	dlms_item_t *item = &dlmsresult.items[i];  // ukazatel na i-tou položku
    	printf("OBIS: %s => %s\n", item->obis, item->value);
    	snprintf(s, 77, "%s => %s", item->obis, item->value);
    	my_debug_log("DLMS", s);
    	storeOBISToData(data, item->obis, item->value);
	}

    ESP_LOGI(TAG, "Parsing complete.");

    // Pokud parsování proběhne v pořádku, vraťte ESP_OK
    return ESP_OK;
};
