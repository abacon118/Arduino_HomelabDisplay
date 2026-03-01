#pragma once

// WiFi network to connect to
const char* WIFI_SSID = "NachoWifi";
const char* WIFI_PASS = "CertianlyN0tMyP@ssw0rd!";
const char* hostname = "ESPDisplay";
// Define password for ArduinoOTA
const char* OTA_PASS = "Pass*Word";


//Pinout
const int SDAPin = 32;
const int SCLPin = 33;
const int ldrPin = 15; //Light Dependant Resistor (LDR) Port


// SNMP hosts
const IPAddress SNMP_ADDR_OctoPi(192, 168, 0, 5);
const IPAddress SNMP_ADDR_TRUENAS(192, 168, 0, 4);
const char* SNMPCommunity = "notpublic";

const IPAddress IPMI_ADDR(192, 168, 0, 2);
const char* IPMI_USER = "user";
const char* IPMI_PASS = "password";


// Proxmox
const char* PROXMOX_ADDR0 = "192.168.0.2:8006";
const char* PROXMOX_NODE0 = "PVE0";
const char* PROXMOX_TOKEN_ID0 = "";
const char* PROXMOX_TOKEN_SECRET0 = "secret";

const char* PROXMOX_NODE1 = "pve1";

const char* PI0_ADDR = "192.168.0.9:2002";
const char* PI0_API ="apikey";
const char* PI0_Name = "OctoPi";

const char* PI1_ADDR = "192.168.0.10:2002";
const char* PI1_API = "apikey";
const char* PI1_Name = "Pi1";


const char* nodeNames[4] = {  //For screen labels
    PROXMOX_NODE0,PROXMOX_NODE1, PI1_Name, PI0_Name   
};
