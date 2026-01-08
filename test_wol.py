#!/usr/bin/env python3
"""
Simple Wake On LAN test script
"""
import socket
import struct

def wake_on_lan(mac_address):
    """Send a Wake On LAN magic packet to the given MAC address"""
    # Remove any separators and convert to bytes
    mac_bytes = bytes.fromhex(mac_address.replace(':', '').replace('-', ''))
    
    # Create magic packet: 6 bytes of 0xFF + 16 repetitions of MAC address
    magic_packet = b'\xff' * 6 + mac_bytes * 16
    
    # Send the packet
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.sendto(magic_packet, ('255.255.255.255', 9))
    sock.close()
    
    print(f"Magic packet sent to {mac_address}")
    print(f"Packet size: {len(magic_packet)} bytes")
    return True

if __name__ == "__main__":
    # Test with your WiFi MAC address
    mac = "94:e6:f7:0d:40:e9"
    print(f"Testing Wake On LAN for MAC: {mac}")
    wake_on_lan(mac)
