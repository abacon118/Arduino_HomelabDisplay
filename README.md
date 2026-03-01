# Arduino Homelab Display V2

ESP32-based status display for various server metrics forked from [phlntn](https://github.com/phlntn/Arduino_HomelabDisplay)

Implements reading the following sensors:
- CPU temperature via a very rudimentary IPMI client implemention
- Proxmox CPU usage via its HTTP API
- TrueNAS disk usage via SNMP
- UPS power draw via SNMP
- CPU usage via SNMP (unused by default)
- Temperature, CPU, Memory usage via Flask API

Additional functionality:
- Web interface for remote viewing or embedding of current sensor values
- Over-the-air firmware updates via ArduinoOTA


## Parts

1. WT-ETH01 ESP32 Ethernet
2. POE module (optional)
3. TCA9548A I2C Multiplexer
4. SH1106 (as many as you want)


## Setup

1. Install Arduino libraries:
   - Adafruit GFX Library
   - Adafruit SSD1327
   - SNMP Manager
   - ArduinoJson (not Arduino_JSON)
   - U8g2lib
2. Update secrets.h and configure
3. Compile and upload code to ESP32


## Acknowledgments

- Custom fonts created with [GFX Font Editor](https://github.com/ScottFerg56/GFXFontEditor)
- Partially developed with Anthropic Claude and Copilot
- Forked from [phlntn](https://github.com/phlntn/Arduino_HomelabDisplay)
