#pragma once

#include "sdkconfig.h"

#include <stdbool.h>
#include <time.h>
// --- GPIO Configuration ---
#define POWER_ENABLE_GPIO       2
#define RS485_TXD_PIN           17
#define RS485_RXD_PIN           16
#define RS485_RTS_PIN           (-1) // Not used for push mode
#define I2C_SDA_PIN             21
#define I2C_SCL_PIN             22
#define RS485_ENABLE_RX			32
#define RS485_BAD_FRAMES_MAX	3

// --- Wi-Fi & MQTT Configuration ---
#define WIFI_SSID               CONFIG_ESP_WIFI_SSID
#define WIFI_PASSWORD           CONFIG_ESP_WIFI_PASSWORD
#define WIFI_IP					CONFIG_ESP_WIFI_IP
#define WIFI_GW					CONFIG_ESP_WIFI_GW
#define WIFI_NETMASK			CONFIG_ESP_WIFI_NETMASK

#ifdef ENV_HOME

#elif defined(ENV_WORK)

#elif defined(ENV_TEST)

#else
#error "Musíš definovat prostředí (ENV_HOME, ENV_WORK, ENV_TEST)"
#endif

#define MQTT_BROKER_URI         CONFIG_MQTT_BROKER_URI
#define MQTT_USERNAME           CONFIG_MQTT_USERNAME
#define MQTT_PASSWORD           CONFIG_MQTT_PASSWORD
#define MQTT_DATA_TOPIC         CONFIG_MQTT_TOPIC_BASE "/data"
#define MQTT_STATUS_TOPIC       CONFIG_MQTT_TOPIC_BASE "/status"
#define OTA_URL 				CONFIG_OTA_URL
// --- Timing Configuration ---
#define NTP_SERVER              "pool.ntp.org"

#ifndef ENV_TEST
  #if CONFIG_MY_DEBUG_ENABLED
    #define DEEP_SLEEP_INTERVAL_MIN 5      // v minutách (15*60=900)
    #define DEEP_SLEEP_INTERVAL_MIN_NTP 5      // v minutách
  #else
    #define DEEP_SLEEP_INTERVAL_MIN 15      // v minutách (15*60=900)
    #define DEEP_SLEEP_INTERVAL_MIN_NTP 15      // v minutách
  #endif
#else
#define DEEP_SLEEP_INTERVAL_MIN 5      // v minutách (15*60=900)
#define DEEP_SLEEP_INTERVAL_MIN_NTP 5      // v minutách
#endif

#define TIME_SYNC_INTERVAL_MIN  60      // v minutách (použito pro rozhodování, ne pro buzení)
#define DATA_RETENTION_HOURS    24      // v hodinách
#define FRAME_WAIT_TIMEOUT_S    70      // v sekundách (o něco více než 60s)
#define FRAME_END_TIMEOUT_MS    100
#define FAILED_FRAME_ATTEMPTS   5
#define DEEP_SLEEP_OFFSET_S     10      // v sekundách (o kolik dříve se probudit)(900-15=885)

// --- Application Settings ---
#define UART_PORT_NUM           UART_NUM_2
#define UART_BAUD_RATE          9600
#define UART_BUFFER_SIZE        512
#define QUEUE_SIZE              100 // (60 min / 15 min) * 24 hours = 96
#define UART_RX_TIMEOUT_MS      20

// --- Optional FRAM ---
// #define USE_FRAM_MEMORY

extern bool wifiInitialized;
extern void formatDatetime(time_t frametime, char *s);
