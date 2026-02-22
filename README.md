[![en](https://img.shields.io/badge/lang-en-red.svg)](https://github.com/psopso/x211-meter/blob/main/README.en.md) [![cs](https://img.shields.io/badge/lang-cs-yellow.svg)](https://github.com/psopso/x211-meter/blob/main/README.md)

# XT211 Meter pro Home Assistant

Tato vlastní integrace pro Home Assistant umožňuje sledovat data z elektroměru **XT211** prostřednictvím protokolu **MQTT**.  Zprávy posílá modul, který není předmětem tohoto popisu, který je umístěn v elektroměru a čte data z rozhraní RS485 DLMS/COSEM, převádí je na mqtt zprávy a posílá do systému Home asistent.
Kromě vytváření senzorů přímo v Home Assistantu dokáže integrace automaticky přeposílat naměřená data do databáze **InfluxDB v2** pro dlouhodobou archivaci a analýzu. 

## Hlavní funkce

* **Senzory v reálném čase**: Automatické vytváření senzorů pro napětí, proud, výkon a frekvenci na jednotlivých fázích.
* **Podpora Energy Dashboardu**: Senzory spotřeby (OBIS kódy 1.8.x a 2.8.x) jsou plně kompatibilní s energetickým panelem Home Assistant (třídy `total_increasing`).
* **Integrace InfluxDB v2**: Přímé odesílání dat do InfluxDB pomocí Line Protocolu s přesným časem měření přímo z přístroje.
* **Diagnostické údaje**: Sledování stavu baterie, síly Wi-Fi signálu (RSSI), počtu probuzení a systémového statusu.
* **Snadná konfigurace**: Plná podpora pro nastavení i následné změny (Options Flow) přímo v uživatelském rozhraní.

## Instalace

1. Zkopírujte složku `xt211` do adresáře `custom_components` ve vaší instalaci Home Assistant.
2. Restartujte Home Assistant.
3. V menu **Nastavení -> Zařízení a služby** klikněte na **Přidat integraci** a vyhledejte "XT211 Meter".

## Konfigurace

Při nastavení zadáváte:

### MQTT Nastavení
* **Data Topic**: Téma pro naměřené hodnoty (výchozí: `elektromer/data`).
* **Status Topic**: Téma pro stavové zprávy (výchozí: `elektromer/status`).

### InfluxDB v2 Nastavení (volitelné)
* **Host & Port**: Adresa serveru (např. `192.168.1.50`, port `8086`).
* **Organization & Bucket**: Vaše údaje z InfluxDB administrace.
* **Token**: V2 API Token pro zápis.

## Podporované OBIS kódy

| Kód | Název senzoru | Jednotka |
| :--- | :--- | :--- |
| **1.8.0** | Total Import Energy | kWh |
| **2.8.0** | Total Export Energy | kWh |
| **16.7.0**| Active Power (Celkový) | W |
| **32.7.0**| Voltage L1 | V |
| **31.7.0**| Current L1 | A |

## Technické detaily

* **Verze**: 0.2.6
* **Závislosti**: Vyžaduje funkční MQTT broker.
* **Zpracování dat**: Data jsou do InfluxDB odesílána do measurementu `home_energy` s tagem `meter`.

### MQTT Datová struktura

Integrace zpracovává JSON zprávy publikované na následujících tématech:

#### 1. Naměřené hodnoty (Topic: elektromer/data)
Tato zpráva nese hlavní data z elektroměru. Integrace automaticky vytvoří senzory pro všechny OBIS kódy nalezené v sekci "values".

Příklad zprávy:

    {
      "datetime": "Sat Feb 21 20:27:33 2026",
      "data": {
        "reading_datetime": "Sat 2026-02-21 20:27:09 GMT",
    	"first_boot":false,    
        "values": {
          "1.8.0": 7369.066,
          "1.8.1": 1338.005,
          "1.8.2": 6031.06,
          "1.8.3": 0,
          "1.8.4": 0,
          "96.1.1": "5100025085"
        }
      }
    }

#### 2. Stav zařízení a baterie (Topic: elektromer/status)
Na tomto tématu integrace očekává diagnostické zprávy. Rozlišuje je podle hlavního klíče v JSONu:

A) Systémový status (klíč "Status") - příklad zprávy:

    {
      "datetime": "Sat Feb 21 20:27:35 2026",
      "Status": {
        "Status": "OK",
        "StatusText": "After wakeup",
        "Wakeups": 1072,
        "LastWait": 8,
        "FirstBootTime": "Tue 2026-02-10 12:07:07 GMT",
        "BuildDatetime":"2025-12-08 13:05:39",
        "Wifi": "-77"
      }
    }

B) Stav baterie (klíč "battery") - příklad zprávy:

    {
      "datetime": "Sat Feb 21 20:27:38 2026",
      "battery": {
        "Voltage": 3.783,
        "SOC": 35.78
      }
    }

---
**Autor**: [@psopso](https://github.com/psopso)



