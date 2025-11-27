#include "my_debug.h"
#include "sdkconfig.h" // Pro Kconfig (viz bonus)
#include <stdio.h>    // Pro snprintf
#include <stdarg.h>   // Pro va_list, ...
#include <stdint.h>

// Statická globální proměnná (viditelná jen v tomto souboru),
// která drží ukazatel na odesílací funkci.
static my_debug_sender_t g_debug_sender = NULL;

char ss[300];

static void bytes_to_hex(char *out, size_t out_size,
                         const uint8_t *data, size_t len);
                         
void my_debug_register_sender(my_debug_sender_t sender_fn)
{
    g_debug_sender = sender_fn;
}

// Funkce, kterou jste chtěl (přijímá dva parametry)
void my_debug_log(const char *tag, const char *message)
{
    // Pokud není nic zaregistrováno (nebo je debug vypnutý), nedělej nic.
    #if CONFIG_MY_DEBUG_ENABLED // Viz krok 7 (Bonus)
    if (g_debug_sender != NULL) {
        
        // Zde můžete zformátovat finální MQTT téma a zprávu
        // Např. chceme téma "device/123/debug/MAIN"
        char debug_topic[128];
        snprintf(debug_topic, sizeof(debug_topic), "device/debug/%s", tag);

        // Zavoláme zaregistrovanou funkci (což je ve skutečnosti vaše MQTT funkce)
        g_debug_sender(debug_topic, message);
    }
    #else
    // Pokud je v menuconfigu vypnuto, neděláme nic.
    // Tímto kompilátor optimalizuje volání na prázdnou operaci.
    (void)tag; // Potlačení varování o nevyužitých proměnných
    (void)message;
    #endif
}

void my_debug_log_hex(const char *tag, const uint8_t *message)
{
    // Pokud není nic zaregistrováno (nebo je debug vypnutý), nedělej nic.
    #if CONFIG_MY_DEBUG_ENABLED // Viz krok 7 (Bonus)
    if (g_debug_sender != NULL) {
        
        // Zde můžete zformátovat finální MQTT téma a zprávu
        // Např. chceme téma "device/123/debug/MAIN"
        char debug_topic[128];
        bytes_to_hex(ss, 300, message, 36);
        
        snprintf(debug_topic, sizeof(debug_topic), "device/debug/%s", tag);

        // Zavoláme zaregistrovanou funkci (což je ve skutečnosti vaše MQTT funkce)
        g_debug_sender(debug_topic, ss);
    }
    #else
    // Pokud je v menuconfigu vypnuto, neděláme nic.
    // Tímto kompilátor optimalizuje volání na prázdnou operaci.
    (void)tag; // Potlačení varování o nevyužitých proměnných
    (void)message;
    #endif
}

// Bonus: Funkce jako printf
void my_debug_log_formatted(const char *tag, const char *format, ...)
{
    #if CONFIG_MY_DEBUG_ENABLED
    if (g_debug_sender != NULL) {
        char message_buffer[256]; // Buffer pro naformátovanou zprávu
        va_list args;
        va_start(args, format);
        vsnprintf(message_buffer, sizeof(message_buffer), format, args);
        va_end(args);

        // Nyní zavoláme naši hlavní logovací funkci
        my_debug_log(tag, message_buffer);
    }
    #else
    (void)tag;
    (void)format;
    #endif
}

/// pomocna funkce: prevod pole na hex string
static void bytes_to_hex(char *out, size_t out_size,
                         const uint8_t *data, size_t len)
{
    size_t pos = 0;
    for (size_t i = 0; i < len && pos + 2 < out_size; i++) {
        pos += snprintf(out + pos, out_size - pos, "%02x", data[i]);
    }
}
