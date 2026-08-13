idf_component_register(SRCS "custom_rs485.c"
                      INCLUDE_DIRS "include"
                      PRIV_REQUIRES custom_config driver esp_timer)