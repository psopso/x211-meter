async def handle_status(hass, payload: str, config):
    # TODO: vlastní logika zpracování statusu
    influx_host = config.get("influxdb_host")
    influx_port = config.get("influxdb_port")
    pass

async def handle_data(hass, payload: str, config):
    # TODO: vlastní logika zpracování dat
    influx_host = config.get("influxdb_host")
    influx_port = config.get("influxdb_port")
    pass
