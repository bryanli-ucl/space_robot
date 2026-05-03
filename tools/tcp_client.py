#!/usr/bin/env python3
"""
PC-side TCP client for the Space Robot Giga WiFi server.

Usage:
    python tools/tcp_client.py <giga_ip> [port]

Example:
    python tools/tcp_client.py 192.168.1.42
    python tools/tcp_client.py 192.168.1.42 8080

Type 'warning' and press Enter to send a warning command.
Type 'quit' or press Ctrl+C to exit.
"""

import socket
import sys


def main() -> None:
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <giga_ip> [port]")
        print(f"Example: python {sys.argv[0]} 192.168.1.42")
        sys.exit(1)

    host = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5.0)

    try:
        print(f"[*] Connecting to {host}:{port} ...")
        sock.connect((host, port))
        print(f"[+] Connected. Type 'warning' to send, 'quit' to exit.\n")

        while True:
            try:
                msg = input("> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if not msg:
                continue
            if msg.lower() == "quit":
                break

            sock.sendall((msg + "\n").encode())
            print(f"[+] Sent: {msg}")

    except ConnectionRefusedError:
        print(f"[-] Connection refused. Is the Giga server running on {host}:{port}?")
    except socket.timeout:
        print("[-] Connection timed out.")
    except KeyboardInterrupt:
        print("\n[*] Interrupted by user.")
    finally:
        sock.close()
        print("[*] Disconnected.")


if __name__ == "__main__":
    main()
