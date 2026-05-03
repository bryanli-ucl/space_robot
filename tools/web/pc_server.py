#!/usr/bin/env python3
"""
PC-side TCP Server + Logger for Space Robot fleet.

Architecture:
    - Giga boards act as TCP Clients, connecting to this PC Server.
    - Each robot sends HELLO:<id> upon connection.
    - PC can send commands to specific robots (e.g., 12:warning) or broadcast (ALL:stop).
    - All upstream data from robots is logged to file and printed to console.

Usage:
    python tools/web/pc_server.py [port]

Console commands:
    <id>:<cmd>      Send command to specific robot, e.g., 12:warning
    ALL:<cmd>       Broadcast to all online robots, e.g., ALL:shutdown
    list            Show all online robots
    quit / exit     Shutdown server
"""

import asyncio
import logging
import sys
from datetime import datetime
from typing import Dict, Optional


class RobotServer:
    def __init__(self, host: str = "0.0.0.0", port: int = 8080):
        self.host = host
        self.port = port
        self.clients: Dict[int, asyncio.StreamWriter] = {}  # robot_id -> writer
        self.lock = asyncio.Lock()
        self.logger = self._setup_logger()

    def _setup_logger(self) -> logging.Logger:
        logger = logging.getLogger("robot_fleet")
        logger.setLevel(logging.DEBUG)

        # Console handler
        ch = logging.StreamHandler()
        ch.setLevel(logging.INFO)

        # File handler with timestamped filename
        fh = logging.FileHandler(
            f"robot_fleet_{datetime.now():%Y%m%d_%H%M%S}.log"
        )
        fh.setLevel(logging.DEBUG)

        fmt = logging.Formatter(
            "%(asctime)s.%(msecs)03d [%(levelname)s] %(message)s",
            datefmt="%Y-%m-%d %H:%M:%S",
        )
        ch.setFormatter(fmt)
        fh.setFormatter(fmt)

        if not logger.handlers:
            logger.addHandler(ch)
            logger.addHandler(fh)
        return logger

    async def handle_client(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ):
        addr = writer.get_extra_info("peername")
        robot_id: Optional[int] = None
        self.logger.info(f"New connection from {addr}")

        try:
            while True:
                line = await reader.readline()
                if not line:
                    break

                msg = line.decode().strip()
                if not msg:
                    continue

                # Handle HELLO handshake
                if msg.startswith("HELLO:"):
                    try:
                        robot_id = int(msg.split(":", 1)[1])
                        async with self.lock:
                            self.clients[robot_id] = writer
                        self.logger.info(f"Robot {robot_id} registered from {addr}")
                        print(f"[+] Robot {robot_id} online")
                        continue
                    except (ValueError, IndexError):
                        self.logger.warning(f"Invalid HELLO from {addr}: {msg}")
                        continue

                # Log all upstream data
                self.logger.info(f"[UPSTREAM] {msg}")
                print(f"  [{robot_id or '?'}] -> {msg}")

        except asyncio.CancelledError:
            pass
        finally:
            if robot_id is not None:
                async with self.lock:
                    self.clients.pop(robot_id, None)
                self.logger.info(f"Robot {robot_id} disconnected")
                print(f"[-] Robot {robot_id} offline")
            else:
                self.logger.info(f"Unregistered client {addr} disconnected")
            writer.close()

    async def send_to(self, robot_id: int, cmd: str) -> bool:
        async with self.lock:
            writer = self.clients.get(robot_id)
            if not writer:
                return False
            try:
                line = f"CMD:{robot_id}:{cmd}\n"
                writer.write(line.encode())
                await writer.drain()
                self.logger.info(f"[DOWNSTREAM] {line.strip()}")
                return True
            except Exception as e:
                self.logger.error(f"Failed to send to {robot_id}: {e}")
                return False

    async def broadcast(self, cmd: str) -> None:
        async with self.lock:
            items = list(self.clients.items())
        for rid, writer in items:
            try:
                line = f"CMD:ALL:{cmd}\n"
                writer.write(line.encode())
                await writer.drain()
                self.logger.info(f"[DOWNSTREAM] {line.strip()}")
            except Exception as e:
                self.logger.error(f"Broadcast to {rid} failed: {e}")

    async def cmd_loop(self):
        print("\n=== Robot Fleet Console ===")
        print("Commands:")
        print("  <id>:<cmd>     Send command to specific robot (e.g., 12:warning)")
        print("  ALL:<cmd>      Broadcast to all robots (e.g., ALL:stop)")
        print("  list           Show online robots")
        print("  quit / exit    Shutdown server\n")

        while True:
            try:
                user_input = await asyncio.to_thread(input, "> ")
            except EOFError:
                break

            user_input = user_input.strip()
            if not user_input:
                continue

            if user_input.lower() in ("quit", "exit", "q"):
                break

            if user_input.lower() == "list":
                async with self.lock:
                    if self.clients:
                        print("Online robots:", sorted(self.clients.keys()))
                    else:
                        print("No robots online.")
                continue

            # Parse <id>:<cmd> or ALL:<cmd>
            if ":" not in user_input:
                print("[!] Invalid format. Use <id>:<cmd> or ALL:<cmd>")
                continue

            target, cmd = user_input.split(":", 1)
            target = target.strip()
            cmd = cmd.strip()

            if target.upper() == "ALL":
                await self.broadcast(cmd)
                print(f"[+] Broadcasted '{cmd}' to all robots")
            else:
                try:
                    robot_id = int(target)
                    ok = await self.send_to(robot_id, cmd)
                    if ok:
                        print(f"[+] Sent '{cmd}' to robot {robot_id}")
                    else:
                        print(f"[!] Robot {robot_id} is not online")
                except ValueError:
                    print("[!] Robot ID must be a number or ALL")

    async def run(self):
        server = await asyncio.start_server(
            self.handle_client, self.host, self.port
        )
        self.logger.info(f"Server listening on {self.host}:{self.port}")
        print(f"[*] Server listening on {self.host}:{self.port}")

        async with server:
            await asyncio.gather(
                server.serve_forever(),
                self.cmd_loop(),
            )


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    srv = RobotServer(port=port)
    try:
        asyncio.run(srv.run())
    except KeyboardInterrupt:
        print("\n[*] Server stopped by user")


if __name__ == "__main__":
    main()
