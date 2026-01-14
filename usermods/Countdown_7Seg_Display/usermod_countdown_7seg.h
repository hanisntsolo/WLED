#pragma once

#include "wled.h"

#ifdef USERMOD_COUNTDOWN_7SEG

/*
 * Countdown 7-Segment Display Usermod for WLED
 * 
 * Drives a 4-digit, 7-segment, Common Anode LED display directly from ESP8266 GPIOs.
 * Shows a countdown timer (default: 90 days) using NTP time.
 * 
 * HARDWARE WIRING (Common Anode Display):
 * ========================================
 * Segments (active LOW - use 220-330Ω resistors):
 *   A → D1 (GPIO5)
 *   B → D2 (GPIO4)
 *   C → D3 (GPIO0)
 *   D → D5 (GPIO14)
 *   E → D6 (GPIO12)
 *   F → D7 (GPIO13)
 *   G → D8 (GPIO15) - IMPORTANT: Must be LOW at boot!
 *   DP → Not used
 * 
 * Digits (active HIGH - no resistors needed):
 *   DIG1 → D0 (GPIO16)
 *   DIG2 → RX (GPIO3)
 *   DIG3 → TX (GPIO1)
 *   DIG4 → SD3 (GPIO10) - Alternative to GPIO3
 * 
 * Note: RX/TX used as GPIO - Serial logging disabled after boot.
 * Note: D4 (GPIO2) is reserved for WLED LED strip data.
 * 
 * SEGMENT LAYOUT:
 *       A
 *      ---
 *   F |   | B
 *      -G-
 *   E |   | C
 *      ---
 *       D
 */

class UsermodCountdown7Seg : public Usermod {
  private:
    // ==================== Configuration ====================
    bool enabled = true;
    bool initDone = false;
    
    // Countdown configuration
    uint32_t countdownDays = 90;           // Default countdown days
    uint32_t targetTimestamp = 0;          // Target epoch timestamp (Unix time)
    bool countdownActive = true;           // Is countdown running?
    
    // Display settings
    uint8_t brightness = 100;              // Display brightness (0-255, affects duty cycle)
    bool leadingZeros = true;              // Show leading zeros (0090 vs 90)
    bool blinkOnLow = true;                // Blink when countdown is low
    uint8_t blinkThresholdDays = 7;        // Days threshold for fast blink
    uint8_t slowBlinkThresholdDays = 30;   // Days threshold for slow blink
    bool commonCathode = false;            // true = Common Cathode, false = Common Anode
    bool testMode = false;                 // Show test pattern "1234" for debugging
    
    // ==================== GPIO Pin Definitions ====================
    // Segments (directly on ESP8266 GPIOs)
    // NOTE: For ESP8266, these are the default mappings
    #ifdef ESP8266
    static const uint8_t PIN_SEG_A = 13;   // D7 (swapped with F)
    static const uint8_t PIN_SEG_B = 4;    // D2
    static const uint8_t PIN_SEG_C = 0;    // D3
    static const uint8_t PIN_SEG_D = 14;   // D5
    static const uint8_t PIN_SEG_E = 12;   // D6
    static const uint8_t PIN_SEG_F = 5;    // D1 (swapped with A)
    static const uint8_t PIN_SEG_G = 15;   // D8 - Must be HIGH at boot!
    
    // Digits (active HIGH for common anode)
    static const uint8_t PIN_DIG_1 = 16;   // D0
    static const uint8_t PIN_DIG_2 = 3;    // RX (GPIO3)
    static const uint8_t PIN_DIG_3 = 1;    // TX (GPIO1)
    static const uint8_t PIN_DIG_4 = 10;   // SD3 (GPIO10) - Alternative to GPIO3
    #else
    // ESP32 defaults (can be configured via settings)
    uint8_t PIN_SEG_A = 5;
    uint8_t PIN_SEG_B = 4;
    uint8_t PIN_SEG_C = 0;
    uint8_t PIN_SEG_D = 14;
    uint8_t PIN_SEG_E = 12;
    uint8_t PIN_SEG_F = 13;
    uint8_t PIN_SEG_G = 15;
    uint8_t PIN_DIG_1 = 16;
    uint8_t PIN_DIG_2 = 17;
    uint8_t PIN_DIG_3 = 18;
    uint8_t PIN_DIG_4 = 19;
    #endif
    
