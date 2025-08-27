/* connections.h */
#pragma once

void connectWiFi();
void disconnectWiFi();
void syncNTP();
void connectMQTT();
void disconnectMQTT();
void publishStatus(const char* message);
void publishData(const char* jsonData);
