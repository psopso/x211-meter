# XT211 Meter pro Home Assistant

Tato vlastní integrace pro Home Assistant umožňuje sledovat data z elektroměru **XT211** prostřednictvím protokolu **MQTT**. Kromě vytváření senzorů přímo v Home Assistantu dokáže integrace automaticky přeposílat naměřená data do databáze **InfluxDB v2** pro dlouhodobou archivaci a analýzu. Je třeba řešit u druhou stranu na straně elektroměru, která načte data z RS485 s DLMS/COSEM a pošle je pomocí mqtt zprávy do home asistenta.

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

---
**Autor**: [@psopso](https://github.com/psopso)
