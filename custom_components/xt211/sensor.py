import json
import logging
from homeassistant.components import mqtt
from homeassistant.components.sensor import SensorEntity
from homeassistant.helpers.entity import DeviceInfo
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddEntitiesCallback

from .const import (
    DOMAIN,
    SENSOR_MAP,
    CONF_TOPIC_DATA,
    CONF_TOPIC_STATUS,
    DEFAULT_TOPIC_DATA,
    DEFAULT_TOPIC_STATUS,
)

from .mqtt_handler import (
        handle_status,
        handle_data
)

_LOGGER = logging.getLogger(__name__)


async def async_setup_entry(
    hass: HomeAssistant, entry: ConfigEntry, async_add_entities: AddEntitiesCallback
):
    """Set up the XT211 sensor platform."""
    
    topic_data = entry.data.get(CONF_TOPIC_DATA, DEFAULT_TOPIC_DATA)
    topic_status = entry.data.get(CONF_TOPIC_STATUS, DEFAULT_TOPIC_STATUS)
    entry_id = entry.entry_id # <--- ZÍSKÁNÍ UNIKÁTNÍHO ID ZÁZNAMU
    
    _LOGGER.info(f"Setting up XT211 with Data Topic: {topic_data} and Status Topic: {topic_status}")

    device_info = DeviceInfo(
        identifiers={(DOMAIN, entry_id)}, # Použijeme entry_id pro unikátnost zařízení
        name="XT211 Meter",
        manufacturer="PSO",
        model="XT211",
    )
    
    # <--- ZMĚNA: Předáváme entry_id do konstruktorů senzorů
    status_sensor = Xt211StatusSensor(device_info, entry_id)
    raw_data_sensor = Xt211RawDataSensor(device_info, entry_id)
    datetime_sensor = Xt211DatetimeSensor(device_info, entry_id)
    battery_sensor = Xt211BatterySensor(device_info, entry_id)
    wakeups_sensor = Xt211WakeupsSensor(device_info, entry_id)
    rssi_sensor = Xt211RSSISensor(device_info, entry_id)

    entities = [status_sensor, raw_data_sensor, datetime_sensor, battery_sensor, wakeups_sensor, rssi_sensor]
    async_add_entities(entities)

    # Uložíme si entity do hass.data, abychom k nim měli přístup později
    if DOMAIN not in hass.data:
        hass.data[DOMAIN] = {}
    hass.data[DOMAIN][entry.entry_id] = entities

#Example
#{"datetime":"Fri 2025-09-19 08:44:03 GMT","first_boot":false,"obis_1_8_0":443.451,"obis_1_8_1":83.769,"obis_1_8_2":359.682,"obis_1_8_3":0,"obis_1_8_4":0}


    async def message_received_data(msg):
        """Handle new MQTT messages for data."""
        try:
            payload = json.loads(msg.payload)
        except Exception as e:
            _LOGGER.error("Invalid JSON received on data topic: %s", e)
            return

        raw_data_sensor.set_raw(payload)

        if "datetime" in payload:
            datetime_sensor.set_value(payload["datetime"])

        new_entities = []
        # Získáme seznam již existujících OBIS senzorů z hass.data
        current_obis_sensors = {
            e.obis: e for e in hass.data[DOMAIN].get(entry.entry_id, []) if isinstance(e, Xt211ObisSensor)
        }

        for obis, value in payload["data"].get("values", {}).items():
            sensor = current_obis_sensors.get(obis)

            if sensor is None:
                _LOGGER.debug(f"Creating new sensor for OBIS code: {obis}")
                # <--- ZMĚNA: Předáváme entry_id i sem
                sensor = Xt211ObisSensor(device_info, entry_id, obis)
                entities.append(sensor)
                new_entities.append(sensor)
            
            sensor.set_value(value)

        if new_entities:
            async_add_entities(new_entities)
            # Aktualizujeme seznam entit v hass.data
            hass.data[DOMAIN][entry.entry_id] = entities

        await handle_data(payload)

