/*
 * HTTP Communication Module
 * 
 * Handles HTTP requests to Proxmox API for retrieving CPU usage data.
 * Uses token-based authentication for secure communication.
 * V2.0 Andrew Bowman
 * Added support for Proxmox Node Memory
 */

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "Secrets.h"

void updateHTTPCPUUsage(SensorData& sensor) {
    HTTPClient http;
    float cpuUsage = 9999.0;
    ProxmoxHosts hostConfig = proxmoxHosts[sensor.host];

    // Construct API URL for Proxmox node status
    String url = "https://" + String(hostConfig.PROXMOX_ADDR) + "/api2/json/nodes/" + String(hostConfig.PROXMOX_NODE) + "/status";
    Serial.println(url);
    http.begin(url);
    http.setUserAgent("HomelabDisplay/1.0");
    Serial.println(http.getString());

    // Set up authentication header with API token
    String authHeader = "PVEAPIToken=" + String(hostConfig.PROXMOX_TOKEN_ID) + "=" + String(hostConfig.PROXMOX_TOKEN_SECRET);
    Serial.println(authHeader);
    http.addHeader("Authorization", authHeader);
    
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println(payload);
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // CPU usage comes as 0-1 value, convert to percentage
            cpuUsage = doc["data"]["cpu"] | 9.9;
            cpuUsage *= 100.0;
            
            sensor.value = cpuUsage;
            sensor.lastUpdate = millis();
            sensor.isValid = true;
            Serial.printf("CPU Usage: %.1f%%\n", cpuUsage);
        } else {
            Serial.println("JSON parsing failed");
            sensor.isValid = false;
        }
    } else {
        //Serial.printf("HTTP Error: %d\n", httpResponseCode);
        sensor.isValid = false;
    }
    
    http.end();
}
void updateHTTPMEMUsage(SensorData& sensor) {
    HTTPClient http;
    ProxmoxHosts memHostConfig = proxmoxHosts[sensor.host];

    String url = "https://" + String(memHostConfig.PROXMOX_ADDR) + "/api2/json/nodes/" + String(memHostConfig.PROXMOX_NODE) + "/status";
    Serial.println("Requesting: " + url);

    http.begin(url);
    http.setUserAgent("HomelabDisplay/1.0");

    String authHeader = "PVEAPIToken=" + String(memHostConfig.PROXMOX_TOKEN_ID) + "=" + String(memHostConfig.PROXMOX_TOKEN_SECRET);
    http.addHeader("Authorization", authHeader);

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println("Payload: " + payload);

        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            uint64_t totalMemory = doc["data"]["memory"]["total"];
            uint64_t usedMemory  = doc["data"]["memory"]["used"];

            float usagePercent = ((float)usedMemory * 100.0) / (float)totalMemory;

            sensor.value = usagePercent;
            sensor.lastUpdate = millis();
            sensor.isValid = true;

            Serial.printf("Total Memory: %.2f GB\n", totalMemory / pow(1024, 3));
            Serial.printf("Used Memory : %.2f GB\n", usedMemory / pow(1024, 3));
            Serial.printf("Usage       : %.2f %%\n", usagePercent);
        } else {
            Serial.println("JSON parsing failed");
            sensor.isValid = false;
        }
    } else {
        Serial.printf("HTTP Error: %d\n", httpResponseCode);
        sensor.isValid = false;
    }

    http.end();
}
