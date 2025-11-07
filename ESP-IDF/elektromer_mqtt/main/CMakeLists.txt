# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

# Získá UTC čas buildu ve formátu ISO 8601 (např. 2025-10-14T08:33:02Z)
# --- Build datetime (UTC) ---
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

string(TOLOWER "${ENV}" ENV_LOWER1)
message(STATUS "DEBUG (Main): ENV_LOWER z cache = '${ENV_LOWER}'")
message(STATUS "DEBUG (Main): ENV_LOWER1 = '${ENV_LOWER1}'")


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

# PĹ?edej jako compile-time definici (musĂ­ bĂ˝t v uvozovkĂĄch)
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    BUILD_DATETIME="${BUILD_TIMESTAMP}"
)
