#pragma once
#include <stdint.h>
#include <time.h>

/**
 * @brief Inicializuje UART port pro komunikaci RS485.
 */
void rs485_init(void);

/**
 * @brief Čte jeden datový rámec z RS485.
 * * @param data Buffer pro uložení přijatých dat.
 * @param length Velikost bufferu.
 * @return Počet přijatých bajtů, nebo 0 v případě timeoutu, nebo -1 v případě chyby.
 */
int rs485_read_frame(uint8_t *data, uint32_t length, time_t *frame_reception_time);