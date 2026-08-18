idf_component_register(SRCS "custom_wifi.c"
                      INCLUDE_DIRS "include"
                      PRIV_REQUIRES 
                        esp_wifi
                        nvs_flash
                        )