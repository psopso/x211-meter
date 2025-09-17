#include "custom_rs485.h"
#include "custom_config.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"

static const char *TAG = "CUSTOM_RS485";
// Fronta pro UART události
static QueueHandle_t uart_queue;

void rs485_init(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Instalace driveru S EVENT очередью
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUFFER_SIZE * 2, UART_BUFFER_SIZE * 2, 20, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, RS485_TXD_PIN, RS485_RXD_PIN, RS485_RTS_PIN, UART_PIN_NO_CHANGE));

    // Nastavení RX timeoutu pro detekci konce paketu
    // Hodnota je v "byte timech", tj. času potřebném na přenos jednoho bytu.
    // Pro 9600 baud/s je 1 byte time cca 1ms. 20ms je tedy cca 20 byte time.
    uint32_t timeout_ticks = (UART_RX_TIMEOUT_MS * UART_BAUD_RATE) / 1000 / 8;
    if (timeout_ticks == 0) timeout_ticks = 1; // Minimální hodnota musí být 1
//    ESP_ERROR_CHECK(uart_set_rx_timeout(UART_PORT_NUM, timeout_ticks));

    ESP_LOGI(TAG, "RS485 UART initialized on port %d with event queue and RX timeout.", UART_PORT_NUM);
}

int rs485_read_frame(uint8_t *data, uint32_t length, time_t *frame_reception_time)
{
    if (!data || length == 0) {
        return -1; // neplatný parametr
    }

    // Čekat na platný datový rámec
    ESP_LOGI(TAG, "Cekam na paket.");

    uint8_t byte;
    int read_len = 0;
    int64_t start_time = esp_timer_get_time();

    // Čekání na první byte 0x0F
    while (true) {
        int n = uart_read_bytes(UART_PORT_NUM, &byte, 1, pdMS_TO_TICKS(100)); // 100ms polling
        
        if (n == 1) {
            if (byte == 0x0F) {
				*frame_reception_time = time(NULL);
                data[read_len++] = byte;
                break;
            }
        }
        if ((esp_timer_get_time() - start_time) / 1000 / 1000 > FRAME_WAIT_TIMEOUT_S) {
            return -2; // timeout čekání na první byte
        }
    }

    // Čtení zbytku rámce, dokud přichází data
    while (read_len < length) {
        int n = uart_read_bytes(UART_PORT_NUM,
                                &data[read_len],
                                50,
                                pdMS_TO_TICKS(FRAME_END_TIMEOUT_MS));
        if (n > 0) {
            read_len += n;
        } else {
            // timeout mezery -> konec rámce
            break;
        }
    }

    return read_len; // počet přečtených bajtů
}
