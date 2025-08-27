/* connections.cpp */
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "connections.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi je již připojeno.");
        return;
    }

    IPAddress local_IP(192, 168, 143, 152);
    IPAddress gateway(192, 168, 143, 250);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress primaryDNS(192, 168, 143, 250);

    Serial.print("Připojuji se k WiFi...");
    WiFi.mode(WIFI_STA);

    WiFi.config(local_IP, gateway, subnet, primaryDNS);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi připojeno. IP adresa: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nNepodařilo se připojit k WiFi.");
    }
}

void disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi odpojeno.");
}

void syncNTP() {
    Serial.print("Synchronizuji čas přes NTP... ");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Nepodařilo se získat čas.");
        return;
    }
    Serial.println("Čas úspěšně synchronizován.");
    char timeStr[30];
    strftime(timeStr, sizeof(timeStr), "%A, %d %B %Y %H:%M:%S", &timeinfo);
    Serial.println(timeStr);
}

void connectMQTT() {
    if (mqttClient.connected()) {
        Serial.println("MQTT je již připojeno.");
        return;
    }
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    Serial.print("Připojuji se k MQTT brokeru...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
        Serial.println(" připojeno.");
    } else {
        Serial.print(" selhalo, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" Zkuste to znovu za 5 sekund");
    }
}

void disconnectMQTT() {
    mqttClient.disconnect();
    Serial.println("MQTT odpojeno.");
}

void publishStatus(const char* message) {
    if (mqttClient.connected()) {
        if(mqttClient.publish(MQTT_TOPIC_STATUS, message)) {
            Serial.printf("Zpráva odeslána na topic [%s]: %s\n", MQTT_TOPIC_STATUS, message);
        } else {
            Serial.println("Odeslání stavové zprávy selhalo.");
        }
    } else {
        Serial.println("Nelze odeslat stav, MQTT není připojeno.");
    }
}

void publishData(const char* jsonData) {
    if (mqttClient.connected()) {
        if(mqttClient.publish(MQTT_TOPIC_DATA, jsonData, true)) { // true = retained message
            Serial.printf("Data odeslána na topic [%s]\n", MQTT_TOPIC_DATA);
        } else {
            Serial.println("Odeslání dat selhalo.");
        }
    } else {
        Serial.println("Nelze odeslat data, MQTT není připojeno.");
    }
}
