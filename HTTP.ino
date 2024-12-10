/*
 * HTTP Communication Module
 * 
 * Handles HTTP requests to Proxmox API for retrieving CPU usage data.
 * Uses token-based authentication for secure communication.
 */

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "Secrets.h"

void updateHTTPCPUUsage(SensorData& sensor) {
    HTTPClient http;
    float cpuUsage = 9999.0;
    
    // Construct API URL for Proxmox node status
    String url = "https://" + String(PROXMOX_ADDR) + "/api2/json/nodes/" + PROXMOX_NODE + "/status";
    http.begin(url);
    
    // Set up authentication header with API token
    String authHeader = "PVEAPIToken=" + String(PROXMOX_TOKEN_ID) + "=" + String(PROXMOX_TOKEN_SECRET);
    http.addHeader("Authorization", authHeader);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
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
        Serial.printf("HTTP Error: %d\n", httpResponseCode);
        sensor.isValid = false;
    }
    
    http.end();
}
