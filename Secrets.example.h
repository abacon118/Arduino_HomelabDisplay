#pragma once

// WiFi network to connect to
const char* WIFI_SSID = "ssid";
const char* WIFI_PASS = "password";

// Define password for ArduinoOTA
const char* OTA_PASS = "password";

// SNMP hosts
const IPAddress SNMP_ADDR_UPS(192, 168, 0, 1);
const IPAddress SNMP_ADDR_TRUENAS(192, 168, 0, 4);

// IPMI host
const IPAddress IPMI_ADDR(192, 168, 0, 2);
const char* IPMI_USER = "user";
const char* IPMI_PASS = "password";

// Proxmox
const char* PROXMOX_ADDR = "192.168.0.3:8006";
const char* PROXMOX_NODE = "node";
const char* PROXMOX_TOKEN_ID = "api@token!here";
const char* PROXMOX_TOKEN_SECRET = "00000000-0000-0000-0000-000000000000";