/*
 * IPMI Communication Module
 * 
 * Implements IPMI protocol for server temperature monitoring.
 * Handles session establishment, authentication, and sensor reading.
 */

#include <MD5Builder.h>
#include <string.h>

#include "Secrets.h"
#include "IPMIComm.h"

const bool DEBUG = false;

/*
 * IPMI command templates
 * created by analyzing IPMITool packet captures with Wireguard
 * and cross-referencing them with the IPMI 1.5 specification.
 */

uint8_t IPMI_ASF_PING[] = {
    0x06, 0x00, 0xff, 0x06, 0x00, 0x00, 0x11, 0xbe, 
    0x80, 0x00, 0x00, 0x00
};

uint8_t IPMI_GET_AUTH_CAP[] = {
    0x06, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x20, 0x18, 
    0xc8, 0x81, 0x04, 0x38, 0x0e, 0x04, 0x31
};

uint8_t IPMI_GET_SESS_CHAL[] = {
    0x06, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x20, 0x18,
    0xc8, 0x81, 0x08, 0x39, 0x02, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x33
};

uint8_t IPMI_GET_SESS_ACTV[] = {
    0x06, 0x00, 0xff, 0x07, 0x02, 0x00, 0x00, 0x00, 
    0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x1d, 0x20, 0x18,
    0xc8, 0x81, 0x0c, 0x3a, 0x02, 0x04, 0xaa, 0xaa, 
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 
    0xaa, 0xaa, 0xaa
};

uint8_t IPMI_SET_SESS_PRIV[] = {
    0x06, 0x00, 0xff, 0x07, 0x00, 0xaa, 0xaa, 0xaa, 
    0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0x08, 0x20, 0x18,
    0xc8, 0x81, 0x10, 0x3b, 0x04, 0x30
};

uint8_t IPMI_GET_SENS_READ[] = {
    0x06, 0x00, 0xff, 0x07, 0x00, 0xaa, 0xaa, 0xaa, 
    0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0x08, 0x20, 0x10,
    0xd0, 0x81, 0x0c, 0x2d, 0xaa, 0x13
};

/*
 * Debug Utilities
 */

void printBytes(const char* label, const uint8_t* bytes, size_t length) {
    if (!DEBUG) return;

    Serial.print(label);
    Serial.print(": ");
    if (length > 16) Serial.print("\n  ");

    for (size_t i = 0; i < length; i++) {
        if (bytes[i] < 0x10) Serial.print("0");
        Serial.print(bytes[i], HEX);
        Serial.print(" ");
        if ((i + 1) % 16 == 0 && i != length - 1) {
            Serial.print("\n  ");
        }
    }
    Serial.println();
}

/*
 * IPMI Protocol Functions
 */

void generateOutboundSequence(uint8_t* sequence) {
    for (int i = 0; i < 4; i++) {
        sequence[i] = random(256);
    }
}

uint8_t calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return ~sum + 1;  // Two's complement
}

void calculateAuthCode(const uint8_t* session_id, uint8_t* hash) {
    // Prepare password buffer
    uint8_t padded_password[16] = {0};
    strncpy((char*)padded_password, IPMI_PASS, 16);

    // Build MD5 input buffer
    uint8_t md5_data[16 + 4 + 29 + 4 + 16];
    memcpy(md5_data, padded_password, 16);
    memcpy(md5_data + 16, session_id, 4);
    memcpy(md5_data + 20, &IPMI_GET_SESS_ACTV[30], 29);
    memset(md5_data + 49, 0, 4);
    memcpy(md5_data + 53, padded_password, 16);

    printBytes("MD5 input", md5_data, sizeof(md5_data));

    // Calculate MD5 hash
    MD5Builder md5;
    md5.begin();
    md5.add(md5_data, sizeof(md5_data));
    md5.calculate();
    md5.getBytes(hash);
}

void incrementSequence(uint8_t* sequence) {
    // Convert from big-endian to little-endian
    uint32_t value = (sequence[3] << 24) | (sequence[2] << 16) | 
                     (sequence[1] << 8) | sequence[0];
    value++;
    // Convert back to big-endian
    sequence[0] = value & 0xFF;
    sequence[1] = (value >> 8) & 0xFF;
    sequence[2] = (value >> 16) & 0xFF;
    sequence[3] = (value >> 24) & 0xFF;
}

