# profiles/prod
# -B build-production -DSDKCONFIG=build-production/sdkconfig -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.prod"

cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED ENV)
    message(WARNING "ENV not set, using default 'home'")
    set(ENV home)
endif()

string(TOLOWER "${ENV}" ENV_LOWER)

# Ulož do cache, aby bylo viditelné i pro podadresáře (main/, components/)
set(ENV_LOWER "${ENV_LOWER}" CACHE INTERNAL "normalized environment name" FORCE)

if(ENV_LOWER STREQUAL "home")
	message(STATUS "Kompilace pro prostředí: HOME")
#    set(IDF_TARGET "esp32" CACHE STRING "" FORCE)
    set(SDKCONFIG_DEFAULTS "sdkconfig.home")
    add_compile_definitions(ENV_HOME)
elseif(ENV_LOWER STREQUAL "work")
	message(STATUS "Kompilace pro prostředí: WORK")
#    set(IDF_TARGET "esp32" CACHE STRING "" FORCE)
    set(SDKCONFIG_DEFAULTS "sdkconfig.work")
    add_compile_definitions(ENV_WORK)
elseif(ENV_LOWER STREQUAL "test")
	message(STATUS "Kompilace pro prostředí: TEST")
#    set(IDF_TARGET "esp32S2" CACHE STRING "" FORCE)
    set(SDKCONFIG_DEFAULTS "sdkconfig.test")
    add_compile_definitions(ENV_TEST)
else()
    message(FATAL_ERROR "Unknown ENV=${ENV}")
endif()

set(PROJECT_NAME "elektromer_mqtt_${ENV_LOWER}")
message(STATUS "Nazev projektu: ${PROJECT_NAME}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(${PROJECT_NAME})