    // ==================== Segment Lookup Table ====================
    // Segments: bit 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G
    // For Common Anode: 1 = segment ON (but we invert when writing)
    const uint8_t SEGMENT_MAP[10] = {
      0b00111111,  // 0: A,B,C,D,E,F
      0b00000110,  // 1: B,C
      0b01011011,  // 2: A,B,D,E,G
      0b01001111,  // 3: A,B,C,D,G
      0b01100110,  // 4: B,C,F,G
      0b01101101,  // 5: A,C,D,F,G
      0b01111101,  // 6: A,C,D,E,F,G
      0b00000111,  // 7: A,B,C
      0b01111111,  // 8: A,B,C,D,E,F,G
      0b01101111   // 9: A,B,C,D,F,G
    };
    const uint8_t SEGMENT_BLANK = 0b00000000;  // All segments off
    const uint8_t SEGMENT_DASH  = 0b01000000;  // Just G segment (dash)
    
    // ==================== Runtime State ====================
    uint8_t currentDigit = 0;              // Which digit we're currently displaying (0-3)
    unsigned long lastMultiplexTime = 0;   // Last time we switched digits
    unsigned long lastCountdownUpdate = 0; // Last time we updated countdown value
    uint16_t displayValue = 0;             // Current value to display (0-9999)
    uint8_t digitValues[4] = {0, 0, 0, 0}; // Individual digit values
    bool displayOn = true;                 // For blinking effect
    unsigned long lastBlinkTime = 0;       // Last blink toggle
    unsigned long lastDebugPrint = 0;      // For periodic debug prints
    
    // Timing constants
    static const unsigned long MULTIPLEX_INTERVAL_US = 2500;  // 2.5ms per digit = 100Hz refresh
    static const unsigned long COUNTDOWN_UPDATE_MS = 1000;    // Update countdown every second
    static const unsigned long SLOW_BLINK_MS = 1000;          // Slow blink period
    static const unsigned long FAST_BLINK_MS = 250;           // Fast blink period
    
    // String constants for config
    static const char _name[];
    static const char _enabled[];
    static const char _countdownDays[];
    static const char _targetTimestamp[];
    static const char _brightness[];
    static const char _leadingZeros[];
    static const char _blinkOnLow[];
    static const char _blinkThreshold[];
    static const char _slowBlinkThreshold[];
    static const char _commonCathode[];
    static const char _testMode[]; 
    
    // ==================== Private Methods ====================
    
    /**
     * Initialize GPIO pins for the 7-segment display
     */
    void initGPIO() {
      // CRITICAL: Set D8 (GPIO15) first to ensure safe boot state
      // GPIO15 must be LOW during boot
      pinMode(PIN_SEG_G, OUTPUT);
      digitalWrite(PIN_SEG_G, commonCathode ? LOW : HIGH);  // Segment OFF
      
      // Initialize segment pins as OUTPUT, all OFF
      // Common Anode: HIGH = OFF, Common Cathode: LOW = OFF
      uint8_t segOff = commonCathode ? LOW : HIGH;
      pinMode(PIN_SEG_A, OUTPUT); digitalWrite(PIN_SEG_A, segOff);
      pinMode(PIN_SEG_B, OUTPUT); digitalWrite(PIN_SEG_B, segOff);
      pinMode(PIN_SEG_C, OUTPUT); digitalWrite(PIN_SEG_C, segOff);
      pinMode(PIN_SEG_D, OUTPUT); digitalWrite(PIN_SEG_D, segOff);
      pinMode(PIN_SEG_E, OUTPUT); digitalWrite(PIN_SEG_E, segOff);
      pinMode(PIN_SEG_F, OUTPUT); digitalWrite(PIN_SEG_F, segOff);
      // PIN_SEG_G already set above
      
      // Initialize digit pins as OUTPUT, all OFF
      // Common Anode: LOW = OFF, Common Cathode: HIGH = OFF
      uint8_t digOff = commonCathode ? HIGH : LOW;
      pinMode(PIN_DIG_1, OUTPUT); digitalWrite(PIN_DIG_1, digOff);
      pinMode(PIN_DIG_2, OUTPUT); digitalWrite(PIN_DIG_2, digOff);
      pinMode(PIN_DIG_3, OUTPUT); digitalWrite(PIN_DIG_3, digOff);
      pinMode(PIN_DIG_4, OUTPUT); digitalWrite(PIN_DIG_4, digOff);
      
      DEBUG_PRINTF("Countdown7Seg: GPIO initialized (Common %s)\n", commonCathode ? "Cathode" : "Anode");
    }
    
