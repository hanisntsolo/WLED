# Countdown 7-Segment Display Usermod

This usermod drives a **4-digit, 7-segment, Common Anode LED display** directly from ESP8266 GPIOs to show a countdown timer. It's designed to coexist cleanly with WLED without interfering with LED strip operation.

## Features

- **90-day countdown timer** (configurable)
- **NTP-based timekeeping** - survives reboots
- **Non-blocking multiplexing** - no interference with WLED
- **Blinking alerts** at configurable thresholds
- **MQTT control** for remote reset
- **JSON API** for integration

## Hardware Requirements

### Display
- **Type:** 4-Digit 7-Segment Common Anode LED Display (12-pin)
- **Example:** Electrobot 4-Digit 7-Segment LED Display
- **Forward Voltage:** 1.8–2.2V (Red)
- **Forward Current:** 10–20mA per segment

### Wiring Diagram

```
     7-Segment Display (Front View)
     
        DIG1   DIG2   DIG3   DIG4
        ┌──┐   ┌──┐   ┌──┐   ┌──┐
        │88│   │88│   │88│   │88│
        └──┘   └──┘   └──┘   └──┘
        
     Segment Layout (each digit):
           ──A──
          │     │
          F     B
          │     │
           ──G──
          │     │
          E     C
          │     │
           ──D──   ○DP
```

### Pin Mapping

#### Datasheet Pin Reference (12-pin display)
| Pin | Function |
|-----|----------|
| 1   | E        |
| 2   | D        |
| 3   | DP       |
| 4   | C        |
| 5   | G        |
| 6   | DIG4     |
| 7   | B        |
| 8   | DIG3     |
| 9   | DIG2     |
| 10  | F        |
| 11  | A        |
| 12  | DIG1     |

#### ESP8266 GPIO Wiring

**Segments (with 220–330Ω resistors in series):**
| Segment | Display Pin | ESP8266 | GPIO |
|---------|-------------|---------|------|
| A       | 11          | D1      | 5    |
| B       | 7           | D2      | 4    |
| C       | 4           | D3      | 0    |
| D       | 2           | D5      | 14   |
| E       | 1           | D6      | 12   |
| F       | 10          | D7      | 13   |
| G       | 5           | D8      | 15   |
| DP      | 3           | (unused)|      |

**Digits (no resistors needed - directly connected):**
| Digit | Display Pin | ESP8266 | GPIO |
|-------|-------------|---------|------|
| DIG1  | 12          | D0      | 16   |
| DIG2  | 9           | RX      | 3    |
| DIG3  | 8           | TX      | 1    |
| DIG4  | 6           | SD3     | 10   |

### Important Notes

1. **D4 (GPIO2) is RESERVED** for WLED LED strip data - do not use!

2. **D8 (GPIO15) Boot Requirement:**
   - GPIO15 must be LOW during boot
   - The usermod sets it HIGH immediately after boot
   - This is handled automatically in the code

3. **RX/TX as GPIOs:**
   - Using RX (GPIO3) and TX (GPIO1) disables serial output
   - Serial logging will not work after boot
   - This is normal and expected

4. **Common Anode Logic:**
   - Digit pin HIGH = digit enabled
   - Segment pin LOW = segment ON
   - This is inverted from common cathode displays

### Schematic

```
ESP8266                    7-Segment Display (Common Anode)
                           
D1 (GPIO5)  ──[220Ω]──┬──  Segment A (Pin 11)
D2 (GPIO4)  ──[220Ω]──┼──  Segment B (Pin 7)
D3 (GPIO0)  ──[220Ω]──┼──  Segment C (Pin 4)
D5 (GPIO14) ──[220Ω]──┼──  Segment D (Pin 2)
D6 (GPIO12) ──[220Ω]──┼──  Segment E (Pin 1)
D7 (GPIO13) ──[220Ω]──┼──  Segment F (Pin 10)
D8 (GPIO15) ──[220Ω]──┴──  Segment G (Pin 5)

D0 (GPIO16) ─────────────  DIG1 (Pin 12)
RX (GPIO3)  ─────────────  DIG2 (Pin 9)
TX (GPIO1)  ─────────────  DIG3 (Pin 8)
SD3 (GPIO10)─────────────  DIG4 (Pin 6)

3.3V ────────────────────  Common Anode (via digit pins)
GND  ────────────────────  Common Ground
```