/*
 * IPMI Session Management
 */

int16_t readIPMISensor(uint8_t sensorId) {
    IPMISession ipmi;

    if (!ipmi.begin(IPMI_ADDR, DEBUG)) {
        Serial.println("Failed to initialize IPMI session");
        return -1;
    }

    // Step 1: ASF Ping
    IPMIResponse response = ipmi.sendCommand(IPMI_ASF_PING, sizeof(IPMI_ASF_PING));
    if (!response.success) return -2;

    // Step 2: Get Authentication Capabilities (although we only actually support MD5)
    response = ipmi.sendCommand(IPMI_GET_AUTH_CAP, sizeof(IPMI_GET_AUTH_CAP));
    if (!response.success) return -3;

    // Step 3: Get Session Challenge
    uint8_t padded_username[16] = {0};
    strncpy((char*)padded_username, IPMI_USER, 16);
    memcpy(&IPMI_GET_SESS_CHAL[21], padded_username, 16);

    response = ipmi.sendCommand(IPMI_GET_SESS_CHAL, sizeof(IPMI_GET_SESS_CHAL));
    if (!response.success) return -4;
    if (response.data[20] != 0x00) return -5;

    // Extract session data
    uint8_t session_id[4];
    memcpy(session_id, &response.data[21], 4);
    printBytes("Session ID", session_id, 4);

    uint8_t challenge[16];
    memcpy(challenge, &response.data[25], 16);
    printBytes("Challenge", challenge, 16);

    // Step 4: Activate Session
    memcpy(&IPMI_GET_SESS_ACTV[9], session_id, 4);
    memcpy(&IPMI_GET_SESS_ACTV[38], challenge, 16);

    uint8_t sequence[4];
    generateOutboundSequence(sequence);
    printBytes("Outbound sequence", sequence, 4);
    memcpy(&IPMI_GET_SESS_ACTV[54], sequence, 4);

    uint8_t checksum = calculateChecksum(&IPMI_GET_SESS_ACTV[38], 20);
    printBytes("Checksum", &checksum, 1);
    memcpy(&IPMI_GET_SESS_ACTV[58], &checksum, 1);

    uint8_t md5hash[16];
    calculateAuthCode(session_id, md5hash);
    printBytes("MD5 hash", md5hash, 16);
    memcpy(&IPMI_GET_SESS_ACTV[13], md5hash, 16);
    
    response = ipmi.sendCommand(IPMI_GET_SESS_ACTV, sizeof(IPMI_GET_SESS_ACTV));
    if (!response.success) return -6;
    if (response.data[36] != 0x00) return -7;

    // Update session data
    memcpy(session_id, &response.data[38], 4);
    printBytes("Session ID", session_id, 4);

    memcpy(sequence, &response.data[42], 4);
    printBytes("Sequence", sequence, 4);

    // Step 5: Set Session Privilege Level
    memcpy(&IPMI_SET_SESS_PRIV[5], sequence, 4);
    memcpy(&IPMI_SET_SESS_PRIV[9], session_id, 4);
    
    response = ipmi.sendCommand(IPMI_SET_SESS_PRIV, sizeof(IPMI_SET_SESS_PRIV));
    if (!response.success) return -8;
    if (response.data[20] != 0x00) return -9;

    // Step 6: Get Sensor Reading
    incrementSequence(sequence);
    memcpy(&IPMI_GET_SENS_READ[5], sequence, 4);
    memcpy(&IPMI_GET_SENS_READ[9], session_id, 4);
    IPMI_GET_SENS_READ[20] = sensorId;
    
    response = ipmi.sendCommand(IPMI_GET_SENS_READ, sizeof(IPMI_GET_SENS_READ));
    if (!response.success) return -10;
    if (response.data[20] != 0x00) return -11;

    return response.data[21];
}

void updateIPMICPUTemp(SensorData& sensor) {
    // Initialize random number generator for sequence generation
    randomSeed(analogRead(0));

    // Read temperature from sensor ID 0x33 (CPU temperature)
    int16_t temp = readIPMISensor(0x33);

    if (temp < 0) {
        Serial.print("\nError: ");
        Serial.println(temp);
    } else {
        sensor.value = temp;
        sensor.lastUpdate = millis();
        sensor.isValid = true;
        Serial.printf("CPU Temp: %d C\n", temp);
    }
}
