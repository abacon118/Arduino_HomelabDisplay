/*
 * Arduino Homelab Display
 * Original Repository: https://github.com/phlntn/Arduino_HomelabDisplay
 * Main controller for an OLED display showing real-time server metrics.
 * Monitors CPU usage, temperature, disk usage, and UPS power consumption
 * through various protocols (SNMP, IPMI, HTTP).

 * V2 Andrew Bowman 2025
 * Added support for multiple Proxmox Nodes and Proxmox Node Memory
 */

#ifndef ETH_PHY_MDC
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#if CONFIG_IDF_TARGET_ESP32
#define ETH_PHY_ADDR  0
#define ETH_PHY_MDC   23
#define ETH_PHY_MDIO  18
#define ETH_PHY_POWER -1
#define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#elif CONFIG_IDF_TARGET_ESP32P4
#define ETH_PHY_ADDR  0
#define ETH_PHY_MDC   31
#define ETH_PHY_MDIO  52
#define ETH_PHY_POWER 51
#define ETH_CLK_MODE  EMAC_CLK_EXT_IN
#endif
#endif

#include <Wire.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <U8g2lib.h>
#include "Secrets.h"

WiFiUDP udp;



// Layout
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int CELL_GAP = 4;
const int CELL_WIDTH = (SCREEN_WIDTH - CELL_GAP) / 2;
const int CELL_HEIGHT = 15;
const int NUM_DISPLAY = 2;
// Sensor config
const int NUM_SENSORS = 10;
const int REFRESH_INTERVAL = 30000;
const int VALIDITY_DURATION = REFRESH_INTERVAL * 3;
int ldrStatus = 0;
bool displaysSleeping = false;
bool rotate180 = true;
#define NUM_PVE_HOSTS 4
#define TCAADDR 0x70  // Default address

// Display setup
U8G2 *displays[NUM_DISPLAY];

// Create one display instance per channel
//U8G2_SH1106_128X64_NONAME_F_HW_I2C displays[NUM_DISPLAY] = {
//  U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE),
//  U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE)
//};



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
  int display;
};

struct APIHosts {
  int host;
  const char* ADDR;
  const char* PROXMOX_NODE;
  const char* PROXMOX_TOKEN_ID;
  const char* TOKEN_SECRET;
};

APIHosts hosts[NUM_PVE_HOSTS] = {
  { 0, PROXMOX_ADDR0, PROXMOX_NODE0, PROXMOX_TOKEN_ID0, PROXMOX_TOKEN_SECRET0 },
  { 1, PROXMOX_ADDR0, PROXMOX_NODE1, PROXMOX_TOKEN_ID0, PROXMOX_TOKEN_SECRET0 },
  { 2, PI0_ADDR, PI0_Name, PI0_Name, PI0_API},
  { 3, PI1_ADDR, PI1_Name, PI1_Name, PI1_API},
};



SensorData sensors[NUM_SENSORS] = { //host, x, y, screen
  { 0.0, ULONG_MAX, false, "cpu", "CPU", "%", 0, 0, 1, 0 },
  { 0.0, ULONG_MAX, false, "cpu", "CPU", "%", 1, 1, 1, 0 },
  { 0.0, ULONG_MAX, false, "mem", "MEM", "%", 0, 0, 2, 0 },
  { 0.0, ULONG_MAX, false, "mem", "MEM", "%", 1, 1, 2, 0 },
  { 0.0, ULONG_MAX, false, "FlaskCPU", "CPU", "%", 3, 0, 1, 1 },
  { 0.0, ULONG_MAX, false, "FlaskMEM", "MEM", "%", 3, 0, 2, 1 },
  { 0.0, ULONG_MAX, false, "FlaskTEMP", "TEMP", "C", 3, 0, 3, 1 },
  { 0.0, ULONG_MAX, false, "FlaskCPU", "CPU", "%", 2, 1, 1, 1 },
  { 0.0, ULONG_MAX, false, "FlaskMEM", "MEM", "%", 2, 1, 2, 1 },
  { 0.0, ULONG_MAX, false, "FlaskTEMP", "TEMP", "C", 2, 1, 3, 1 },

};


void tcaSelect(uint8_t i) {
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);  // Select channel i
  Wire.endTransmission();
}

static bool eth_connected = false;

void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname(hostname);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.println(ETH.localIP());
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void initializeDisplays() {
    for (uint8_t i = 0; i < NUM_DISPLAY; i++) {
      if (rotate180) {
        displays[i] = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R2, U8X8_PIN_NONE);
      } else {
        displays[i] = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
      }}
  Wire.begin(SDAPin,SCLPin); 
  for (uint8_t j = 0; j < NUM_DISPLAY; j++) {
    U8G2* d = displays[j];      // Reference correct display instance
    tcaSelect(j);
    d->begin();
    d->clearBuffer();
    d->setFont(u8g2_font_6x10_tf);  // Choose your font
    d->drawStr(0, 12, "Init Display");
    d->sendBuffer();
  }
}




