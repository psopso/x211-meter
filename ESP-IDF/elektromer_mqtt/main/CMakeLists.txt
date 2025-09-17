# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c         # list the source files of this component
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
      custom_config
      custom_rs485
      custom_mqtt
      custom_dlms
      custom_storage
      custom_time
      custom_battery
      custom_ota
      custom_wifi
      custom_led
      driver
      nvs_flash
      
    PRIV_REQUIRES       # optional, list the private requirements
)
