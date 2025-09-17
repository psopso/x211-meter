idf_component_register(SRCS "custom_led.c"
                      INCLUDE_DIRS "include"
                      PRIV_REQUIRES custom_config led_strip)