    /**
     * Turn off all digits (for multiplexing transitions)
     */
    void allDigitsOff() {
      // Common Anode: LOW = OFF, Common Cathode: HIGH = OFF
      uint8_t digOff = commonCathode ? HIGH : LOW;
      digitalWrite(PIN_DIG_1, digOff);
      digitalWrite(PIN_DIG_2, digOff);
      digitalWrite(PIN_DIG_3, digOff);
      digitalWrite(PIN_DIG_4, digOff);
    }
    
    /**
     * Turn off all segments
     */
    void allSegmentsOff() {
      // Common Anode: HIGH = OFF, Common Cathode: LOW = OFF
      uint8_t segOff = commonCathode ? LOW : HIGH;
      digitalWrite(PIN_SEG_A, segOff);
      digitalWrite(PIN_SEG_B, segOff);
      digitalWrite(PIN_SEG_C, segOff);
      digitalWrite(PIN_SEG_D, segOff);
      digitalWrite(PIN_SEG_E, segOff);
      digitalWrite(PIN_SEG_F, segOff);
      digitalWrite(PIN_SEG_G, segOff);
    }
    
    /**
     * Set segments for a specific digit value (0-9), blank (10), or dash (11)
     */
    void setSegments(uint8_t value) {
      uint8_t segments;
      if (value < 10) {
        segments = SEGMENT_MAP[value];
      } else if (value == 11) {
        segments = SEGMENT_DASH;  // Show dash for "syncing" state
      } else {
        segments = SEGMENT_BLANK; // Blank for value 10 or any other
      }
      
      // Common Anode: LOW = ON, HIGH = OFF
      // Common Cathode: HIGH = ON, LOW = OFF
      uint8_t segOn = commonCathode ? HIGH : LOW;
      uint8_t segOff = commonCathode ? LOW : HIGH;
      
      digitalWrite(PIN_SEG_A, (segments & 0b00000001) ? segOn : segOff);
      digitalWrite(PIN_SEG_B, (segments & 0b00000010) ? segOn : segOff);
      digitalWrite(PIN_SEG_C, (segments & 0b00000100) ? segOn : segOff);
      digitalWrite(PIN_SEG_D, (segments & 0b00001000) ? segOn : segOff);
      digitalWrite(PIN_SEG_E, (segments & 0b00010000) ? segOn : segOff);
      digitalWrite(PIN_SEG_F, (segments & 0b00100000) ? segOn : segOff);
      digitalWrite(PIN_SEG_G, (segments & 0b01000000) ? segOn : segOff);
    }
    
    /**
     * Enable a specific digit (0-3)
     */
    void enableDigit(uint8_t digit) {
      // Common Anode: HIGH = ON, Common Cathode: LOW = ON
      uint8_t digOn = commonCathode ? LOW : HIGH;
      switch (digit) {
        case 0: digitalWrite(PIN_DIG_1, digOn); break;
        case 1: digitalWrite(PIN_DIG_2, digOn); break;
        case 2: digitalWrite(PIN_DIG_3, digOn); break;
        case 3: digitalWrite(PIN_DIG_4, digOn); break;
      }
    }
    
    /**
     * Update the digit values array from displayValue
     */
    void updateDigitValues() {
      digitValues[0] = (displayValue / 1000) % 10;  // Thousands
      digitValues[1] = (displayValue / 100) % 10;   // Hundreds
      digitValues[2] = (displayValue / 10) % 10;    // Tens
      digitValues[3] = displayValue % 10;           // Ones
      
      // Handle leading zeros
      if (!leadingZeros) {
        if (displayValue < 1000) digitValues[0] = 10;  // Blank
        if (displayValue < 100)  digitValues[1] = 10;  // Blank
        if (displayValue < 10)   digitValues[2] = 10;  // Blank
      }
    }
    
