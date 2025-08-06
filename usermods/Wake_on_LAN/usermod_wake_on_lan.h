#pragma once

#include "wled.h"

#ifndef USERMOD_WAKE_ON_LAN_H
#define USERMOD_WAKE_ON_LAN_H

class UsermodWakeOnLAN : public Usermod {
  private:
    // Default settings
    bool enabled = true;
    bool initDone = false;
    bool triggered = false;
    unsigned long lastWOL = 0;
    unsigned long retryDelay = 30000;        // 30 seconds between retries
    unsigned long timeoutDuration = 300000;  // 5 minutes total timeout
    bool sendOnWifiConnect = true;           // Send WOL when WiFi connects
    bool periodicRetry = true;               // Enable periodic retries
    
    // MAC address storage (6 bytes)
    uint8_t targetMAC[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    // UDP for sending WOL packets
    WiFiUDP udp;
    
    // String constants for config
    static const char _name[];
    static const char _enabled[];
    static const char _targetMAC[];
    static const char _retryDelay[];
    static const char _timeoutDuration[];
    static const char _sendOnWifiConnect[];
    static const char _periodicRetry[];
    
    /**
     * Send a Wake-on-LAN magic packet
     */
    void sendWOLPacket() {
      if (!WiFi.isConnected()) {
        DEBUG_PRINTLN(F("WOL: WiFi not connected, cannot send packet"));
        return;
      }
      
      // Check if MAC address is set (not all zeros)
      bool macIsSet = false;
      for (int i = 0; i < 6; i++) {
        if (targetMAC[i] != 0) {
          macIsSet = true;
          break;
        }
      }
      
      if (!macIsSet) {
        DEBUG_PRINTLN(F("WOL: Target MAC address not configured"));
        return;
      }
      
      // Create WOL magic packet
      uint8_t packet[102];
      
      // First 6 bytes are 0xFF
      for (int i = 0; i < 6; i++) {
        packet[i] = 0xFF;
      }
      
      // Next 96 bytes are the MAC address repeated 16 times
      for (int i = 6; i < 102; i++) {
        packet[i] = targetMAC[(i - 6) % 6];
      }
      
      // Calculate broadcast address
      #ifdef ESP8266
      IPAddress broadcastIP = WiFi.localIP();
      broadcastIP[0] |= (~WiFi.subnetMask()[0]);
      broadcastIP[1] |= (~WiFi.subnetMask()[1]);
      broadcastIP[2] |= (~WiFi.subnetMask()[2]);
      broadcastIP[3] |= (~WiFi.subnetMask()[3]);
      #else
      IPAddress broadcastIP = WiFi.localIP() | (~WiFi.subnetMask());
      #endif
      
      // Send the packet
      if (udp.beginPacket(broadcastIP, 9)) {
        size_t written = udp.write(packet, 102);
        if (udp.endPacket() && written == 102) {
          lastWOL = millis();
          DEBUG_PRINTF("WOL: Magic packet sent to %02X:%02X:%02X:%02X:%02X:%02X via %s:9\n",
                      targetMAC[0], targetMAC[1], targetMAC[2], 
                      targetMAC[3], targetMAC[4], targetMAC[5],
                      broadcastIP.toString().c_str());
        } else {
          DEBUG_PRINTLN(F("WOL: Failed to send packet"));
        }
      } else {
        DEBUG_PRINTLN(F("WOL: Failed to begin UDP packet"));
      }
    }
    
    /**
     * Parse MAC address from string format (e.g., "AA:BB:CC:DD:EE:FF")
     */
    bool parseMACAddress(const char* macStr) {
      if (!macStr || strlen(macStr) < 17) return false;
      
      uint8_t tempMAC[6];
      int values[6];
      
      if (sscanf(macStr, "%x:%x:%x:%x:%x:%x", 
                 &values[0], &values[1], &values[2], 
                 &values[3], &values[4], &values[5]) == 6) {
        for (int i = 0; i < 6; i++) {
          tempMAC[i] = (uint8_t)values[i];
        }
        memcpy(targetMAC, tempMAC, 6);
        return true;
      }
      return false;
    }
    
    /**
     * Convert MAC address to string format
     */
    String getMACString() {
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               targetMAC[0], targetMAC[1], targetMAC[2],
               targetMAC[3], targetMAC[4], targetMAC[5]);
      return String(macStr);
    }

  public:
    /**
     * Setup function called once at boot
     */
    void setup() override {
      DEBUG_PRINTLN(F("WOL: Usermod initializing..."));
      initDone = true;
    }
    
    /**
     * Called when WiFi is connected
     */
    void connected() override {
      if (!enabled || !initDone) return;
      
      DEBUG_PRINTLN(F("WOL: WiFi connected"));
      
      if (sendOnWifiConnect && !triggered) {
        triggered = true;
        sendWOLPacket();
      }
    }
    
