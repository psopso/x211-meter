idf_component_register(SRCS "custom_mqtt.c"
                      INCLUDE_DIRS "include"
                      PRIV_REQUIRES 
                        custom_config 
                        custom_storage
                        custom_dlms
                        custom_ota
                        custom_wifi
                        mqtt
                        json
                        esp_wifi
                        )