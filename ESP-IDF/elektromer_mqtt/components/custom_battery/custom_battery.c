#include "custom_battery.h"
#include "custom_config.h"
#include "esp_log.h"

#include "driver/i2c.h"

#define I2C_PORT        I2C_NUM_0
#define I2C_TIMEOUT_MS  100

static const char *TAG = "CUSTOM_BATTERY";

esp_err_t max17048_init(void);
esp_err_t max17048_get_voltage(float *voltage);
esp_err_t max17048_get_soc(float *soc);
void readVoltageandSOC(float *voltage, float *soc);

void battery_init(void) {
    ESP_LOGI(TAG, "Initializing battery monitor...");
    // TODO: Zde implementovat inicializaci I2C sběrnice
    // pro komunikaci s MAX17048.
    ESP_ERROR_CHECK(max17048_init());
//    ESP_LOGW(TAG, "Battery monitor I2C init is not yet implemented.");
}

esp_err_t battery_get_status(float *voltage, float *soc) {
    ESP_LOGI(TAG, "Reading battery status...");
    // TODO: Zde implementovat čtení dat z I2C,
    // čtení registrů z MAX17048 a přepočet na napětí a procenta.
    readVoltageandSOC(voltage, soc);
    // Fiktivní hodnoty pro testování
//    *voltage = 4.05;
//    *soc = 85.5;

//    ESP_LOGW(TAG, "Battery status reading is not yet implemented (using dummy data).");
    ESP_LOGI(TAG, "Battery status reading is completed.");
    return ESP_OK;
}

void readVoltageandSOC(float *voltage, float *soc) {
   // inicializace I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));

    ESP_ERROR_CHECK(max17048_init());
	vTaskDelay(pdMS_TO_TICKS(1000));          // dej ÄŤipu ÄŤas na aktualizaci
    if (max17048_get_voltage(voltage) == ESP_OK &&
        max17048_get_soc(soc) == ESP_OK) {
        ESP_LOGI("MAIN", "Battery: %.3f V, %.1f %%", *voltage, *soc);
        ESP_LOGI("MAIN1", "Battery: %f , %f", *voltage, *soc);
    }	
    
}

esp_err_t max17048_write_register(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = {reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    return i2c_master_write_to_device(I2C_PORT, MAX17048_DEFAULT_ADDR,
                                      buf, sizeof(buf),
                                      pdMS_TO_TICKS(I2C_TIMEOUT_MS));
}

esp_err_t max17048_read_register(uint8_t reg, uint16_t *value)
{
    uint8_t data[2];
    esp_err_t ret = i2c_master_write_read_device(
        I2C_PORT, MAX17048_DEFAULT_ADDR, &reg, 1,
        data, sizeof(data), pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    //ESP_LOGI(TAG, "MAX17048 sizeof(data): %i", sizeof(data));
    if (ret == ESP_OK) {
        *value = ((uint16_t)data[0] << 8) | data[1];
    }
    return ret;
}

esp_err_t max17048_init(void)
{
    // volitelné: QuickStart pro lepší přesnost po startu
    esp_err_t ret = ESP_OK;  //max17048_write_register(0x06, 0x4000);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "MAX17048 initialized (QuickStart)");
    } else {
        ESP_LOGW(TAG, "MAX17048 init failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t max17048_get_voltage(float *voltage)
{
    uint16_t raw;
    esp_err_t ret = max17048_read_register(0x02, &raw);
    if (ret == ESP_OK) {
        *voltage = (raw >> 4) * 1.25f / 1000.0f;  // převod na V
    }
    return ret;
}

esp_err_t max17048_get_soc(float *soc)
{
    uint16_t raw;
    esp_err_t ret = max17048_read_register(0x04, &raw);
    if (ret == ESP_OK) {
        *soc = 1.0f * raw / 256.0f;  // horní bajt = procenta, dolní bajt = 1/256 %
    }
    return ret;
}
