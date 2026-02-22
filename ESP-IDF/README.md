# Čtečka elektroměru XT211 přes RS485 do MQTT (ESP32)

Tento projekt implementuje energetický uzel postavený na **ESP32**, který v režimu "push" vyčítá data z elektroměru **XT211** a odesílá je na MQTT broker. Je navržen pro maximální efektivitu při provozu na baterie.

## Klíčové vlastnosti

* **Smart Power Management:** Využití Deep Sleep (10 min interval) pro minimální odběr.
* **DLMS/COSEM parsování:** Čtení standardizovaných OBIS kódů (1.8.0 až 1.8.4).
* **Batch Data Sending:** Data se ukládají do lokální fronty a odesílají se hromadně jednou za hodinu, aby se šetřila energie při startu Wi-Fi.
* **Časová synchronizace:** NTP synchronizace s korektním ošetřením časových pásem (SEČ/LSEČ) i po probuzení ze spánku.
* **Diagnostika baterie:** Integrace s čipem MAX17048 pro sledování stavu akumulátoru.

## Hardwarové složení

| Komponenta | Odkaz / Typ | Funkce |
| :--- | :--- | :--- |
| **MCU** | [LaskaKit ESP32-DEVKit](https://www.laskakit.cz/laskakit-esp32-devkit/) | Řízení a Wi-Fi konektivita |
| **Interface** | [Waveshare UART-RS485](https://botland.cz/prevadece-usb-uart-rs232-rs485/4496-prevodnik-uart-rs485-33-v-ark-rj11-waveshare-4777-5904422371463.html) | Fyzická vrstva pro XT211 |
| **Battery Gauge** | [LaskaKit MAX17048](https://www.laskakit.cz/laskakit-fuel-gauge-max17048-bateriovy-meric/) | Měření kapacity baterie |

## Logika programu (Workflow)

Program běží v cyklech, které rozlišují mezi "rychlým čtením" a "komunikačním oknem":

1. **Probuzení (každých 10 min):**
   * Inicializace UART pro RS485.
   * Čekaní na platný frame (DLMS push zpráva).
   * Označením datem/časem a uložení do vnitřní fronty.
2. **Komunikace (každou 1 hodinu):**
   * Aktivace Wi-Fi (výkon omezen na 10-11 dBm pro úspory).
   * Aktualizace času přes NTP.
   * Odeslání celé fronty na MQTT (duplicitní odesílání pro zajištění doručení).
3. **Údržba:**
   * Mazání dat z fronty starších než 24 hodin (FIFO).

## Konfigurace a instalace

### Softwarové požadavky
* **ESP-IDF Framework** (projekt je strukturován pro nativní vývojové prostředí).
* Knihovna pro MQTT (esp-mqtt).
* Knihovna pro I2C (pro MAX17048).

### Postup sestavení
```bash
# Klonování repozitáře
git clone [https://github.com/psopso/x211-meter.git](https://github.com/psopso/x211-meter.git)
cd x211-meter/ESP-IDF/elektromer_mqtt

# Konfigurace (Wi-Fi, MQTT broker, piny)
idf.py menuconfig

# Flashování do zařízení
idf.py build flash monitor
```

## Sledované OBIS kódy
V seznamu `OBIS_CODES` jsou definovány tyto registry:
* `1.8.0`: Celková činná energie
* `1.8.1` - `1.8.4`: Energie v jednotlivých tarifech (T1-T4)

---
*Tento projekt je open-source. Dokumentace byla vygenerována automaticky.*

