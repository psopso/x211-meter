DOMAIN = "xt211"

# --- Konfigurační klíče ---
CONF_TOPIC_DATA = "mqtt_topic_data"
CONF_TOPIC_STATUS = "mqtt_topic_status"

CONF_INFLUXDB_HOST = "influxdb_host"
CONF_INFLUXDB_PORT = "influxdb_port"
CONF_INFLUXDB_BUCKET = "influxdb_bucket"
CONF_INFLUXDB_ORG = "influxdb_org"
CONF_INFLUXDB_TOKEN = "influxdb_token"

# --- Výchozí hodnoty MQTT topiců ---
DEFAULT_TOPIC_DATA = "elektromer/data"
DEFAULT_TOPIC_STATUS = "elektromer/status"

# Mapování OBIS kódů na srozumitelné názvy a jednotky
SENSOR_MAP = {
    # --- Energie (kWh) ---
    "1.8.0": {"name": "Total Import Energy", "unit": "kWh"},
    "1.8.1": {"name": "Import Energy Tariff 1", "unit": "kWh"},
    "1.8.2": {"name": "Import Energy Tariff 2", "unit": "kWh"},
    "1.8.3": {"name": "Import Energy Tariff 3", "unit": "kWh"},
    "1.8.4": {"name": "Import Energy Tariff 4", "unit": "kWh"},
    "2.8.0": {"name": "Total Export Energy", "unit": "kWh"},
    "2.8.1": {"name": "Export Energy Tariff 1", "unit": "kWh"},
    "2.8.2": {"name": "Export Energy Tariff 2", "unit": "kWh"},
    "2.8.3": {"name": "Export Energy Tariff 3", "unit": "kWh"},
    "2.8.4": {"name": "Export Energy Tariff 4", "unit": "kWh"},

    # --- Okamžité hodnoty ---
    "16.7.0": {"name": "Active Power", "unit": "W"},
    "36.7.0": {"name": "Active Power L1", "unit": "W"},
    "56.7.0": {"name": "Active Power L2", "unit": "W"},
    "76.7.0": {"name": "Active Power L3", "unit": "W"},

    "32.7.0": {"name": "Voltage L1", "unit": "V"},
    "52.7.0": {"name": "Voltage L2", "unit": "V"},
    "72.7.0": {"name": "Voltage L3", "unit": "V"},

    "31.7.0": {"name": "Current L1", "unit": "A"},
    "51.7.0": {"name": "Current L2", "unit": "A"},
    "71.7.0": {"name": "Current L3", "unit": "A"},

    "14.7.0": {"name": "Frequency", "unit": "Hz"},
    "13.7.0": {"name": "Power Factor", "unit": None},
}