async def handle_status(hass, payload: str, config):
    # TODO: vlastní logika zpracování statusu
    influx_host = config.get("influxdb_host")
    influx_port = config.get("influxdb_port")
    bucket = config.get(CONF_INFLUXDB_BUCKET)
    org = config.get(CONF_INFLUXDB_ORG)
    token = config.get(CONF_INFLUXDB_TOKEN)
    
    pass

async def handle_data(hass, payload: str, config):
    # TODO: vlastní logika zpracování dat
    influx_host = config.get("influxdb_host")
    influx_port = config.get("influxdb_port")
    bucket = config.get(CONF_INFLUXDB_BUCKET)
    org = config.get(CONF_INFLUXDB_ORG)
    token = config.get(CONF_INFLUXDB_TOKEN)
    
    pass
