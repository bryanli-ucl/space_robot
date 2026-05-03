#!/usr/bin/env python3
"""
PC-side TCP Server + Web UI for Space Robot fleet.

Usage:
    python pc_server_ui.py [tcp_port] [web_port]

Dependencies:
    pip install flask flask-sock

Then open http://<pc_ip>:5000 in your browser.
"""

import asyncio
import json
import queue
import sys
import threading
import time
from datetime import datetime

from flask import Flask, render_template_string
from flask_sock import Sock

# ---------------------------------------------------------------------------
# Shared state & thread-safe queues
# ---------------------------------------------------------------------------

tcp_to_ws = queue.Queue()   # TCP Server -> WebSocket (dict messages)
ws_to_tcp = queue.Queue()   # WebSocket -> TCP Server (dict commands)

# robot_id -> asyncio.StreamWriter
clients: dict[int, asyncio.StreamWriter] = {}
clients_lock = threading.Lock()

# robot_id -> last_seen_timestamp (for heartbeat timeout)
robot_last_seen: dict[int, float] = {}

# ---------------------------------------------------------------------------
# Asyncio TCP Server (background thread)
# ---------------------------------------------------------------------------

async def tcp_server(host: str, port: int):
    async def handle_client(reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        addr = writer.get_extra_info("peername")
        robot_id: int | None = None
        tcp_to_ws.put({"t": "log", "msg": f"New connection from {addr}"})

        try:
            while True:
                line = await reader.readline()
                if not line:
                    break
                msg = line.decode().strip()
                if not msg:
                    continue

                if msg.startswith("HELLO:"):
                    try:
                        robot_id = int(msg.split(":", 1)[1])
                        with clients_lock:
                            # 强制关闭该机器人的旧连接，避免重复
                            if robot_id in clients:
                                old_writer = clients[robot_id]
                                try:
                                    old_writer.close()
                                except Exception:
                                    pass
                            clients[robot_id] = writer
                            robot_last_seen[robot_id] = time.time()
                        tcp_to_ws.put({
                            "t": "status",
                            "robot_id": robot_id,
                            "event": "online",
                        })
                    except (ValueError, IndexError):
                        pass
                    continue

                # 心跳刷新
                if robot_id is not None:
                    with clients_lock:
                        robot_last_seen[robot_id] = time.time()

                tcp_to_ws.put({
                    "t": "data",
                    "robot_id": robot_id,
                    "msg": msg,
                    "time": datetime.now().strftime("%H:%M:%S.%f")[:-3],
                })
        except asyncio.CancelledError:
            pass
        finally:
            if robot_id is not None:
                with clients_lock:
                    # 只有当前 writer 仍是记录中的那个才 pop，防止误删新连接
                    if clients.get(robot_id) is writer:
                        clients.pop(robot_id, None)
                        robot_last_seen.pop(robot_id, None)
                    tcp_to_ws.put({
                        "t": "status",
                        "robot_id": robot_id,
                        "event": "offline",
                    })
            writer.close()

    srv = await asyncio.start_server(handle_client, host, port)
    async with srv:
        # Background coroutine: drain commands from WebSocket -> TCP
        async def cmd_pump():
            while True:
                await asyncio.sleep(0.05)
                try:
                    cmd = ws_to_tcp.get_nowait()
                except queue.Empty:
                    continue

                target = cmd.get("target")
                command = cmd.get("command", "")
                if not command:
                    continue

                with clients_lock:
                    if target == "ALL":
                        for rid, w in list(clients.items()):
                            try:
                                w.write(f"CMD:ALL:{command}\n".encode())
                            except Exception:
                                pass
                    else:
                        try:
                            rid = int(target)
                        except ValueError:
                            continue
                        w = clients.get(rid)
                        if w:
                            try:
                                w.write(f"CMD:{rid}:{command}\n".encode())
                            except Exception:
                                pass

        # Background coroutine: health check — mark stale robots offline
        async def health_checker():
            while True:
                await asyncio.sleep(5)
                now = time.time()
                stale = []
                with clients_lock:
                    for rid, last_seen in list(robot_last_seen.items()):
                        if now - last_seen > 15:
                            stale.append(rid)
                    for rid in stale:
                        w = clients.pop(rid, None)
                        robot_last_seen.pop(rid, None)
                        if w:
                            try:
                                w.close()
                            except Exception:
                                pass
                        tcp_to_ws.put({
                            "t": "status",
                            "robot_id": rid,
                            "event": "offline",
                        })

        await asyncio.gather(srv.serve_forever(), cmd_pump(), health_checker())


def run_tcp_server(host: str, port: int):
    asyncio.run(tcp_server(host, port))


# ---------------------------------------------------------------------------
# Flask Web App
# ---------------------------------------------------------------------------

app = Flask(__name__)
sock = Sock(app)

HTML_PAGE = r"""
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Robot Fleet Control Center</title>
<style>
    :root {
        --bg: #0d1117;
        --bg2: #161b22;
        --border: #30363d;
        --text: #c9d1d9;
        --text-dim: #8b949e;
        --dat: #58a6ff;
        --evt: #d29922;
        --log: #3fb950;
        --cmd: #8b949e;
        --err: #f85149;
        --online: #3fb950;
        --offline: #8b949e;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
        background: var(--bg);
        color: var(--text);
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace;
        height: 100vh;
        display: flex;
        flex-direction: column;
        overflow: hidden;
    }
    header {
        background: var(--bg2);
        border-bottom: 1px solid var(--border);
        padding: 12px 20px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        flex-shrink: 0;
    }
    header h1 { font-size: 18px; font-weight: 600; letter-spacing: 0.5px; }
    #status-bar {
        display: flex;
        gap: 10px;
        align-items: center;
    }
    .robot-dot {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        background: var(--bg);
        border: 1px solid var(--border);
        padding: 3px 10px;
        border-radius: 12px;
        font-size: 12px;
        font-weight: 600;
        cursor: pointer;
        user-select: none;
        transition: border-color 0.2s;
    }
    .robot-dot:hover { border-color: var(--text-dim); }
    .robot-dot.active { border-color: var(--dat); }
    .robot-dot .dot {
        width: 8px; height: 8px; border-radius: 50%;
        background: var(--offline);
    }
    .robot-dot.online .dot { background: var(--online); box-shadow: 0 0 6px var(--online); }
    #log-container {
        flex: 1;
        overflow-y: auto;
        padding: 10px 20px;
        font-family: "SF Mono", Monaco, "Cascadia Code", monospace;
        font-size: 13px;
        line-height: 1.6;
    }
    .line {
        display: flex;
        gap: 10px;
        padding: 2px 0;
        border-bottom: 1px solid transparent;
        animation: fadeIn 0.2s ease;
    }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(2px); } to { opacity: 1; } }
    .line:hover { background: rgba(255,255,255,0.03); }
    .ts { color: var(--text-dim); min-width: 80px; flex-shrink: 0; }
    .badge {
        padding: 1px 6px;
        border-radius: 4px;
        font-size: 11px;
        font-weight: 700;
        text-transform: uppercase;
        flex-shrink: 0;
        min-width: 36px;
        text-align: center;
    }
    .badge-DAT { background: rgba(88,166,255,0.15); color: var(--dat); }
    .badge-EVT { background: rgba(210,153,34,0.15); color: var(--evt); }
    .badge-LOG { background: rgba(63,185,80,0.15); color: var(--log); }
    .badge-CMD { background: rgba(139,148,158,0.15); color: var(--cmd); font-style: italic; }
    .badge-UNK { background: rgba(139,148,158,0.1); color: var(--text-dim); }
    .rid { color: var(--text-dim); min-width: 24px; text-align: right; flex-shrink: 0; }
    .msg { word-break: break-all; }
    .msg-dat { color: #79c0ff; }
    .msg-evt { color: #e3b341; }
    .msg-log { color: #56d364; }
    .msg-cmd { color: var(--text-dim); font-style: italic; }

    #controls {
        background: var(--bg2);
        border-top: 1px solid var(--border);
        padding: 12px 20px;
        display: flex;
        gap: 10px;
        align-items: center;
        flex-shrink: 0;
    }
    select, input[type="text"] {
        background: var(--bg);
        color: var(--text);
        border: 1px solid var(--border);
        border-radius: 6px;
        padding: 8px 12px;
        font-size: 14px;
        outline: none;
    }
    select { cursor: pointer; }
    input[type="text"] { flex: 1; min-width: 120px; }
    input[type="text"]:focus { border-color: var(--dat); }
    button {
        background: var(--bg);
        color: var(--text);
        border: 1px solid var(--border);
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 13px;
        font-weight: 600;
        cursor: pointer;
        transition: all 0.15s;
    }
    button:hover { background: var(--border); }
    button.primary { background: var(--dat); color: #fff; border-color: var(--dat); }
    button.primary:hover { filter: brightness(1.1); }
    button.danger { background: var(--err); color: #fff; border-color: var(--err); }
    button.danger:hover { filter: brightness(1.1); }
    .quick-btns { display: flex; gap: 6px; }
    #conn-status {
        font-size: 12px;
        color: var(--text-dim);
        margin-left: auto;
    }
    .filter-hint {
        font-size: 11px;
        color: var(--text-dim);
        margin-left: 10px;
    }
</style>
</head>
<body>

<header>
    <h1>🤖 Robot Fleet Control Center</h1>
    <div id="status-bar">
        <span style="font-size:12px;color:var(--text-dim)">Online:</span>
        <div id="robot-badges"></div>
        <span class="filter-hint" id="filter-hint"></span>
    </div>
</header>

<div id="log-container"></div>

<div id="controls">
    <select id="target-select">
        <option value="ALL">ALL</option>
    </select>
    <input type="text" id="cmd-input" placeholder="Type command... (e.g. warning, shutdown, stop)" autocomplete="off">
    <div class="quick-btns">
        <button onclick="sendQuick('warning')">⚠️ warning</button>
        <button onclick="sendQuick('shutdown')" class="danger">⏻ shutdown</button>
        <button onclick="sendQuick('stop')">⏹ stop</button>
    </div>
    <button onclick="sendCmd()" class="primary">Send</button>
    <span id="conn-status">Connecting...</span>
</div>

<script>
    const logContainer = document.getElementById('log-container');
    const targetSelect = document.getElementById('target-select');
    const cmdInput = document.getElementById('cmd-input');
    const connStatus = document.getElementById('conn-status');
    const robotBadges = document.getElementById('robot-badges');
    const filterHint = document.getElementById('filter-hint');

    let ws;
    let onlineRobots = new Set();
    let filterRobot = null;
    let autoScroll = true;
    let lineCount = 0;
    const MAX_LINES = 2000;

    function connect() {
        const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
        ws = new WebSocket(`${proto}//${location.host}/ws`);

        ws.onopen = () => {
            connStatus.textContent = 'WebSocket connected';
            connStatus.style.color = 'var(--log)';
        };

        ws.onmessage = (ev) => {
            const data = JSON.parse(ev.data);
            handleMessage(data);
        };

        ws.onclose = () => {
            connStatus.textContent = 'Disconnected - reconnecting...';
            connStatus.style.color = 'var(--err)';
            setTimeout(connect, 2000);
        };

        ws.onerror = (err) => {
            connStatus.textContent = 'WebSocket error';
            connStatus.style.color = 'var(--err)';
        };
    }

    function getBadgeClass(msg) {
        if (msg.startsWith('DAT:')) return 'badge-DAT';
        if (msg.startsWith('EVT:')) return 'badge-EVT';
        if (msg.startsWith('LOG:')) return 'badge-LOG';
        if (msg.startsWith('CMD:')) return 'badge-CMD';
        return 'badge-UNK';
    }

    function getMsgClass(cls) {
        if (cls === 'badge-DAT') return 'msg-dat';
        if (cls === 'badge-EVT') return 'msg-evt';
        if (cls === 'badge-LOG') return 'msg-log';
        if (cls === 'badge-CMD') return 'msg-cmd';
        return '';
    }

    function parseType(msg) {
        if (msg.startsWith('DAT:')) return 'DAT';
        if (msg.startsWith('EVT:')) return 'EVT';
        if (msg.startsWith('LOG:')) return 'LOG';
        if (msg.startsWith('CMD:')) return 'CMD';
        return 'UNK';
    }

    function extractRobotId(msg) {
        const m = msg.match(/^\w+:(\d+|[A-Z]+):/);
        if (m) return m[1];
        const m2 = msg.match(/^\w+:(\d+)/);
        if (m2) return m2[1];
        return '';
    }

    function handleMessage(data) {
        if (data.t === 'status') {
            const rid = data.robot_id;
            if (data.event === 'online') {
                onlineRobots.add(rid);
                appendLine({
                    time: new Date().toLocaleTimeString(),
                    badge: 'badge-LOG',
                    rid: rid,
                    msg: `Robot ${rid} online`,
                    msgClass: 'msg-log',
                });
            } else {
                onlineRobots.delete(rid);
                appendLine({
                    time: new Date().toLocaleTimeString(),
                    badge: 'badge-EVT',
                    rid: rid,
                    msg: `Robot ${rid} offline`,
                    msgClass: 'msg-evt',
                });
            }
            updateRobotBadges();
            updateTargetSelect();
            return;
        }

        if (data.t === 'log') {
            appendLine({
                time: new Date().toLocaleTimeString(),
                badge: 'badge-UNK',
                rid: '',
                msg: data.msg,
                msgClass: '',
            });
            return;
        }

        if (data.t === 'data') {
            const type = parseType(data.msg);
            const rid = extractRobotId(data.msg) || (data.robot_id || '');
            if (filterRobot && rid !== String(filterRobot)) return;

            appendLine({
                time: data.time || new Date().toLocaleTimeString(),
                badge: getBadgeClass(data.msg),
                rid: rid,
                msg: data.msg,
                msgClass: getMsgClass(getBadgeClass(data.msg)),
            });
        }
    }

    function appendLine(item) {
        const div = document.createElement('div');
        div.className = 'line';
        div.innerHTML = `
            <span class="ts">${item.time}</span>
            <span class="badge ${item.badge}">${item.badge.replace('badge-', '')}</span>
            <span class="rid">${item.rid}</span>
            <span class="msg ${item.msgClass}">${escapeHtml(item.msg)}</span>
        `;
        logContainer.appendChild(div);
        lineCount++;

        if (lineCount > MAX_LINES) {
            logContainer.removeChild(logContainer.firstChild);
            lineCount--;
        }

        if (autoScroll) {
            logContainer.scrollTop = logContainer.scrollHeight;
        }
    }

    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    function updateRobotBadges() {
        robotBadges.innerHTML = '';
        const sorted = Array.from(onlineRobots).sort((a, b) => a - b);
        for (const rid of sorted) {
            const el = document.createElement('span');
            el.className = 'robot-dot online' + (filterRobot === rid ? ' active' : '');
            el.innerHTML = `<span class="dot"></span>#${rid}`;
            el.onclick = () => toggleFilter(rid);
            robotBadges.appendChild(el);
        }
        if (sorted.length === 0) {
            robotBadges.innerHTML = '<span style="color:var(--text-dim);font-size:12px">None</span>';
        }
    }

    function updateTargetSelect() {
        const current = targetSelect.value;
        targetSelect.innerHTML = '<option value="ALL">ALL</option>';
        for (const rid of Array.from(onlineRobots).sort((a, b) => a - b)) {
            const opt = document.createElement('option');
            opt.value = rid;
            opt.textContent = `#${rid}`;
            targetSelect.appendChild(opt);
        }
        if (Array.from(targetSelect.options).some(o => o.value === current)) {
            targetSelect.value = current;
        }
    }

    function toggleFilter(rid) {
        if (filterRobot === rid) {
            filterRobot = null;
            filterHint.textContent = '';
        } else {
            filterRobot = rid;
            filterHint.textContent = `Showing only #${rid} (click again to clear)`;
        }
        updateRobotBadges();
    }

    function sendCmd() {
        const target = targetSelect.value;
        const command = cmdInput.value.trim();
        if (!command) return;

        ws.send(JSON.stringify({ target, command }));

        appendLine({
            time: new Date().toLocaleTimeString(),
            badge: 'badge-CMD',
            rid: target,
            msg: `CMD:${target}:${command}`,
            msgClass: 'msg-cmd',
        });

        cmdInput.value = '';
    }

    function sendQuick(cmd) {
        const target = targetSelect.value;
        ws.send(JSON.stringify({ target, command: cmd }));

        appendLine({
            time: new Date().toLocaleTimeString(),
            badge: 'badge-CMD',
            rid: target,
            msg: `CMD:${target}:${cmd}`,
            msgClass: 'msg-cmd',
        });
    }

    cmdInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') sendCmd();
    });

    logContainer.addEventListener('scroll', () => {
        const nearBottom = logContainer.scrollHeight - logContainer.scrollTop - logContainer.clientHeight < 50;
        autoScroll = nearBottom;
    });

    connect();
</script>

</body>
</html>
"""


@app.route("/")
def index():
    return render_template_string(HTML_PAGE)


@sock.route("/ws")
def websocket(ws):
    # Forward TCP -> WS
    def sender():
        while True:
            data = tcp_to_ws.get()
            try:
                ws.send(json.dumps(data))
            except Exception:
                break

    t = threading.Thread(target=sender, daemon=True)
    t.start()

    # Forward WS -> TCP
    while True:
        data = ws.receive()
        if data is None:
            break
        try:
            ws_to_tcp.put(json.loads(data))
        except Exception:
            pass


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    tcp_port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    web_port = int(sys.argv[2]) if len(sys.argv) > 2 else 5000

    threading.Thread(
        target=run_tcp_server, args=("0.0.0.0", tcp_port), daemon=True
    ).start()

    print(f"[*] TCP Server  : 0.0.0.0:{tcp_port}")
    print(f"[*] Web UI      : http://0.0.0.0:{web_port}")
    print(f"[*] Open in browser: http://<your-pc-ip>:{web_port}")
    print()

    # Disable Flask request logging to keep console clean
    import logging

    log = logging.getLogger("werkzeug")
    log.setLevel(logging.ERROR)

    app.run(host="0.0.0.0", port=web_port, debug=False, use_reloader=False)
