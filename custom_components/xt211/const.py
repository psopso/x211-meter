DOMAIN = "xt211"
MQTT_TOPIC_DATA = "elektromer/xt211/data"
MQTT_TOPIC_STATUS = "elektromer/xt211/status"

SENSOR_MAP = {
    "1.8.0": {"name": "Total Energy", "unit": "kWh"},
    "1.8.1": {"name": "Tariff 1 Energy", "unit": "kWh"},
    "1.8.2": {"name": "Tariff 2 Energy", "unit": "kWh"},
    "1.8.3": {"name": "Tariff 3 Energy", "unit": "kWh"},
    "1.8.4": {"name": "Tariff 4 Energy", "unit": "kWh"},
    "2.8.0": {"name": "Total Export Energy", "unit": "kWh"},
    "16.7.0": {"name": "Active Power", "unit": "W"},
}
