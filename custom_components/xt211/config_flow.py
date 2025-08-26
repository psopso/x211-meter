from homeassistant import config_entries
from .const import DOMAIN

class Xt211ConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    async def async_step_user(self, user_input=None):
        if user_input is not None:
            return self.async_create_entry(title="XT211 Meter", data={})
        return self.async_show_form(step_id="user", data_schema=None)
