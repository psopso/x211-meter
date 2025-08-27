import json
import logging
from homeassistant.components import mqtt
from homeassistant.components.sensor import SensorEntity
from homeassistant.helpers.entity import DeviceInfo

from .const import DOMAIN, MQTT_TOPIC_DATA, MQTT_TOPIC_STATUS, SENSOR_MAP

_LOGGER = logging.getLogger(__name__)

async def async_setup_entry(hass, entry, async_add_entities):
    device_info = DeviceInfo(
        identifiers={(DOMAIN, "xt211")},
        name="XT211 Meter",
        manufacturer="Unknown",
        model="XT211",
    )

    status_sensor = Xt211StatusSensor(device_info)
    raw_data_sensor = Xt211RawDataSensor(device_info)
    datetime_sensor = Xt211DatetimeSensor(device_info)

    entities = [status_sensor, raw_data_sensor, datetime_sensor]
    async_add_entities(entities)

    async def message_received_data(msg):
        try:
            payload = json.loads(msg.payload)
        except Exception as e:
            _LOGGER.error("Invalid JSON: %s", e)
            return

        # raw data
        raw_data_sensor.set_raw(payload)

        # datetime
        if "datetime" in payload:
            datetime_sensor.set_value(payload["datetime"])

        new_entities = []
        for obis, value in payload.get("values", {}).items():
            sensor = next(
                (e for e in entities if isinstance(e, Xt211ObisSensor) and e.obis == obis),
                None,
            )
            if sensor is None:
                # nový senzor podle OBIS mapy
                sensor = Xt211ObisSensor(device_info, obis)
                entities.append(sensor)
                new_entities.append(sensor)
            sensor.set_value(value)

        if new_entities:
            async_add_entities(new_entities)

        await handle_data(payload)

    async def message_received_status(msg):
        status_sensor.set_value(msg.payload)
        await handle_status(msg.payload)

    await mqtt.async_subscribe(hass, MQTT_TOPIC_DATA, message_received_data)
    await mqtt.async_subscribe(hass, MQTT_TOPIC_STATUS, message_received_status)


# --- ENTITY CLASSES ---

class Xt211StatusSensor(SensorEntity):
    def __init__(self, device_info):
        self._attr_name = "XT211 Status"
        self._attr_unique_id = "xt211_status"
        self._attr_device_info = device_info
        self._state = None

    def set_value(self, val):
        self._state = val
        if self.hass:
            self.async_write_ha_state()

    @property
    def native_value(self):
        return self._state


class Xt211RawDataSensor(SensorEntity):
    def __init__(self, device_info):
        self._attr_name = "XT211 Raw Data"
        self._attr_unique_id = "xt211_raw"
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
    def __init__(self, device_info):
        self._attr_name = "XT211 Last Datetime"
        self._attr_unique_id = "xt211_datetime"
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
    def __init__(self, device_info, obis):
        self.obis = obis
        obis_info = SENSOR_MAP.get(obis, {})
        name = obis_info.get("name", obis)
        unit = obis_info.get("unit", None)

        self._attr_name = f"XT211 {name}"
        self._attr_unique_id = f"xt211_{obis}"
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

async def handle_status(msg: str):
    pass

async def handle_data(payload: dict):
    pass
