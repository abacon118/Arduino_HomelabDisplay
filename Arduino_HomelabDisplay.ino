/*
 * Arduino Homelab Display
 * Original Repository: https://github.com/phlntn/Arduino_HomelabDisplay
 * Main controller for an OLED display showing real-time server metrics.
 * Monitors CPU usage, temperature, disk usage, and UPS power consumption
 * through various protocols (SNMP, IPMI, HTTP).

 * V2 Andrew Bowman 2025
 * Added multiple Proxmox Nodes
 */

#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> 
#include "Secrets.h"
#include "FontWide8.h"
#include "FontCond16.h"
#include <Fonts/FreeSans9pt7b.h>

WiFiUDP udp;

// Display configuration
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = SCREEN_WIDTH;
const int I2C_SPEED = 1000000;
Adafruit_SH1107 display = Adafruit_SH1107(128, 128, &Wire); // width, height.

// Layout configuration
const int CELL_GAP = 10;
const int CELL_WIDTH = round((SCREEN_WIDTH - CELL_GAP) / 2);
const int CELL_HEIGHT = 42;

// Sensor configuration
const int NUM_SENSORS = 4;
const int REFRESH_INTERVAL = 10000;
const int VALIDITY_DURATION = REFRESH_INTERVAL * 3;

#define NUM_PVE_HOSTS 2

struct SensorData {
    float value;
    unsigned long lastUpdate;
    bool isValid;
    const char* id;
    const char* label;
    const char* unit;
    int host;
    int gridX;
    int gridY;
};

struct ProxmoxHosts {
    // Proxmox
    int host;
    const char* PROXMOX_ADDR;
    const char* PROXMOX_NODE;
    const char* PROXMOX_TOKEN_ID;
    const char* PROXMOX_TOKEN_SECRET;

};

struct ProxmoxSensor {
    int host;
    SensorData sensor;
    ProxmoxHosts proxmox;
};


ProxmoxHosts proxmoxHosts[NUM_PVE_HOSTS] = {
    {0,PROXMOX_ADDR0,PROXMOX_NODE0,PROXMOX_TOKEN_ID0,PROXMOX_TOKEN_SECRET0},
    {1,PROXMOX_ADDR0,PROXMOX_NODE1,PROXMOX_TOKEN_ID0,PROXMOX_TOKEN_SECRET0}
};

SensorData sensors[NUM_SENSORS] = {
    {0.0, ULONG_MAX, false, "cpu",  "CPU2",  "%", 0, 0, 0},
    {0.0, ULONG_MAX, false, "mem", "MEMORY2", "%", 0, 1, 0} ,
    {0.0, ULONG_MAX, false, "cpu", "CPU4", "%", 1, 0, 1},
    {0.0, ULONG_MAX, false, "mem",  "MEM4",  "%", 1, 1, 1}
};


/*
 * Setup Functions
 */

void drawSpinner() {
    unsigned long ms = millis();
    int phase = (ms / 350) % 8;
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;
    int radius = 15;
    
    for (int i = 0; i < 8; i++) {
        float angle = i * (2 * PI / 8);
        int x = round(centerX + cos(angle) * radius);
        int y = round(centerY + sin(angle) * radius);
        
        // Calculate brightness based on phase
       // int brightness = (phase - i) % 8;
        //brightness = 15 - (brightness * 2);
        //if (brightness > 0) {
            display.fillCircle(x, y, 2, SH110X_WHITE);
        //}
    }
}

void initializeDisplay() {
    Wire.begin(8,9);
    Wire.setClock(I2C_SPEED);
    
    if (!display.begin(0x3C)) {
        Serial.println("OLED initialization failed!");
    }
    
    display.setRotation(2);
    display.clearDisplay();
    
    Serial.println("OLED initialized successfully");
}

void connectToWiFi() {
    Serial.printf("Connecting to WiFi network: %s\n", WIFI_SSID);
    Serial.println(WIFI_PASS);

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    display.clearDisplay();
    
    while (WiFi.status() != WL_CONNECTED) {
        display.clearDisplay();
        drawSpinner();
        display.display();
        delay(100);
    }
    
    Serial.println("\nConnected to WiFi");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    
    // Give network stack time to stabilize
    delay(1000);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nStarting status display...");
  
    initializeDisplay();
    connectToWiFi();
    initializeSNMP();
    setupServer();

    ArduinoOTA.setPassword(OTA_PASS);
    ArduinoOTA.begin();

}

/*
 * Main Loop Functions
 */

void updateSensorIfStale(SensorData& sensor) {
    unsigned long now = millis();
    if (now - sensor.lastUpdate >= REFRESH_INTERVAL) {
        if (sensor.id == "temp") {
            updateIPMICPUTemp(sensor);
        }
        else if (sensor.id == "cpu") {
            updateHTTPCPUUsage(sensor);
        }
        else if (sensor.id == "mem") {
            updateHTTPMEMUsage(sensor);
        }
        else if (sensor.id == "ups") {
            updateSNMPUPSPower(sensor);
        }
        else if (sensor.id == "disk") {
            updateSNMPDiskUsage(sensor);
        }
    }
}

void updateAllSensors() {  
    for (int i = 0; i < NUM_SENSORS; i++) {
        updateSensorIfStale(sensors[i]);
    }
    refreshSNMP();
}

void updateDisplay() {
    display.clearDisplay();
    unsigned long now = millis();

    for (int i = 0; i < NUM_SENSORS; i++) {
        int row = sensors[i].gridY;
           
        // Calculate grid position
        int x = round(sensors[i].gridX * (CELL_WIDTH + CELL_GAP));
        int y = round(sensors[i].gridY * (CELL_HEIGHT + CELL_GAP)) + (64-CELL_HEIGHT);
        
        // Draw cell background
        display.fillRoundRect(x, y, CELL_WIDTH, CELL_HEIGHT, 7, 5);
        
        // Draw sensor label
        display.setFont(&FontWide8);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(x + 5, y + 16);
        display.println(sensors[i].label);
        
        // Draw separator line
        int separatorY = y + 20;
        display.drawLine(x, separatorY, x + CELL_WIDTH, separatorY, SH110X_WHITE);
        display.drawLine(x, separatorY + 1, x + CELL_WIDTH, separatorY + 1, SH110X_WHITE);
        
        // Draw sensor value
        display.setFont(&FontCond16);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(x + 5, y + 40);
        char valueStr[6] = "-";
        if (sensors[i].isValid && (now - sensors[i].lastUpdate < VALIDITY_DURATION)) {
            float displayValue = sensors[i].value;
            snprintf(valueStr, sizeof(valueStr), "%.0f%s", displayValue, sensors[i].unit);
        }
        display.println(valueStr);
    }
    display.drawLine(0, 63, 128, 63, SH110X_WHITE);
    display.drawLine(0, 65, 128, 65, SH110X_WHITE);
    
    //Display Node Name
    for(int i = 0; i <2; i++){
        display.setFont(&FreeSans9pt7b); // Use the same font as sensor labels
        display.setTextColor(SH110X_WHITE);
        // Estimate width: FontWide8 is ~6 pixels per character
        int textWidth = strlen(nodeNames[i]) * 7;
        int centeredX = (SCREEN_WIDTH - textWidth) / 2;
        int centeredY = i*(SCREEN_HEIGHT/2)+16; // Top of screen, or adjust as needed

        display.setCursor(centeredX, centeredY);
        display.println(nodeNames[i]); }

    display.display();
}

void loop() {
    ArduinoOTA.handle();

    updateAllSensors();
    updateDisplay();
    handleServer();

    delay(50);
}
