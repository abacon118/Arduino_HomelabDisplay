/*
 * IPMI Communication Library
 * 
 * Handles low-level UDP communication for IPMI protocol.
 * Provides session management and command transmission functionality.
 */

#include "IPMIComm.h"

bool IPMISession::begin(IPAddress serverAddr, bool debugFlag) {
    serverIP = serverAddr;
    
    // Initialize UDP connection
    if (!udp.begin(localPort)) {
        Serial.println("Failed to start UDP");
        return false;
    }

    debug = debugFlag;
    
    if (debug) {
        Serial.print("UDP connected to: ");
        Serial.println(serverIP.toString());
    }

    return true;
}

void IPMISession::logBytes(const char* direction, const uint8_t* data, size_t length) {
    if (!debug) return;
    
    Serial.print(direction);
    Serial.print(" ");
    Serial.print(length);
    Serial.println(" bytes");
    
    // Format bytes in rows of 16
    for (size_t i = 0; i < length; i++) {
        if (i % 16 == 0) {
            Serial.print("  ");
        }
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
        if ((i + 1) % 16 == 0) {
            Serial.println();
        }
    }
    if (length % 16 != 0) {
        Serial.println();
    }
}

bool IPMISession::waitForResponse(unsigned long timeout) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (udp.parsePacket()) {
            return true;
        }
        yield();  // Allow ESP32 background tasks to run
    }

    return false;
}

IPMIResponse IPMISession::sendCommand(uint8_t* command, size_t length) {
    IPMIResponse response = {0};
    response.success = false;
    
    // Log outgoing command bytes
    logBytes("TX", command, length);

    // Send command via UDP
    udp.beginPacket(serverIP, serverPort);
    size_t written = udp.write(command, length);
    bool success = udp.endPacket();
    
    if (!success || written != length) {
        if (debug) {
            Serial.println("\nFailed to send command");
        }
        return response;
    }

    // Wait for and process response
    if (waitForResponse()) {
        response.length = udp.read(response.data, sizeof(response.data));
        logBytes("RX", response.data, response.length);
        response.success = true;
        return response;
    }

    if (debug) {
        Serial.println("\nNo response received within timeout");
    }
    return response;
}

void IPMISession::end() {
    udp.stop();
    
    if (debug) {
        Serial.println("\nUDP session ended");
    }
}

IPMISession::~IPMISession() {
    end();
}