    /**
     * Calculate days remaining from target timestamp
     */
    int32_t calculateDaysRemaining() {
      if (targetTimestamp == 0 || !ntpEnabled) {
        DEBUG_PRINTF("Countdown7Seg: calcDays SKIP - target=%lu, ntpEnabled=%d\n", targetTimestamp, ntpEnabled);
        return -1;  // Not configured or NTP not available
      }
      
      time_t now = toki.second();  // Get current epoch time from WLED's time system
      if (now < 1600000000) {
        DEBUG_PRINTF("Countdown7Seg: calcDays SKIP - NTP not synced (now=%lu)\n", (unsigned long)now);
        return -1;  // Time not yet synced (before Sept 2020)
      }
      
      int32_t secondsRemaining = targetTimestamp - now;
      if (secondsRemaining <= 0) {
        DEBUG_PRINTF("Countdown7Seg: calcDays=0 (expired) target=%lu, now=%lu\n", targetTimestamp, (unsigned long)now);
        return 0;
      }
      
      int32_t days = secondsRemaining / 86400;
      DEBUG_PRINTF("Countdown7Seg: calcDays=%ld (target=%lu, now=%lu, secRem=%ld)\n", days, targetTimestamp, (unsigned long)now, secondsRemaining);
      return days;  // Convert to days
    }
    
    /**
     * Update the countdown display value
     */
    void updateCountdown() {
      int32_t daysRemaining = calculateDaysRemaining();
      
      if (daysRemaining < 0) {
        // NTP not synced or not configured - show dashes "----"
        displayValue = 0;
        digitValues[0] = 11;  // Dash
        digitValues[1] = 11;  // Dash
        digitValues[2] = 11;  // Dash
        digitValues[3] = 11;  // Dash
        DEBUG_PRINTLN(F("Countdown7Seg: updateCountdown -> showing dashes (NTP not ready)"));
      } else if (daysRemaining > 9999) {
        displayValue = 9999;
        updateDigitValues();
        DEBUG_PRINTLN(F("Countdown7Seg: updateCountdown -> capped at 9999"));
      } else {
        displayValue = daysRemaining;
        updateDigitValues();
        DEBUG_PRINTF("Countdown7Seg: updateCountdown -> display=%u digits=[%u,%u,%u,%u]\n", 
                     displayValue, digitValues[0], digitValues[1], digitValues[2], digitValues[3]);
      }
    }
    
    /**
     * Handle blinking effect based on countdown value
     */
    void handleBlinking() {
      if (!blinkOnLow) {
        displayOn = true;
        return;
      }
      
      int32_t daysRemaining = calculateDaysRemaining();
      unsigned long blinkPeriod = 0;
      
      if (daysRemaining >= 0 && daysRemaining < blinkThresholdDays) {
        blinkPeriod = FAST_BLINK_MS;
      } else if (daysRemaining >= 0 && daysRemaining < slowBlinkThresholdDays) {
        blinkPeriod = SLOW_BLINK_MS;
      }
      
      if (blinkPeriod > 0) {
        if (millis() - lastBlinkTime >= blinkPeriod) {
          displayOn = !displayOn;
          lastBlinkTime = millis();
        }
      } else {
        displayOn = true;
      }
    }
    
    /**
     * Reset countdown to specified number of days from now
     */
    void resetCountdown(uint32_t days) {
      countdownDays = days;
      time_t now = toki.second();
      if (now > 1600000000) {  // Time is synced
        targetTimestamp = now + (days * 86400);
        DEBUG_PRINTF("Countdown7Seg: Reset to %u days, target=%lu\n", days, targetTimestamp);
      }
    }
    