# Examples
# {"battery":{"Voltage":4.15749979019165,"SOC":96.234375}}
#  {"Status":{"Status":"OK","StatusText":"After wakeup","Resets":1,"Wakeups":1015}}

    async def message_received_status(msg):
        """Handle new MQTT messages for status."""
        
        if msg.payload != "":
            json_data = json.loads(msg.payload)
            if json_data.get("battery") != None:
                if (json_data["battery"].get("Voltage") != None) and (json_data["battery"].get("SOC") != None):
                    str = f"{round(json_data["battery"]["Voltage"],2)}V, {round(json_data["battery"]["SOC"],2)}%"
                    battery_sensor.set_value(str)
                    battery_sensor._attr_extra_state_attributes = {"last_message": json_data}
                else:
                    battery_sensor.set_value("Failed")
            if json_data.get("Status") != None:
                if json_data["Status"].get("Wakeups") != None:
                    wakeups_sensor.set_value(json_data["Status"]["Wakeups"])
                if json_data["Status"].get("RSSI") != None:
                    rssi_sensor_sensor.set_value(json_data["Status"]["Wifi"])
                if json_data["Status"].get("Status") != None:
                    status_sensor.set_value(json_data["Status"]["Status"]+", "+json_data["Status"]["StatusText"])
                status_sensor._attr_extra_state_attributes = {"last_message": json_data}
        
        await handle_status(msg.payload)
    
    await mqtt.async_subscribe(hass, topic_data, message_received_data)
    await mqtt.async_subscribe(hass, topic_status, message_received_status)


# --- ENTITY CLASSES ---

class Xt211StatusSensor(SensorEntity):
    def __init__(self, device_info, entry_id): # <--- ZMĚNA: Přidán argument entry_id
        self._attr_name = "XT211 Status"
        self._attr_unique_id = f"{entry_id}_status" # <--- ZMĚNA: Opraveno vytváření unique_id
        self._attr_device_info = device_info
        self._state = None
        self._attr_extra_state_attributes = {}

    def set_value(self, val):
        self._state = val
        if self.hass:
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state


class Xt211RawDataSensor(SensorEntity):
    def __init__(self, device_info, entry_id): # <--- ZMĚNA: Přidán argument entry_id
        self._attr_name = "XT211 Raw Data"
        self._attr_unique_id = f"{entry_id}_raw" # <--- ZMĚNA: Opraveno vytváření unique_id
        self._attr_device_info = device_info
        self._state = "OK"
        self._attr_extra_state_attributes = {}

    def set_raw(self, payload):
        self._attr_extra_state_attributes = {"last_message": payload}
        if self.hass:
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state


class Xt211DatetimeSensor(SensorEntity):
    def __init__(self, device_info, entry_id): # <--- ZMĚNA: Přidán argument entry_id
        self._attr_name = "XT211 Last Datetime"
        self._attr_unique_id = f"{entry_id}_datetime" # <--- ZMĚNA: Opraveno vytváření unique_id
        self._attr_device_info = device_info
        self._state = None

    def set_value(self, val):
        self._state = val
        if self.hass:
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state

class Xt211BatterySensor(SensorEntity):
    def __init__(self, device_info, entry_id):
        self._attr_name = "XT211 Battery"
        self._attr_unique_id = f"{entry_id}_battery"
        self._attr_device_info = device_info
        self._state = None
        self._attr_extra_state_attributes = {}

    def set_value(self, val):
        self._state = val
        if self.hass:
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state

class Xt211WakeupsSensor(SensorEntity):
    def __init__(self, device_info, entry_id):
        self._attr_name = "XT211 Wakeups"
        self._attr_unique_id = f"{entry_id}_wakeups"
        self._attr_device_info = device_info
        self._state = None

    def set_value(self, val):
        self._state = val
        if self.hass:
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state

class Xt211RSSISensor(SensorEntity):
    def __init__(self, device_info, entry_id):
        self._attr_name = "XT211 Wifi"
        self._attr_unique_id = f"{entry_id}_rssi"
        self._attr_device_info = device_info
        self._state = None

    def set_value(self, val):
        self._state = val
        if self.hass:
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state


class Xt211ObisSensor(SensorEntity):
    def __init__(self, device_info, entry_id, obis): # <--- ZMĚNA: Přidán argument entry_id
        self.obis = obis
        obis_info = SENSOR_MAP.get(obis, {})
        name = obis_info.get("name", obis)
        unit = obis_info.get("unit", None)

        self._attr_name = f"XT211 {name}"
        self._attr_unique_id = f"{entry_id}_{obis}" # <--- ZMĚNA: Opraveno vytváření unique_id
        self._attr_device_info = device_info
        self._attr_native_unit_of_measurement = unit
        self._state = None
        self._pending_state = None

    async def async_added_to_hass(self):
        if self._pending_state is not None:
            self._state = self._pending_state
            self._pending_state = None
            self.async_write_ha_state()

    def set_value(self, val):
        if self.hass is None:
            self._pending_state = val
        else:
            self._state = val
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state


# --- PLACEHOLDER HANDLERS ---

#async def handle_status(msg: str):
#    pass

#async def handle_data(payload: dict):
#    pass
