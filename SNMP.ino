/*
 * SNMP Monitoring Module
 * 
 * Handles SNMP monitoring of:
 * - UPS power consumption
 * - Disk usage statistics
 * - CPU utilization metrics
 */

#include <Arduino_SNMP_Manager.h>

#include "Secrets.h"

// SNMP configuration
SNMPManager snmp = SNMPManager("public");
const uint16_t SNMP_PORT = 161;

// SNMP callbacks and requests
ValueCallback* callbackUPSPower;
ValueCallback* callbackDiskUsed;
ValueCallback* callbackDiskAvail;
ValueCallback* callbackCpuUser;
ValueCallback* callbackCpuNice;
ValueCallback* callbackCpuSystem;
ValueCallback* callbackCpuIdle;
SNMPGet snmpRequest = SNMPGet("public", 1);  // SNMP v2c

// SNMP OID definitions
const char* OID_UPS_POWER  = ".1.3.6.1.4.1.534.1.4.4.1.4.1";
const char* OID_DISK_USED  = ".1.3.6.1.4.1.50536.1.2.1.1.3.2";
const char* OID_DISK_AVAIL = ".1.3.6.1.4.1.50536.1.2.1.1.4.2";
const char* OID_CPU_USER   = ".1.3.6.1.4.1.2021.11.50.0";
const char* OID_CPU_NICE   = ".1.3.6.1.4.1.2021.11.51.0";
const char* OID_CPU_SYSTEM = ".1.3.6.1.4.1.2021.11.52.0";
const char* OID_CPU_IDLE   = ".1.3.6.1.4.1.2021.11.53.0";

// Response storage
int upsResponse = 0;
uint64_t diskUsedResponse = 0;
uint64_t diskAvailResponse = 0;
uint32_t cpuUserResponse = 0;
uint32_t cpuNiceResponse = 0;
uint32_t cpuSystemResponse = 0;
uint32_t cpuIdleResponse = 0;

// Previous values for delta calculation
uint32_t lastCpuUser = 0;
uint32_t lastCpuNice = 0;
uint32_t lastCpuSystem = 0;
uint32_t lastCpuIdle = 0;

/*
 * Setup
 */

void initializeSNMP() {
    snmp.setUDP(&udp);
    snmp.begin();

    callbackUPSPower  = snmp.addIntegerHandler  (SNMP_ADDR_UPS,     OID_UPS_POWER,  &upsResponse);
    callbackDiskUsed  = snmp.addCounter64Handler(SNMP_ADDR_TRUENAS, OID_DISK_USED,  &diskUsedResponse);
    callbackDiskAvail = snmp.addCounter64Handler(SNMP_ADDR_TRUENAS, OID_DISK_AVAIL, &diskAvailResponse);
    callbackCpuUser   = snmp.addCounter32Handler(SNMP_ADDR_TRUENAS, OID_CPU_USER,   &cpuUserResponse);
    callbackCpuNice   = snmp.addCounter32Handler(SNMP_ADDR_TRUENAS, OID_CPU_NICE,   &cpuNiceResponse);
    callbackCpuSystem = snmp.addCounter32Handler(SNMP_ADDR_TRUENAS, OID_CPU_SYSTEM, &cpuSystemResponse);
    callbackCpuIdle   = snmp.addCounter32Handler(SNMP_ADDR_TRUENAS, OID_CPU_IDLE,   &cpuIdleResponse);
}

/*
 * Fetch values
 */

void getSNMPUPSData() {
    snmpRequest.setIP(WiFi.localIP());
    snmpRequest.setUDP(&udp);

    snmpRequest.setRequestID(random(9999));
    snmpRequest.addOIDPointer(callbackUPSPower);
    snmpRequest.sendTo(SNMP_ADDR_UPS);

    delay(100);
    snmp.loop();
    snmpRequest.clearOIDList();
}

void getSNMPDiskData() {
    snmpRequest.setIP(WiFi.localIP());
    snmpRequest.setUDP(&udp);
    snmpRequest.setRequestID(random(9999));

    snmpRequest.addOIDPointer(callbackDiskUsed);
    snmpRequest.addOIDPointer(callbackDiskAvail);
    snmpRequest.sendTo(SNMP_ADDR_TRUENAS);

    delay(100);
    snmp.loop();
    snmpRequest.clearOIDList();
}

void getSNMPCPUData() {
    snmpRequest.setIP(WiFi.localIP());
    snmpRequest.setUDP(&udp);
    snmpRequest.setRequestID(random(9999));

    snmpRequest.addOIDPointer(callbackCpuUser);
    snmpRequest.addOIDPointer(callbackCpuNice);
    snmpRequest.addOIDPointer(callbackCpuSystem);
    snmpRequest.addOIDPointer(callbackCpuIdle);
    snmpRequest.sendTo(SNMP_ADDR_TRUENAS);

    delay(100);
    snmp.loop();
    snmpRequest.clearOIDList();
}

/*
 * Parse values
 */

void updateSNMPUPSPower(SensorData& sensor) {
    getSNMPUPSData();

    // Only update if we got a valid response
    if (upsResponse > 0) {
        sensor.value = upsResponse;
        sensor.lastUpdate = millis();
        sensor.isValid = true;
        Serial.printf("UPS Power: %d W\n", upsResponse);
    } else {
        sensor.isValid = false;
    }
}

void updateSNMPDiskUsage(SensorData& sensor) {
    getSNMPDiskData();

    // Calculate percentage
    if (diskUsedResponse > 0 || diskAvailResponse > 0) {
        float totalSpace = diskUsedResponse + diskAvailResponse;
        float percentage = (diskUsedResponse * 100.0) / totalSpace;
        sensor.value = percentage;
        sensor.lastUpdate = millis();
        sensor.isValid = true;
        Serial.printf("Disk Usage: %.1f%%\n", percentage);
    } else {
        sensor.isValid = false;
    }
}

void updateSNMPCPUUsage(SensorData& sensor) {
    getSNMPCPUData();
    
    // Calculate deltas
    uint32_t deltaUser   = cpuUserResponse   - lastCpuUser;
    uint32_t deltaNice   = cpuNiceResponse   - lastCpuNice;
    uint32_t deltaSystem = cpuSystemResponse - lastCpuSystem;
    uint32_t deltaIdle   = cpuIdleResponse   - lastCpuIdle;
    
    // Save current values for next calculation
    lastCpuUser   = cpuUserResponse;
    lastCpuNice   = cpuNiceResponse;
    lastCpuSystem = cpuSystemResponse;
    lastCpuIdle   = cpuIdleResponse;
    
    // Calculate total and percentage
    uint32_t totalDelta = deltaUser + deltaNice + deltaSystem + deltaIdle;
    if (totalDelta > 0) {
        float cpuUsage = 100.0 * (deltaUser + deltaNice + deltaSystem) / totalDelta;
        sensor.value = cpuUsage;
        sensor.lastUpdate = millis();
        sensor.isValid = true;
        Serial.printf("CPU Usage: %.1f%%\n", cpuUsage);
    } else {
        sensor.isValid = false;
    }
}

/*
 * Loop
 */

void refreshSNMP() {
    snmp.loop();
}