void connectToEthernet() {
  Serial.println("Starting Ethernet...");

  Network.onEvent(onEvent);
  ETH.begin();

  // Wait for IP, not just link
  unsigned long start = millis();
  while (!eth_connected && millis() - start < 10000) {
    Serial.println("Waiting for DHCP...");
    delay(500);
  }

  if (!eth_connected) {
    Serial.println("DHCP failed-> IP is still 0.0.0.0");
  } else {
    Serial.print("Ethernet ready. IP: ");
    Serial.println(ETH.localIP());
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println("Starting Homelab Display (SH1106)");

  initializeDisplays();
  //testDisplays();
  connectToEthernet();
  initializeSNMP();
  setupServer();

  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();
}

void updateSensorIfStale(SensorData& sensor) {
  unsigned long now = millis();
  if (now - sensor.lastUpdate >= REFRESH_INTERVAL) {
    if (strcmp(sensor.id, "temp") == 0) updateIPMICPUTemp(sensor);
    else if (strcmp(sensor.id, "cpu") == 0) updateHTTPCPUUsage(sensor);
    else if (strcmp(sensor.id, "mem") == 0) updateHTTPMEMUsage(sensor);
    else if (strcmp(sensor.id, "FlaskCPU") == 0) updateHTTPFlaskUsage(sensor);
    else if (strcmp(sensor.id, "FlaskMEM") == 0) updateHTTPFlaskUsage(sensor);
    else if (strcmp(sensor.id, "FlaskTEMP") == 0) updateHTTPFlaskUsage(sensor);
    else if (strcmp(sensor.id, "ups") == 0) updateSNMPUPSPower(sensor);
    else if (strcmp(sensor.id, "disk") == 0) updateSNMPDiskUsage(sensor);
    else if (strcmp(sensor.id, "disk") == 0) updateSNMPCPUUsage(sensor);
  }
}

void updateAllSensors() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    updateSensorIfStale(sensors[i]);
  }
  refreshSNMP();
}

void updateDisplay() {


  unsigned long now = millis();
  
  for (uint8_t j = 0; j < NUM_DISPLAY; j++) {
    tcaSelect(j);               // Select correct I²C channel
    U8G2* d = displays[j];      // Reference correct display instance
    d->clearBuffer();            // Clear buffer before drawing
    Serial.printf("Updating display %d\n", j);

    
    // Draw header bar
    d->setDrawColor(1);          // White
    d->drawBox(0, 0, SCREEN_WIDTH, 12);
    // Draw node names in black over white bar
    d->setDrawColor(0);          // Black text
    d->setFont(u8g2_font_7x13_tf);
    d->drawStr(2, 10, nodeNames[j*2]);  // Left-aligned node name
    d->drawStr(SCREEN_WIDTH - 64, 10, nodeNames[(j*2)+1]);  // Right-aligned peer
    d->setDrawColor(1);          // Back to white drawing
    d->drawLine(SCREEN_WIDTH / 2, 12, SCREEN_WIDTH / 2, SCREEN_HEIGHT);     // Divider line

    // Draw sensors assigned to this display
    for (int i = 0; i < NUM_SENSORS; i++) {
      if (sensors[i].display == j) {
        int x = sensors[i].gridX * (CELL_WIDTH + CELL_GAP);
        int y = sensors[i].gridY * (CELL_HEIGHT + 2);

        //d->drawBox(x, y, CELL_WIDTH, CELL_HEIGHT);          // Sensor cell background
        // Sensor label
        d->setDrawColor(1); 
        d->drawStr(x + 2, y + 12, sensors[i].label);
        // Sensor value
        char valueStr[8] = "-";
        if (sensors[i].isValid && (now - sensors[i].lastUpdate < VALIDITY_DURATION)) {
          snprintf(valueStr, sizeof(valueStr), "%.0f%s", sensors[i].value, sensors[i].unit);
        }

        d->drawStr(x + CELL_WIDTH - 30, y + 12, valueStr);
        d->setDrawColor(1);  // Reset to white for next box
      }
    }

    d->sendBuffer();  // Flush buffer to display
  }
}



void loop() {
  ArduinoOTA.handle();
  ldrStatus = analogRead(ldrPin);
  Serial.print("LDR Brightness: ");
  Serial.println(ldrStatus);

  updateAllSensors();

  if(ldrStatus < 2000 && !displaysSleeping){
    Serial.println("Putting displays to sleep");
    for (uint8_t j = 0; j < NUM_DISPLAY; j++) {
      tcaSelect(j);               // Select correct I²C channel
      U8G2* d = displays[j];      // Reference correct display instance
      d->clearBuffer();            // Clear buffer before drawing
      d->setPowerSave(1);
  }
  displaysSleeping = true; }

  if(ldrStatus >= 2000 && displaysSleeping){
    Serial.println("Waking up displays");
    for (uint8_t j = 0; j < NUM_DISPLAY; j++) {
      tcaSelect(j);               // Select correct I²C channel
      U8G2* d = displays[j];      // Reference correct display instance
      d->clearBuffer();            // Clear buffer before drawing
      d->setPowerSave(0);
  }
  displaysSleeping = false; }
  
  if(!displaysSleeping){
  updateDisplay();
  handleServer();
  }


  

    delay(500);
}
