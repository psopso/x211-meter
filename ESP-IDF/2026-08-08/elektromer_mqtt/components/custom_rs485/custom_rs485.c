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
    if (!data || length < 2) { // Upraveno: buffer musí mít místo alespoň pro 2 byty
        return -1; // neplatný parametr nebo malý buffer
    }

    ESP_LOGI(TAG, "Cekam na hlavicku 0x0F 0x80.");

    uint8_t byte;
    int read_len = 0;
    int64_t start_time = esp_timer_get_time();
    
    bool waiting_for_second_byte = false; // Příznak, že jsme našli 0x0F a čekáme na 0x80

    // Čekání na sekvenci 0x0F 0x80
    while (true) {
        int n = uart_read_bytes(UART_PORT_NUM, &byte, 1, pdMS_TO_TICKS(100)); 
        
        if (n == 1) {
            if (waiting_for_second_byte) {
                // Našli jsme už 0x0F, teď testujeme, co přišlo potom
                if (byte == 0x80) {
                    // SKVĚLÉ: Máme kompletní hlavičku 0x0F 0x80
                    *frame_reception_time = time(NULL);
                    data[read_len++] = 0x0F; // Uložíme první byte
                    data[read_len++] = 0x80; // Uložíme druhý byte
                    break; // Vyskočíme z hledací smyčky a jdeme načítat zbytek
                } 
                else if (byte == 0x0F) {
                    // Přišlo 0x0F a hned potom zase 0x0F. 
                    // Považujeme toto nové 0x0F za potenciální začátek nové sekvence.
                    // Příklad: Data jsou "00 0F 0F 80". První 0F aktivovalo čekání, 
                    // druhé 0F ho udržuje, 80 ho potvrdí.
                    waiting_for_second_byte = true; 
                } 
                else {
                    // Přišlo 0x0F a potom něco jiného (např. 0x55). 
                    // Není to naše hlavička, resetujeme hledání.
                    waiting_for_second_byte = false;
                }
            } else {
                // Čekáme na první byte 0x0F
                if (byte == 0x0F) {
                    waiting_for_second_byte = true;
                }
            }
        }

        // Kontrola globálního timeoutu pro nalezení hlavičky
        if ((esp_timer_get_time() - start_time) / 1000 / 1000 > FRAME_WAIT_TIMEOUT_S) {
            return -2; // timeout čekání na start sekvenci
        }
    }

    // Čtení zbytku rámce (stejné jako v původním kódu)
    while (read_len < length) {
        int n = uart_read_bytes(UART_PORT_NUM,
                                &data[read_len],
                                (length - read_len), // Číst jen do konce bufferu
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
