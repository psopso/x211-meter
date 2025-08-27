#pragma once
#include <stdint.h> // Potřebné pro typ uint64_t

/**
 * @brief Uvede zařízení do hlubokého spánku na zadaný počet sekund.
 * @param sleepSeconds Doba spánku v sekundách.
 */
void goToSleep(uint64_t sleepSeconds);
