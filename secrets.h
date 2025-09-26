#pragma once

// WiFi network to connect to
const char* WIFI_SSID = "DropItLikeItsHotspot";
const char* WIFI_PASS = "NachoWifi";

// Define password for ArduinoOTA
const char* OTA_PASS = "Ch@ngeM3!!!";

// SNMP hosts
const IPAddress SNMP_ADDR_UPS(192, 168, 0, 1);
const IPAddress SNMP_ADDR_TRUENAS(192, 168, 0, 4);

const IPAddress IPMI_ADDR(192, 168, 0, 2);
const char* IPMI_USER = "user";
const char* IPMI_PASS = "password";


// Proxmox
const char* PROXMOX_ADDR0 = "192.168.0.1:8006";
const char* PROXMOX_NODE0 = "NODE0";
const char* PROXMOX_TOKEN_ID0 = "user@pve!APIKEY";
const char* PROXMOX_TOKEN_SECRET0 = "secret";

const char* PROXMOX_NODE1 = "NODE1";


const char* nodeNames[2] = {
    PROXMOX_NODE0,PROXMOX_NODE1
};