    /**
     * Perform one multiplex cycle - display one digit
     * Called from loop() as frequently as possible
     */
    void multiplexDisplay() {
      // Check if it's time to switch to next digit
      unsigned long now = micros();
      if (now - lastMultiplexTime < MULTIPLEX_INTERVAL_US) {
        return;
      }
      lastMultiplexTime = now;
      
      // CRITICAL: Turn off current digit FIRST (prevents ghosting)
      allDigitsOff();
      
      // Small delay to ensure digit is fully off before changing segments
      // This prevents ghosting/bleeding between digits
      delayMicroseconds(50);
      
      // Turn off all segments while switching
      allSegmentsOff();
      
      // If display is off (blinking), keep all digits off
      if (!displayOn) {
        return;
      }
      
      // Move to next digit
      currentDigit = (currentDigit + 1) % 4;
      
      // Get the value for this digit
      uint8_t value;
      if (testMode) {
        // Test pattern: show 1, 2, 3, 4
        value = currentDigit + 1;
      } else {
        value = digitValues[currentDigit];
      }
      
      // Optional debug print (once per second) when in test mode
      uint8_t segments = (value < 10) ? SEGMENT_MAP[value] : SEGMENT_BLANK;
      if (testMode && (millis() - lastDebugPrint >= 1000)) {
        DEBUG_PRINTF("Countdown7Seg: test digit=%u value=%u segs=0x%02X\n", currentDigit, value, segments);
        lastDebugPrint = millis();
      }
      
      // Set segments for this digit BEFORE enabling the digit
      setSegments(value);
      
      // Small delay to let segments settle
      delayMicroseconds(50);
      
      // NOW enable this digit
      enableDigit(currentDigit);
    }

  public:
    /**
     * Setup function called once at boot
     */
    void setup() override {
      DEBUG_PRINTLN(F("Countdown7Seg: Initializing..."));
      
      initGPIO();
      
      // Initialize target timestamp if not set
      if (targetTimestamp == 0 && countdownDays > 0) {
        // Will be set properly once NTP syncs
        time_t now = toki.second();
        if (now > 1600000000) {
          targetTimestamp = now + (countdownDays * 86400);
        }
      }
      
      updateCountdown();
      initDone = true;
      
      DEBUG_PRINTLN(F("Countdown7Seg: Initialization complete"));
    }
    
    /**
     * Called when WiFi is connected
     */
    void connected() override {
      // Send multiple debug beacons to ensure at least one gets through
      DEBUG_PRINTLN(F(""));
      DEBUG_PRINTLN(F("========================================"));
      DEBUG_PRINTLN(F("  WLED DEBUG BEACON - WiFi CONNECTED!"));
      DEBUG_PRINTLN(F("  Countdown7Seg Usermod Active"));
      DEBUG_PRINTLN(F("========================================"));
      DEBUG_PRINTLN(F(""));
      
      // If target timestamp is not set and we have NTP time, set it
      if (targetTimestamp == 0 && countdownDays > 0) {
        time_t now = toki.second();
        if (now > 1600000000) {
          targetTimestamp = now + (countdownDays * 86400);
          DEBUG_PRINTF("Countdown7Seg: Target set to %u\n", targetTimestamp);
        }
      }
      
      // Print current config for debugging
      DEBUG_PRINTF("Countdown7Seg: Config - days=%u, target=%u, testMode=%d, enabled=%d\n", 
                   countdownDays, targetTimestamp, testMode, enabled);
    }
    
    /**
     * Main loop function - called continuously
     * MUST be non-blocking!
     */
    void loop() override {
      if (!enabled || !initDone) return;
      
      // Always do multiplexing - this is time-critical
      multiplexDisplay();
      
      // Update countdown value periodically (not every loop)
      if (millis() - lastCountdownUpdate >= COUNTDOWN_UPDATE_MS) {
        lastCountdownUpdate = millis();
        
        // Periodic debug status (every 10 seconds)
        static unsigned long lastStatusPrint = 0;
        if (millis() - lastStatusPrint >= 10000) {
          lastStatusPrint = millis();
          DEBUG_PRINTF("Countdown7Seg: STATUS testMode=%d, enabled=%d, target=%lu, days=%u, display=%u\n",
                       testMode, enabled, targetTimestamp, countdownDays, displayValue);
          DEBUG_PRINTF("Countdown7Seg: STATUS digits=[%u,%u,%u,%u], ntpEnabled=%d, ntpTime=%lu\n",
                       digitValues[0], digitValues[1], digitValues[2], digitValues[3], 
                       ntpEnabled, (unsigned long)toki.second());
        }
        
        // Auto-set target timestamp once NTP syncs (if not already set)
        if (targetTimestamp == 0 && countdownDays > 0) {
          time_t now = toki.second();
          if (now > 1600000000) {  // NTP synced (after Sept 2020)
            targetTimestamp = now + (countdownDays * 86400);
            DEBUG_PRINTF("Countdown7Seg: Auto-set target to %lu (%u days)\n", targetTimestamp, countdownDays);
          }
        }
        
        // Skip countdown update if in test mode
        if (!testMode) {
          updateCountdown();
        }
        handleBlinking();
      }
    }
    
