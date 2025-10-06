#pragma once

#include <stdbool.h>
#include <time.h>
// --- GPIO Configuration ---
#define POWER_ENABLE_GPIO       2
#define RS485_TXD_PIN           17
#define RS485_RXD_PIN           16
#define RS485_RTS_PIN           (-1) // Not used for push mode
#define I2C_SDA_PIN             21
#define I2C_SCL_PIN             22
#define RS485_BAD_FRAMES_MAX	3

// --- Wi-Fi & MQTT Configuration ---
#define WIFI_SSID               "KaiserData"
#define WIFI_PASSWORD           "Tamade69"
#define WIFI_IP					"192.168.143.153"
#define WIFI_GW					"192.168.143.250"
#define WIFI_NETMASK			"255.255.255.0"

#define MQTT_BROKER_URI         "mqtt://192.168.143.151"
#define MQTT_USERNAME           "admin"
#define MQTT_PASSWORD           "Bubak,3390"
#define MQTT_DATA_TOPIC         "elektromer/data"
#define MQTT_STATUS_TOPIC       "elektromer/status"
#define OTA_URL 				"http://192.168.143.151:8123/api/download/elektromer_mqtt.bin"
// --- Timing Configuration ---
#define NTP_SERVER              "pool.ntp.org"
#define DEEP_SLEEP_INTERVAL_MIN 3  //15      // v minutách
#define DEEP_SLEEP_INTERVAL_MIN_NTP 5  //15      // v minutách
#define TIME_SYNC_INTERVAL_MIN  10 //60      // v minutách (použito pro rozhodování, ne pro buzení)
#define DATA_RETENTION_HOURS    24      // v hodinách
#define FRAME_WAIT_TIMEOUT_S    70      // v sekundách (o něco více než 60s)
#define FRAME_END_TIMEOUT_MS    100
#define FAILED_FRAME_ATTEMPTS   5
#define DEEP_SLEEP_OFFSET_S     15      // v sekundách (o kolik dříve se probudit)

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
