import logging
import aiohttp
# Přidán import timezone
from datetime import datetime, timezone 

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

_LAST_STATUS_VALUES = {}

def _parse_and_convert_time(time_str):
    """
    Parsuje časový řetězec z měřiče a vrací Unix timestamp v nanosekundách.
    Zajišťuje správnou interpretaci GMT/UTC.
    """
    if not time_str:
        return None


    try:
        # 1. Naparsování řetězce podle formátu
        dt_obj = datetime.strptime(time_str, KNOWN_TIME_FORMAT)
        
        # 2. KLÍČOVÝ KROK: Vynucení UTC (GMT = UTC)
        # Protože strptime často vrací objekt bez zóny, .replace mu ji vnutí.
        # Tím zajistíme, že .timestamp() nebude brát čas jako lokální čas serveru.
        dt_obj = dt_obj.replace(tzinfo=timezone.utc)
        
        # 3. Převod na Unix timestamp (sekundy od epochy)
        # Teď už .timestamp() ví, že dt_obj je v UTC, a neprovede žádný posun.
        timestamp_s = dt_obj.timestamp()
        
        # Převod na nanosekundy
        timestamp_ns = int(timestamp_s * 1000000000)
        return timestamp_ns
        
    except ValueError as e:
        _LOGGER.warning(f"Nepodařilo se parsovat čas '{time_str}'. Chyba: {e}")
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
            # ---> ZMĚNA: Přidán 'return' <---
            return await _send_to_influx(hass, config, "\n".join(lines))

        return None # Nebylo co odeslat
        
    except Exception as e:
        _LOGGER.error(f"Kritická chyba při zpracování dat pro InfluxDB: {e}")

# ----------------------------------------------------------------------

async def handle_status(hass, payload_str, config):
    """Zpracování statusu a odeslání pouze ZMĚN vybraných hodnot do InfluxDB."""
    host = config.get(CONF_INFLUXDB_HOST)
    if not host:
        return

    import json
    try:
        data = json.loads(payload_str)
    except:
        _LOGGER.warning("Nebylo možné naparsovat JSON statusu.")
        return

    lines = []
    tags = "device=XT211_Status"
    fields = []

    # Zpracování baterie (připraveno pro případné budoucí použití)
    if "battery" in data:
        for k, v in data["battery"].items():
             if isinstance(v, (int, float)):
                field_key = f"battery_{k.lower()}"
                # 👇 Zkontrolujeme, zda se hodnota změnila
                if _LAST_STATUS_VALUES.get(field_key) != float(v):
                    fields.append(f"{field_key}={float(v)}")
                    _LAST_STATUS_VALUES[field_key] = float(v) # Uložíme si novou hodnotu

    # Zpracování obecného statusu
    if "Status" in data:
        # Váš whitelist
        allowed_numeric_keys = ["lastwaitmin"] 
        
        for k, v in data["Status"].items():
            key_lower = k.lower()
            
            if isinstance(v, (int, float)):
                if key_lower in allowed_numeric_keys:
                    field_key = f"status_{key_lower}"
                    # 👇 Zkontrolujeme, zda se číselná hodnota změnila
                    if _LAST_STATUS_VALUES.get(field_key) != float(v):
                        fields.append(f"{field_key}={float(v)}")
                        _LAST_STATUS_VALUES[field_key] = float(v)
                    
            elif isinstance(v, str):
                 if key_lower in ["status", "statustext"]:
                    field_key = f"status_{key_lower}"
                    # 👇 Zkontrolujeme, zda se textová hodnota změnila
                    if _LAST_STATUS_VALUES.get(field_key) != v:
                        fields.append(f'{field_key}="{v}"')
                        _LAST_STATUS_VALUES[field_key] = v

    # Odesíláme do InfluxDB POUZE tehdy, pokud se do 'fields' něco přidalo (tj. nastala změna)
    if fields:
        lines.append(f"xt211_status,{tags} {','.join(fields)}")
        await _send_to_influx(hass, config, "\n".join(lines))

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
         return  False # ---> ZMĚNA: Vracíme False

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
                return False # ---> ZMĚNA: Zápis selhal
            else:
                _LOGGER.debug(f"Data úspěšně odeslána do InfluxDB V2 do bucketu: {bucket}")
                return True # ---> ZMĚNA: Zápis byl úspěšný
                
    except aiohttp.ClientError as e:
        _LOGGER.error(f"Nepodařilo se připojit k InfluxDB V2 na adrese http://{host}:{port}: {e}")
        return False # ---> ZMĚNA: Připojení selhalo
