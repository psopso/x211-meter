/* dlms_parser.h */
#pragma once
#include "config.h"
#include <time.h>

/**
 * @brief Čte data z RS485 a pokouší se je parsovat.
 * @param outData Struktura pro uložení výsledků.
 * @param timeoutMs Maximální doba čekání na data v milisekundách.
 * @param frameTimestamp Výstupní parametr, kam se uloží čas příchodu prvního bytu rámce.
 * @return true při úspěšném přečtení a parsování, jinak false.
 */

bool readAndParseFrame(MeterData& outData, unsigned long timeoutMs, time_t& frameTimestamp);