    /**
     * Main loop function
     */
    void loop() override {
      if (!enabled || !initDone || !WiFi.isConnected()) return;
      
      // Handle periodic retries
      if (periodicRetry && triggered && 
          (millis() - lastWOL >= retryDelay) && 
          (millis() <= timeoutDuration)) {
        sendWOLPacket();
      }
    }
    
    /**
     * Add custom info to JSON info response
     */
    void addToJsonInfo(JsonObject& root) override {
      if (!initDone) return;
      
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");
      
      JsonArray wolInfo = user.createNestedArray(FPSTR(_name));
      if (enabled) {
        wolInfo.add(F("Target: "));
        wolInfo.add(getMACString());
      } else {
        wolInfo.add(F("Disabled"));
      }
    }
    
    /**
     * Add usermod state to JSON state
     */
    void addToJsonState(JsonObject& root) override {
      if (!initDone || !enabled) return;
      
      JsonObject usermod = root[FPSTR(_name)];
      if (usermod.isNull()) usermod = root.createNestedObject(FPSTR(_name));
      
      usermod["enabled"] = enabled;
      usermod["mac"] = getMACString();
      usermod["lastSent"] = lastWOL;
    }
    
    /**
     * Read usermod state from JSON
     */
    void readFromJsonState(JsonObject& root) override {
      if (!initDone) return;
      
      JsonObject usermod = root[FPSTR(_name)];
      if (!usermod.isNull()) {
        if (usermod["wol"].as<bool>()) {
          // Manual trigger
          triggered = true;
          sendWOLPacket();
        }
      }
    }
    
    /**
     * Add configuration options to settings
     */
    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_targetMAC)] = getMACString();
      top[FPSTR(_retryDelay)] = retryDelay / 1000; // Convert to seconds for UI
      top[FPSTR(_timeoutDuration)] = timeoutDuration / 1000; // Convert to seconds for UI
      top[FPSTR(_sendOnWifiConnect)] = sendOnWifiConnect;
      top[FPSTR(_periodicRetry)] = periodicRetry;
    }
    
    /**
     * Read configuration from settings
     */
    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      
      bool configComplete = !top.isNull();
      
      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
      
      // Read MAC address
      String macString = top[FPSTR(_targetMAC)] | getMACString();
      parseMACAddress(macString.c_str());
      
      // Read timing values (convert from seconds to milliseconds)
      uint32_t retryDelaySeconds = top[FPSTR(_retryDelay)] | (retryDelay / 1000);
      retryDelay = retryDelaySeconds * 1000;
      
      uint32_t timeoutSeconds = top[FPSTR(_timeoutDuration)] | (timeoutDuration / 1000);
      timeoutDuration = timeoutSeconds * 1000;
      
      configComplete &= getJsonValue(top[FPSTR(_sendOnWifiConnect)], sendOnWifiConnect);
      configComplete &= getJsonValue(top[FPSTR(_periodicRetry)], periodicRetry);
      
      return configComplete;
    }
    
    /**
     * Add configuration metadata for the settings page
     */
    void appendConfigData() override {
      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_targetMAC)).c_str());
      oappend(F("',1,'Target device MAC address (format: AA:BB:CC:DD:EE:FF)');"));
      
      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_retryDelay)).c_str());
      oappend(F("',1,'Seconds between retry attempts');"));
      
      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_timeoutDuration)).c_str());
      oappend(F("',1,'Total timeout duration in seconds');"));
    }

#ifndef WLED_DISABLE_MQTT
    /**
     * Handle MQTT messages
     */
    bool onMqttMessage(char* topic, char* payload) override {
      if (!enabled || !initDone) return false;
      
      if (strlen(topic) == 4 && strncmp_P(topic, PSTR("/wol"), 4) == 0) {
        String action = payload;
        if (action == "send" || action == "wake" || action == "1") {
          triggered = true;
          sendWOLPacket();
          return true;
        }
      }
      return false;
    }
#endif
    
    /**
     * Get usermod ID
     */
    uint16_t getId() override {
      return USERMOD_ID_WAKE_ON_LAN;
    }
};

// String constants defined here to save flash memory
const char UsermodWakeOnLAN::_name[] PROGMEM = "WakeOnLAN";
const char UsermodWakeOnLAN::_enabled[] PROGMEM = "enabled";
const char UsermodWakeOnLAN::_targetMAC[] PROGMEM = "targetMAC";
const char UsermodWakeOnLAN::_retryDelay[] PROGMEM = "retryDelay";
const char UsermodWakeOnLAN::_timeoutDuration[] PROGMEM = "timeoutDuration";
const char UsermodWakeOnLAN::_sendOnWifiConnect[] PROGMEM = "sendOnWifiConnect";
const char UsermodWakeOnLAN::_periodicRetry[] PROGMEM = "periodicRetry";

#endif // USERMOD_WAKE_ON_LAN_H
