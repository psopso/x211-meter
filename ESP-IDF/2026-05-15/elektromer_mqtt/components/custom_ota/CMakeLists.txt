idf_component_register(SRCS "custom_ota.c"
                      INCLUDE_DIRS "include"
                      PRIV_REQUIRES esp_http_client app_update)