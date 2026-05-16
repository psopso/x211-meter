#ifndef MY_DEBUG_H
#define MY_DEBUG_H

#include <stddef.h> // Pro size_t
#include <stdint.h>


/**
 * @brief Definice typu pro funkci, která umí odeslat data.
 * * Toto je ukazatel na funkci. Říkáme "kdokoli nám dá funkci,
 * která přijímá dva textové řetězce (např. téma a zprávu),
 * budeme ji schopni použít."
 */
typedef void (*my_debug_sender_t)(const char *topic, const char *payload);

/**
 * @brief Registruje funkci, která se má použít pro odeslání logu.
 * * Tuto funkci zavoláte jednou při startu aplikace (např. v main.c)
 * a předáte jí vaši reálnou funkci pro odeslání MQTT.
 *
 * @param sender_fn Ukazatel na vaši MQTT odesílací funkci.
 */
void my_debug_register_sender(my_debug_sender_t sender_fn);

/**
 * @brief Hlavní logovací funkce, kterou budete volat odkudkoli.
 * * @param tag "Značka" modulu (např. "MAIN", "MQTT", "WIFI")
 * @param message Zpráva k odeslání
 */
void my_debug_log(const char *tag, const char *message);

/**
 * @brief Logovací funkce s formátováním (jako printf).
 */
void my_debug_log_formatted(const char *tag, const char *format, ...);
void my_debug_log_hex(const char *tag, const uint8_t *message);

#endif // MY_DEBUG_H