    /**
     * Add custom info to JSON info response
     */
    void addToJsonInfo(JsonObject& root) override {
      if (!initDone) return;
      
      JsonObject user = root["u"];
      if (user.isNull()) user = root.createNestedObject("u");
      
      int32_t daysRemaining = calculateDaysRemaining();
      
      JsonArray countdownInfo = user.createNestedArray(F("Countdown"));
      if (enabled) {
        if (daysRemaining >= 0) {
          countdownInfo.add(daysRemaining);
          countdownInfo.add(F(" days"));
        } else {
          countdownInfo.add(F("Syncing..."));
        }
      } else {
        countdownInfo.add(F("Disabled"));
      }
    }
    
    /**
     * Add usermod state to JSON state
     */
    void addToJsonState(JsonObject& root) override {
      if (!initDone) return;
      
      JsonObject usermod = root[FPSTR(_name)];
      if (usermod.isNull()) usermod = root.createNestedObject(FPSTR(_name));
      
      usermod["enabled"] = enabled;
      usermod["daysRemaining"] = calculateDaysRemaining();
      usermod["targetTimestamp"] = targetTimestamp;
      usermod["displayValue"] = displayValue;
    }
    
    /**
     * Read usermod state from JSON
     */
    void readFromJsonState(JsonObject& root) override {
      if (!initDone) return;
      
      JsonObject usermod = root[FPSTR(_name)];
      if (!usermod.isNull()) {
        // Check for reset command
        if (usermod.containsKey("reset")) {
          uint32_t days = usermod["reset"] | countdownDays;
          resetCountdown(days);
        }
        
        // Check for manual target timestamp
        if (usermod.containsKey("targetTimestamp")) {
          targetTimestamp = usermod["targetTimestamp"];
        }
        
        // Check for enable/disable
        if (usermod.containsKey("enabled")) {
          enabled = usermod["enabled"];
        }
        
        // Check for test mode toggle
        if (usermod.containsKey("testMode")) {
          testMode = usermod["testMode"];
        }
      }
    }
    
