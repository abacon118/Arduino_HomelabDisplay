/*
 * IPMI Communication Library
 * 
 * Handles low-level UDP communication for IPMI protocol.
 * Provides session management and command transmission functionality.
 */

#ifndef IPMI_COMM_H
#define IPMI_COMM_H

#include <WiFiUdp.h>

// Structure to hold IPMI command responses
struct IPMIResponse {
    uint8_t data[255];
    size_t length;
    bool success;
};

class IPMISession {
private:
    WiFiUDP udp;
    IPAddress serverIP;
    const int serverPort = 623;    // Standard IPMI port
    const int localPort = 8888;    // Local port for responses
    bool debug;

    bool waitForResponse(unsigned long timeout = 500);
    void logBytes(const char* direction, const uint8_t* data, size_t length);

public:
    IPMISession() = default;
    ~IPMISession();
    
    bool begin(IPAddress serverIP, bool debug);
    IPMIResponse sendCommand(uint8_t* command, size_t length);
    void end();
};

#endif
