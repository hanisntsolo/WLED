# Wake-on-LAN Usermod

This usermod enables WLED to send Wake-on-LAN (WOL) magic packets to wake up remote devices on the network. It supports both WiFi and Ethernet connections and includes advanced features for cross-segment network support.

## Features

- **Multi-Interface Support**: Works with both WiFi and Ethernet connections
- **Cross-Segment Support**: Sends WOL packets across network segments using multiple strategies
- **Multiple Broadcast Methods**: Uses both local and limited broadcast addresses
- **Multiple Ports**: Sends to both standard WOL ports (7 and 9) for maximum compatibility
- **Custom Target IP**: Allows specifying a custom target IP for directed WOL packets
- **Automatic Retry**: Configurable retry mechanism with timeout
- **MQTT Integration**: Can be triggered via MQTT messages
- **JSON API Support**: Can be triggered via JSON API calls

## Installation

1. Copy `usermod_wake_on_lan.h` to your WLED `usermods/Wake_on_LAN/` folder
2. Add the following to your `usermods_list.cpp`:

```cpp
#include "../usermods/Wake_on_LAN/usermod_wake_on_lan.h"
```

3. Register the usermod in `usermods_list.cpp`:

```cpp
void registerUsermods()
{
  // ... other usermods
  usermods.add(new UsermodWakeOnLAN());
}
```

4. Define the usermod ID in `const.h`:

```cpp
#define USERMOD_ID_WAKE_ON_LAN 42  // Choose an available ID
```

## Configuration

### Web Interface

Navigate to Config → Usermods → WakeOnLAN to configure:

- **Enabled**: Enable/disable the usermod
- **Target MAC**: MAC address of the device to wake (format: AA:BB:CC:DD:EE:FF)
- **Retry Delay**: Seconds between retry attempts (default: 30)
- **Timeout Duration**: Total timeout duration in seconds (default: 300)
- **Send on WiFi Connect**: Automatically send WOL when network connects (default: true)
- **Periodic Retry**: Enable periodic retries until timeout (default: true)
- **Use Multiple Ports**: Send to both ports 7 and 9 (default: true)
- **Use Limited Broadcast**: Send to 255.255.255.255 in addition to local broadcast (default: true)
- **Custom Target IP**: Optional custom IP address for directed WOL packets

### JSON Configuration

```json
{
  "WakeOnLAN": {
    "enabled": true,
    "targetMAC": "AA:BB:CC:DD:EE:FF",
    "retryDelay": 30,
    "timeoutDuration": 300,
    "sendOnWifiConnect": true,
    "periodicRetry": true,
    "useMultiplePorts": true,
    "useLimitedBroadcast": true,
    "customTargetIP": "192.168.1.100"
  }
}
```

## Usage

### Automatic Trigger

When `sendOnWifiConnect` is enabled, WOL packets are automatically sent when the ESP connects to the network.

### Manual Trigger via JSON API

Send a POST request to `/json/state` with:

```json
{
  "WakeOnLAN": {
    "wol": true
  }
}
```

### MQTT Trigger

Send any of these values to the `/wol` topic:
- `send`
- `wake` 
- `1`

Example: `mosquitto_pub -h broker -t "wled/device/wol" -m "send"`

## Cross-Segment Network Support

This usermod implements several strategies to ensure WOL packets reach devices across network segments:

1. **Local Broadcast**: Sends to the calculated broadcast address of the local subnet
2. **Limited Broadcast**: Sends to 255.255.255.255 (if enabled)
3. **Custom Target IP**: Sends to a user-specified IP address
4. **Gateway Forwarding**: Attempts to send via the gateway for cross-segment delivery
5. **Multiple Ports**: Uses both standard WOL ports (7 and 9) for maximum compatibility

## Troubleshooting

### Network Capture Analysis

Your tcpdump output shows WOL packets are being sent correctly:

```
01:18:05.422078 IP 192.168.29.11.61945 > 192.168.29.255.discard: UDP, length 102
01:18:15.456931 IP 192.168.29.11.61945 > 192.168.29.255.discard: UDP, length 102
```

The packets are 102 bytes (correct for WOL magic packets) and are being sent to the broadcast address.

### Common Issues

1. **Target device not waking**: 
   - Verify Wake-on-LAN is enabled in target device BIOS/UEFI
   - Check network adapter WOL settings in device manager/OS
   - Try using a custom target IP pointing to the device's last known IP

2. **Cross-segment issues**:
   - Enable `customTargetIP` and set it to the target device's IP
   - Ensure network infrastructure supports WOL packet forwarding
   - Try enabling `useLimitedBroadcast` for better router compatibility

3. **Ethernet not working**:
   - Ensure Ethernet is properly initialized in WLED
   - Check that `ETHERNET_ENABLE` is defined for your build
   - Verify network cable and switch support WOL

### Debug Information

Enable debug output to see detailed WOL packet information:

```
WOL: Using WiFi interface
WOL: Sent to local broadcast 192.168.29.255:9
WOL: Sent to limited broadcast 255.255.255.255:9
WOL: Magic packet(s) sent for AA:BB:CC:DD:EE:FF
```

## Network Requirements

- Target device must support and have Wake-on-LAN enabled
- Network switches must not filter broadcast/multicast packets
- For cross-segment operation, routers must be configured to forward WOL packets
- Some enterprise networks may block broadcast traffic for security

## Performance Notes

- UDP packets are sent asynchronously and don't block WLED operation
- Multiple packet transmission increases success rate but uses more network bandwidth
- Retry mechanism helps with packet loss but can create network traffic during timeout period
- Consider adjusting retry delay and timeout based on your network characteristics
