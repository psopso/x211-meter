import voluptuous as vol
from homeassistant import config_entries
from homeassistant.core import callback

from .const import (
    DOMAIN,
    CONF_TOPIC_DATA,
    CONF_TOPIC_STATUS,
    DEFAULT_TOPIC_DATA,
    DEFAULT_TOPIC_STATUS,
    CONF_INFLUXDB_HOST,
    CONF_INFLUXDB_PORT,
    CONF_INFLUXDB_DATABASE,
    CONF_INFLUXDB_USERNAME,
    CONF_INFLUXDB_PASSWORD

)

class Xt211ConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """Handle a config flow for XT211 Meter."""

    VERSION = 1

    async def async_step_user(self, user_input=None):
        """Handle the initial step."""
        # Zabráníme přidání více instancí integrace
        if self._async_current_entries():
            return self.async_abort(reason="single_instance_allowed")

        if user_input is not None:
            return self.async_create_entry(title="XT211 Meter", data=user_input)

        data_schema = vol.Schema(
            {
                vol.Required(CONF_TOPIC_DATA, default=DEFAULT_TOPIC_DATA): str,
                vol.Required(CONF_TOPIC_STATUS, default=DEFAULT_TOPIC_STATUS): str,
                vol.Required(CONF_INFLUXDB_HOST): str,
                vol.Required(CONF_INFLUXDB_PORT, default="8086"): str,
                vol.Required(CONF_INFLUXDB_DATABASE): str,
                vol.Optional(CONF_INFLUXDB_USERNAME, default=""): str,
                vol.Required(CONF_INFLUXDB_PASSWORD): str,
            }
        )
        return self.async_show_form(step_id="user", data_schema=data_schema)
        
    # --- NOVÁ ČÁST: Připojení Options Flow ---
    @staticmethod
    @callback
    def async_get_options_flow(config_entry: config_entries.ConfigEntry):
        """Get the options flow for this handler."""
        return Xt211OptionsFlowHandler(config_entry)


class Xt211OptionsFlowHandler(config_entries.OptionsFlow):
    """Handle an options flow for XT211 Meter."""

    def __init__(self, config_entry: config_entries.ConfigEntry):
        """Initialize options flow."""
        self.config_entry = config_entry

    async def async_step_init(self, user_input=None):
        """Manage the options."""
        if user_input is not None:
            # Uložíme data. Tím se automaticky spustí update_listener
            return self.async_create_entry(title="", data=user_input)

        # Zobrazíme formulář, ale jako výchozí hodnoty použijeme
        # ty, které jsou aktuálně nastavené.
        data_schema = vol.Schema(
            {
                vol.Required(
                    CONF_TOPIC_DATA,
                    default=self.config_entry.data.get(CONF_TOPIC_DATA),
                ): str,
                vol.Required(
                    CONF_TOPIC_STATUS,
                    default=self.config_entry.data.get(CONF_TOPIC_STATUS),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_HOST,
                    default=self.config_entry.data.get(CONF_INFLUXDB_HOST, ""),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_PORT,
                    default=self.config_entry.data.get(CONF_INFLUXDB_PORT, "8086"),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_DATABASE,
                    default=self.config_entry.data.get(CONF_INFLUXDB_DATABASE, ""),
                ): str,
                vol.Optional(
                    CONF_INFLUXDB_USERNAME,
                    default=self.config_entry.data.get(CONF_INFLUXDB_USERNAME, ""),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_PASSWORD,
                    default=self.config_entry.data.get(CONF_INFLUXDB_PASSWORD, ""),
                ): str,
            }
        )
        return self.async_show_form(step_id="init", data_schema=data_schema)
        