## Software Installation

### 1. Enable the Usermod

Add to your `platformio_override.ini`:

```ini
[platformio]
default_envs = esp8266_countdown

[env:esp8266_countdown]
extends = env:nodemcuv2
build_flags = ${env:nodemcuv2.build_flags}
  -D USERMOD_COUNTDOWN_7SEG
custom_usermods = Countdown_7Seg_Display
```

Or add to `my_config.h`:

```cpp
#define USERMOD_COUNTDOWN_7SEG
```

### 2. Build and Flash

```bash
pio run -e esp8266_countdown -t upload
```

## Configuration

Configuration is available in the WLED web UI under **Config → Usermods → Countdown7Seg**:

| Setting | Default | Description |
|---------|---------|-------------|
| enabled | true | Enable/disable the display |
| countdownDays | 90 | Initial countdown duration |
| targetTimestamp | 0 | Target Unix timestamp (auto-calculated) |
| brightness | 100 | Display brightness (0-255) |
| leadingZeros | true | Show leading zeros (0090 vs 90) |
| blinkOnLow | true | Enable blinking when countdown is low |
| blinkThreshold | 7 | Days for fast blink |
| slowBlinkThreshold | 30 | Days for slow blink |

## API

### JSON State API

**Read state:**
```bash
curl http://[WLED_IP]/json/state
```

Response includes:
```json
{
  "Countdown7Seg": {
    "enabled": true,
    "daysRemaining": 85,
    "targetTimestamp": 1736000000,
    "displayValue": 85
  }
}
```

**Reset countdown:**
```bash
curl -X POST http://[WLED_IP]/json/state \
  -H "Content-Type: application/json" \
  -d '{"Countdown7Seg": {"reset": 90}}'
```

**Set specific target:**
```bash
curl -X POST http://[WLED_IP]/json/state \
  -H "Content-Type: application/json" \
  -d '{"Countdown7Seg": {"targetTimestamp": 1736000000}}'
```

### MQTT

Subscribe to countdown topics:
- `[deviceTopic]/countdown/reset` - Send number of days to reset
- `[deviceTopic]/countdown/enable` - Send "1" or "0"

Example:
```bash
mosquitto_pub -t "wled/countdown/reset" -m "90"
```

## Troubleshooting

### Display shows nothing
1. Check all wiring connections
2. Verify resistors are on segment lines (not digit lines)
3. Ensure NTP is enabled and time is synced
4. Check WLED info page for "Countdown" status

### Display shows garbled numbers
1. Verify segment wiring matches the pin mapping
2. Check that you're using a **Common Anode** display
3. Test each segment individually

### Display flickers
1. Ensure no `delay()` calls in other usermods
2. Check WiFi signal strength
3. Reduce other heavy processing

### ESP8266 won't boot
1. Check D8 (GPIO15) - must not be pulled HIGH externally at boot
2. Remove any connections to D8 temporarily to test
3. The usermod handles D8 correctly after boot

### Time not syncing
1. Enable NTP in WLED settings
2. Set correct timezone
3. Ensure internet connectivity
4. Display will show "----" while waiting for NTP sync

## Integration with Wake-on-LAN

This usermod can work alongside the Wake-on-LAN usermod. Both share the ESP8266 and can be enabled simultaneously:

```ini
build_flags = 
  -D USERMOD_COUNTDOWN_7SEG
  -D USERMOD_WAKE_ON_LAN
custom_usermods = Countdown_7Seg_Display Wake_on_LAN
```

## Technical Details

### Multiplexing
- Refresh rate: ~100Hz (2.5ms per digit × 4 digits)
- Non-blocking implementation using `micros()`
- No `delay()` calls - fully cooperative

### Memory Usage
- ~2KB Flash
- ~100 bytes RAM

### Timing
- Countdown updates every 1 second
- NTP time used for accuracy across reboots

## License

MIT License - Same as WLED project.

## Author

Created as a WLED usermod for ESP8266-based countdown displays.
