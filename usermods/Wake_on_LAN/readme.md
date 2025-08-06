# Wake-on-LAN Usermod

This usermod enables your WLED device to send Wake-on-LAN (WOL) magic packets to wake up devices on your network when the WLED device connects to WiFi.

## Features

- Automatically sends WOL packet when WiFi connects
- Configurable target MAC address
- Periodic retry mechanism with timeout
- Manual triggering via JSON API, MQTT, or web interface
- Fully configurable through WLED's usermod settings page
- Debug output for troubleshooting

## Configuration Options

| Setting | Description | Default |
|---------|-------------|---------|
| `enabled` | Enable/disable the usermod | `true` |
| `targetMAC` | Target device MAC address (format: AA:BB:CC:DD:EE:FF) | `00:00:00:00:00:00` |
| `retryDelay` | Seconds between retry attempts | `30` |
| `timeoutDuration` | Total timeout duration in seconds | `300` (5 minutes) |
| `sendOnWifiConnect` | Send WOL packet when WiFi connects | `true` |
| `periodicRetry` | Enable periodic retries until timeout | `true` |

## Usage

### Setting up the Target MAC Address

1. Find your target device's MAC address:
   - **Windows**: Run `ipconfig /all` and look for "Physical Address"
   - **Linux/macOS**: Run `ifconfig` or `ip link show` and look for the MAC address
   - **Router**: Check your router's DHCP client list

2. Configure the MAC address in WLED:
   - Go to Config → Usermod Settings
   - Find the "WakeOnLAN" section
   - Enter the MAC address in format `AA:BB:CC:DD:EE:FF`
   - Save settings

### Automatic Wake-up

The usermod will automatically send a WOL packet when:
- WLED connects to WiFi (if `sendOnWifiConnect` is enabled)
- Periodically retry every `retryDelay` seconds until `timeoutDuration` is reached

### Manual Triggering

#### Via JSON API
Send a POST request to `/json/state` with:
```json
{
  "WakeOnLAN": {
    "wol": true
  }
}
```

#### Via MQTT
Publish to topic `[deviceTopic]/wol` with payload:
- `send`
- `wake` 
- `1`

### Target Device Configuration

Ensure your target device supports Wake-on-LAN:

1. **BIOS/UEFI Settings**: Enable "Wake on LAN" or "PME Event Wake Up"
2. **Network Adapter Settings** (Windows):
   - Device Manager → Network Adapters → Properties
   - Power Management tab: Check "Allow this device to wake the computer"
   - Advanced tab: Set "Wake on Magic Packet" to "Enabled"
3. **Linux**: Use `ethtool` to enable WOL:
   ```bash
   sudo ethtool -s eth0 wol g
   ```

## Installation

### Option 1: Using Build Flags (Recommended)
Add to your `platformio_override.ini`:
```ini
[env:your_board]
build_flags = 
  ${common.build_flags_esp32}
  -D USERMOD_WAKE_ON_LAN

build_src_filter = 
  ${common.build_src_filter}
  +<../usermods/Wake_on_LAN/>
```

### Option 2: Manual Integration
1. Copy the `usermod_wake_on_lan.h` file to your WLED project directory
2. Add `#include "usermod_wake_on_lan.h"` to your main file
3. Register the usermod in your setup

## Troubleshooting

### Common Issues

1. **WOL packet not received**:
   - Verify target device MAC address is correct
   - Ensure target device has WOL enabled in BIOS/OS
   - Check that devices are on the same network/broadcast domain

2. **No debug output**:
   - Enable serial debugging in WLED configuration
   - Monitor serial output at 115200 baud

3. **Target device not waking**:
   - Some devices only wake from specific sleep states
   - Ensure device is powered (not completely off)
   - Try different WOL settings in target device

### Debug Output

Enable serial debugging to see WOL activity:
```
WOL: WiFi connected
WOL: Magic packet sent to AA:BB:CC:DD:EE:FF via 192.168.1.255:9
```

## Network Requirements

- WLED device and target device must be on the same network segment
- Router/switch must forward broadcast packets (most do by default)
- Target device must be powered and connected to network (even when asleep)

## Security Considerations

- WOL packets are sent to the broadcast address and contain the target MAC address
- Consider network security implications of automatic wake-up
- Some networks may filter broadcast packets

## License

This usermod is part of WLED and follows the same MIT license.
