/*
esp_err_t led_strip_set_pixel 	(led_strip_handle_t strip,
		uint32_t 	index,
		uint32_t 	red,
		uint32_t 	green,
		uint32_t 	blue )
*/

#include "esp_log.h"
#include "led_strip.h"
#include "custom_led.h"

static const char *TAG = "CUSTOM_LED";
led_strip_handle_t configure_led(void);

void switchLED(bool led_on_off,  int red, int green, int blue) {

    led_strip_handle_t led_strip = configure_led();
    if (led_on_off) {
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, red, green, blue));
    
    /* Refresh the strip to send data */
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    ESP_LOGI(TAG, "LED ON!");
    } else {
            /* Set all LED off to clear all pixels */
            ESP_ERROR_CHECK(led_strip_clear(led_strip));
            ESP_LOGI(TAG, "LED OFF!");
    }
}

//https://components.espressif.com/components/espressif/led_strip/versions/3.0.1~1/readme

#define LED_STRIP_USE_DMA  0
#define LED_STRIP_LED_COUNT 1
#define LED_STRIP_MEMORY_BLOCK_WORDS 0 // let the driver choose a proper memory block size automatically  
#define LED_STRIP_GPIO_PIN 4

// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA,     // Using DMA can improve performance when driving more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

