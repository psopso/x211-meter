import logging
import aiohttp
from datetime import datetime
from homeassistant.helpers.aiohttp_client import async_get_clientsession

# Předpokládáme, že tento externí modul je součástí závislostí integrace
# nebo se spoléhá na standardní funkce Pythons.
# Pokud používáte externí modul jako 'dateutil', musíte ho přidat do manifest.json.
# Pro jednoduchost se spoléháme na standardní funkce a známý formát.
# Váš funkční formát by mohl být: "%a %Y-%m-%d %H:%M:%S %Z"
# Mon 2025-11-24 07:23:38 GMT
KNOWN_TIME_FORMAT = "%a %Y-%m-%d %H:%M:%S %Z"

from .const import (
    CONF_INFLUXDB_HOST,
    CONF_INFLUXDB_PORT,
    CONF_INFLUXDB_BUCKET,
    CONF_INFLUXDB_ORG,
    CONF_INFLUXDB_TOKEN,
)

_LOGGER = logging.getLogger(__name__)


def _parse_and_convert_time(time_str):
    """
    Parsuje časový řetězec z měřiče a vrací Unix timestamp v nanosekundách.
    
    POZNÁMKA: Nahraďte parsování uvnitř bloku 'try' vaší ověřenou logikou,
    která zvládá lokální nastavení Home Assistanta.
    """
    if not time_str:
        return None
    try:
        # Příklad implementace (pro standardní formát a odstranění denní zkratky,
        # pokud to dělá problémy s locale, ale vy byste měli vložit vaši FUNKČNÍ LOGIKU):
        # Pokud je váš řetězec "Fri 2025-09-19 08:44:03 GMT", a to parsování funguje:
        dt_obj = datetime.strptime(time_str, KNOWN_TIME_FORMAT)
        
        # Převod na Unix timestamp (sekundy)
        timestamp_s = int(dt_obj.timestamp())
        
        # Převod na nanosekundy (pro maximální přesnost v InfluxDB)
        timestamp_ns = timestamp_s * 1000000000
        return timestamp_ns
    except ValueError as e:
        _LOGGER.warning(f"Nepodařilo se parsovat čas '{time_str}'. Zkontrolujte KNOWN_TIME_FORMAT. Chyba: {e}")
        return None


# ----------------------------------------------------------------------

async def handle_data(hass, payload, config):
    """Zpracování dat z MQTT a odeslání do InfluxDB s přesným časem měření."""
    
    host = config.get(CONF_INFLUXDB_HOST)
    if not host:
        return # InfluxDB není nastaven
    
    try:
        data_values = payload.get("data", {}).get("values", {})
        reading_time_str = payload.get("data", {}).get("reading_datetime")
        
        # Získání timestampu v nanosekundách (včetně parsování)
        timestamp_ns = _parse_and_convert_time(reading_time_str)

        if not data_values or not timestamp_ns:
            return

        lines = []
        # 'device=XT211' je tag (indexované)
        tags = "" 
        sn=""
        
        fields = []
        for key, value in data_values.items():
            if isinstance(value, (int, float)):
                # Všechna číselná data ukládáme jako float pro InfluxDB
                clean_key = key.replace('.', '_')
                fields.append(f"{clean_key}={float(value)}")
                #fields.append(f'"{key}"={float(value)}')
            if isinstance(value, (str)):
                # string by mělo být sériové číslo
                #clean_key = key.replace('.', '_')
                sn = value

        # 'device=XT211' je tag (indexované)
        tags = "meter="+sn
        
        if fields:
            fields_str = ",".join(fields)
            
            # Formát: measurement,tagy field1=hodnota,field2=hodnota TIMESTAMP
            line = f"home_energy,{tags} {fields_str} {timestamp_ns}"
            lines.append(line)

        if lines:
            await _send_to_influx(hass, config, "\n".join(lines))

    except Exception as e:
        _LOGGER.error(f"Kritická chyba při zpracování dat pro InfluxDB: {e}")


# ----------------------------------------------------------------------

async def handle_status(hass, payload_str, config):
    return  #Status se neposílá do influxdb2
    """Zpracování statusu (baterie, atd.) a odeslání do InfluxDB."""
    host = config.get(CONF_INFLUXDB_HOST)
    if not host:
        return

    import json
    try:
        data = json.loads(payload_str)
    except:
        _LOGGER.warning("Nebylo možné napařsovat JSON statusu.")
        return

    lines = []
    tags = ""

    # Statusy posíláme bez timestampu, InfluxDB použije čas přijetí (NOW)
    # Tím zajistíme, že status bude mít čas co nejblíže realitě.

    # Zpracování baterie
    '''    
    if "battery" in data:
        fields = []
        for k, v in data["battery"].items():
             if isinstance(v, (int, float)):
                fields.append(f"battery_{k.lower()}={float(v)}")
        
        if fields:
            lines.append(f"xt211_status,{tags} {','.join(fields)}")
    '''
    # Zpracování obecného statusu
    if "Status" in data:
        fields = []
        for k, v in data["Status"].items():
            if isinstance(v, (int, float)):
                if 1==1:
                    fields.append(f"status_{k.lower()}={float(v)}")
            elif isinstance(v, str):
                 # Stringy musí být v uvozovkách (tzv. "quoted string literal")
                 if (k.lower() == "status") or (k.lower() == "statustext"):
                    fields.append(f'status_{k.lower()}="{v}"')
        
        if fields:
            lines.append(f"xt211_status,{tags} {','.join(fields)}")

    if lines:
        await _send_to_influx(hass, config, "\n".join(lines))


# ----------------------------------------------------------------------

async def _send_to_influx(hass, config, data_payload):
    """Pomocná funkce pro asynchronní HTTP POST do InfluxDB V2 API."""
    
    host = config.get(CONF_INFLUXDB_HOST)
    port = config.get(CONF_INFLUXDB_PORT, "8086")
    
    # --- NOVÉ PARAMETRY PRO V2 ---
    bucket = config.get(CONF_INFLUXDB_BUCKET)
    org = config.get(CONF_INFLUXDB_ORG)
    token = config.get(CONF_INFLUXDB_TOKEN)
    # -----------------------------

    # Kontrola, zda máme token a základní V2 data
    if not token or not bucket or not org:
         _LOGGER.error("Chybí token, bucket nebo organizace pro InfluxDB V2.")
         return

    # V2 API URL
    url = f"http://{host}:{port}/api/v2/write"
    
    # Parametry dotazu pro V2
    params = {
        "bucket": bucket,
        "org": org,
        "precision": "ns" # Používáme nanosekundy
    }
    
    # Autorizační hlavička
    headers = {
        "Authorization": f"Token {token}",
        "Content-Type": "text/plain; charset=utf-8"
    }

    session = async_get_clientsession(hass)
    
    try:
        async with session.post(url, params=params, data=data_payload, headers=headers) as response:
            if response.status not in (204, 200): # 204 je success, ale 200 může být vrácena s chybou
                text = await response.text()
                _LOGGER.error(f"Chyba zápisu do InfluxDB V2 ({response.status}) - Odpověď: {text}")
            else:
                _LOGGER.debug(f"Data úspěšně odeslána do InfluxDB V2 do bucketu: {bucket}")
                
    except aiohttp.ClientError as e:
        _LOGGER.error(f"Nepodařilo se připojit k InfluxDB V2 na adrese http://{host}:{port}: {e}")
        