    /**
     * Add configuration options to settings
     */
    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_countdownDays)] = countdownDays;
      top[FPSTR(_targetTimestamp)] = targetTimestamp;
      top[FPSTR(_brightness)] = brightness;
      top[FPSTR(_leadingZeros)] = leadingZeros;
      top[FPSTR(_blinkOnLow)] = blinkOnLow;
      top[FPSTR(_blinkThreshold)] = blinkThresholdDays;
      top[FPSTR(_slowBlinkThreshold)] = slowBlinkThresholdDays;
      top[FPSTR(_commonCathode)] = commonCathode;
      top[FPSTR(_testMode)] = testMode;
    }
    
    /**
     * Read configuration from settings
     */
    bool readFromConfig(JsonObject& root) override {
      JsonObject top = root[FPSTR(_name)];
      
      bool configComplete = !top.isNull();
      
      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
      configComplete &= getJsonValue(top[FPSTR(_countdownDays)], countdownDays);
      configComplete &= getJsonValue(top[FPSTR(_targetTimestamp)], targetTimestamp);
      configComplete &= getJsonValue(top[FPSTR(_brightness)], brightness);
      configComplete &= getJsonValue(top[FPSTR(_leadingZeros)], leadingZeros);
      configComplete &= getJsonValue(top[FPSTR(_blinkOnLow)], blinkOnLow);
      configComplete &= getJsonValue(top[FPSTR(_blinkThreshold)], blinkThresholdDays);
      configComplete &= getJsonValue(top[FPSTR(_slowBlinkThreshold)], slowBlinkThresholdDays);
      configComplete &= getJsonValue(top[FPSTR(_commonCathode)], commonCathode);
      configComplete &= getJsonValue(top[FPSTR(_testMode)], testMode);
      
      // Re-init GPIO if config changed
      if (initDone) {
        initGPIO();
      }
      
      return configComplete;
    }
    
    /**
     * Add configuration metadata for the settings page
     */
    void appendConfigData() override {
      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_countdownDays)).c_str());
      oappend(F("',1,'Initial countdown in days (used on first boot)');"));
      
      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_targetTimestamp)).c_str());
      oappend(F("',1,'Target Unix timestamp (auto-set from countdown days)');"));
      
      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_blinkThreshold)).c_str());
      oappend(F("',1,'Days threshold for fast blinking');"));
      
      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_slowBlinkThreshold)).c_str());
      oappend(F("',1,'Days threshold for slow blinking');"));

      oappend(F("addInfo('"));
      oappend(String(FPSTR(_name)).c_str());
      oappend(F(":"));
      oappend(String(FPSTR(_testMode)).c_str());
      oappend(F("',1,'Show test pattern \"1234\" for debugging');"));
    }
    
    /**
     * Handle HTTP API requests
     * Endpoint: /countdown/reset?days=90
     */
    bool handleButton(uint8_t b) override {
      // Not using button handling for this usermod
      return false;
    }

#ifndef WLED_DISABLE_MQTT
    /**
     * Handle MQTT messages
     */
    bool onMqttMessage(char* topic, char* payload) override {
      if (!enabled || !initDone) return false;
      
      // Handle /countdown/reset topic
      if (strlen(topic) >= 10 && strncmp_P(topic, PSTR("/countdown"), 10) == 0) {
        char* subTopic = topic + 10;
        
        if (strcmp_P(subTopic, PSTR("/reset")) == 0) {
          uint32_t days = atoi(payload);
          if (days == 0) days = countdownDays;
          resetCountdown(days);
          return true;
        }
        
        if (strcmp_P(subTopic, PSTR("/enable")) == 0) {
          enabled = (strcmp(payload, "1") == 0 || strcmp_P(payload, PSTR("true")) == 0);
          return true;
        }
      }
      
      return false;
    }
    
    /**
     * Called when MQTT connects
     */
    void onMqttConnect(bool sessionPresent) override {
      if (!enabled) return;
      
      char subBuffer[48];
      if (mqttDeviceTopic[0] != 0) {
        snprintf_P(subBuffer, sizeof(subBuffer), PSTR("%s/countdown/#"), mqttDeviceTopic);
        mqtt->subscribe(subBuffer, 0);
      }
    }
#endif
    
    /**
     * Get usermod ID
     */
    uint16_t getId() override {
      return USERMOD_ID_COUNTDOWN_7SEG;
    }
};

// String constants defined here to save flash memory
const char UsermodCountdown7Seg::_name[] PROGMEM = "Countdown7Seg";
const char UsermodCountdown7Seg::_enabled[] PROGMEM = "enabled";
const char UsermodCountdown7Seg::_countdownDays[] PROGMEM = "countdownDays";
const char UsermodCountdown7Seg::_targetTimestamp[] PROGMEM = "targetTimestamp";
const char UsermodCountdown7Seg::_brightness[] PROGMEM = "brightness";
const char UsermodCountdown7Seg::_leadingZeros[] PROGMEM = "leadingZeros";
const char UsermodCountdown7Seg::_blinkOnLow[] PROGMEM = "blinkOnLow";
const char UsermodCountdown7Seg::_blinkThreshold[] PROGMEM = "blinkThreshold";
const char UsermodCountdown7Seg::_slowBlinkThreshold[] PROGMEM = "slowBlinkThreshold";
const char UsermodCountdown7Seg::_commonCathode[] PROGMEM = "commonCathode";
const char UsermodCountdown7Seg::_testMode[] PROGMEM = "testMode";

#endif // USERMOD_COUNTDOWN_7SEG
