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
    CONF_INFLUXDB_BUCKET,
    CONF_INFLUXDB_ORG,
    CONF_INFLUXDB_TOKEN

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
                vol.Required(CONF_INFLUXDB_BUCKET): str,
                vol.Optional(CONF_INFLUXDB_ORG, default=""): str,
                vol.Required(CONF_INFLUXDB_TOKEN): str,
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
            return self.async_create_entry(title="", data=user_input)

        # --- OPRAVA: Načtení aktuálních hodnot ---
        # Podíváme se, jestli už máme něco v options, jinak bereme data
        current_config = {**self.config_entry.data, **self.config_entry.options}

        data_schema = vol.Schema(
            {
                vol.Required(
                    CONF_TOPIC_DATA,
                    default=current_config.get(CONF_TOPIC_DATA, DEFAULT_TOPIC_DATA),
                ): str,
                vol.Required(
                    CONF_TOPIC_STATUS,
                    default=current_config.get(CONF_TOPIC_STATUS, DEFAULT_TOPIC_STATUS),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_HOST,
                    default=current_config.get(CONF_INFLUXDB_HOST, ""),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_PORT,
                    default=current_config.get(CONF_INFLUXDB_PORT, "8086"),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_BUCKET,
                    default=current_config.get(CONF_INFLUXDB_BUCKET, ""),
                ): str,
                vol.Optional(
                    CONF_INFLUXDB_ORG,
                    default=current_config.get(CONF_INFLUXDB_ORG, ""),
                ): str,
                vol.Required(
                    CONF_INFLUXDB_TOKEN,
                    default=current_config.get(CONF_INFLUXDB_TOKEN, ""),
                ): str,
            }
        )
        return self.async_show_form(step_id="init", data_schema=data_schema)
        