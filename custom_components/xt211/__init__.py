"""The XT211 Meter integration."""
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant

# Seznam platforem, které integrace používá (v našem případě jen sensor)
PLATFORMS = ["sensor"]

async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Set up XT211 Meter from a config entry."""
    # Předá nastavení platformě sensor
    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    
    # Přidá listener, který bude sledovat změny v options
    entry.async_on_unload(entry.add_update_listener(update_listener))
    
    return True


async def async_unload_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Unload a config entry."""
    # Správně "vyloží" platformy, když je integrace odstraňována nebo znovu načítána
    return await hass.config_entries.async_unload_platforms(entry, PLATFORMS)


async def update_listener(hass: HomeAssistant, entry: ConfigEntry) -> None:
    """Handle options update."""
    # Znovu načte integraci, aby se projevily nové hodnoty z options
    await hass.config_entries.async_reload(entry.entry_id)
