#!/usr/bin/env python3
"""
WLED Network Debug Listener
Listens for UDP debug messages from WLED devices.

Usage: python3 udp_debug_listener.py [port]
Default port: 7868
"""

import socket
import sys
from datetime import datetime

def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 7868
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('0.0.0.0', port))
    
    print(f"=" * 60)
    print(f"WLED Debug Listener started on UDP port {port}")
    print(f"Waiting for debug messages from ESP8266...")
    print(f"Your IP: Run 'hostname -I' to verify it matches WLED config")
    print(f"=" * 60)
    print()
    
    try:
        while True:
            data, addr = sock.recvfrom(4096)
            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
            message = data.decode('utf-8', errors='replace').strip()
            if message:
                print(f"[{timestamp}] {addr[0]}: {message}")
    except KeyboardInterrupt:
        print("\nListener stopped.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
