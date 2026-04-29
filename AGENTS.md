# Space Robot — Agent Guide

> **Target reader:** AI coding agents with no prior knowledge of this project.

## Project Overview

This is an embedded robotics firmware project for the **Arduino Giga R1 WiFi** (STM32H747xx dual-core, M7 core). It controls a mobile robot chassis via I2C motor controllers, reads RFID tags, connects over WiFi, and maintains a 9×9 navigation grid. The project is built with **PlatformIO** using the Arduino framework on top of mbed.

- **Board:** Arduino Giga R1 (M7 core)
- **Platform:** `ststm32`
- **Framework:** Arduino (mbed underneath)
- **Language:** C++14 (GNU++14)
- **MCU:** STM32H747xx Cortex-M7

## Repository Layout

```
space_robot/
├── platformio.ini          # Build configuration
├── dfu_upload.py           # Custom DFU upload script
├── .clang-format           # Code formatter configuration
├── src/
│   ├── space_robot.cpp     # Main firmware entry point (setup / loop)
│   ├── printf_redirect.cpp # Redirects stdout/stderr → Serial1
│   ├── include/            # Core headers shared across the project
│   │   ├── space_robot.hpp # Global constants, I2C addresses, grid map
│   │   ├── literals.hpp    # Compile-time physics unit system
│   │   ├── serial_logger.hpp # Macro-based logging with levels
│   │   └── singleton.hpp   # Generic singleton template + macro
│   ├── tasks/              # Finite-state machine logic
│   │   ├── tinyfsm.hpp     # Third-party header-only FSM library
│   │   ├── robot_fsm.hpp   # Robot FSM events
│   │   └── robot_fsm.cpp   # (currently empty)
│   └── utils/              # Stand-alone utility sketches
│       └── motoron_set_addr.cpp  # I2C address tool for Motoron controllers
├── libraries/              # Custom hardware drivers
│   ├── motor_driver/       # Pololu Motoron I2C wrapper
│   ├── rfid_driver/        # MFRC522 I2C wrapper
│   ├── wifi_driver/        # WiFi connection manager
│   └── ir_driver/          # IR sensor driver (placeholder)
├── docs/                   # Hardware datasheets & schematics
└── cad/                    # CAD files
```

## Build System

### Key Configuration (`platformio.ini`)

- **Environments:**
  - `release` — main robot firmware. Excludes files under `src/utils/`.
  - `utils` — utility sketches. Excludes `space_robot.cpp` and most `utils/**.cpp` except `motoron_set_addr.cpp`.
- **Upload method:** Custom DFU (`dfu_upload.py`). The script waits for USB VID/PID `0483:df11` and then calls `dfu-util -d 0483:df11 -a 0 -s 0x08040000 -D <firmware.bin>`.
- **Build flags:** `-Os` (release), `-O0 -g` (debug), `-I src/include`.
- **Library dependencies (managed by PlatformIO):**
  - `kkloesener/MFRC522_I2C@^1.0`
  - `pololu/Motoron@^1.5.0`

### Common Commands

```bash
# Build the main firmware
pio run -e release

# Build utility sketches
pio run -e utils

# Upload main firmware via DFU
pio run -e release --target upload

# Upload utility sketch via DFU
pio run -e utils --target upload
```

**Note:** After a successful DFU upload you must **press the RESET button** on the board for the new firmware to run.

## Hardware Architecture

### I2C Buses
- **Wire** — connected to the MFRC522 RFID reader.
- **Wire1** — connected to the two Pololu Motoron motor controllers.

### I2C Addresses (`src/include/space_robot.hpp`)
| Device        | Address |
|---------------|---------|
| Motoron #1    | `0x20`  |
| Motoron #2    | `0x21`  |
| MFRC522 RFID  | `0x28`  |

### Pins
- **Serial1** — debug console at 115200 baud (redirected to `stdout`/`stderr`).
- **RGB LED** — `D86` (R), `D87` (G), `D88` (B), active-low.

### Motors (4-wheel chassis)
Each motor is mapped to a controller + channel via `MotorDriver::get_chassis_endpoint`:
- FrontLeft  → controller 1, channel 2
- FrontRight → controller 1, channel 3
- RearLeft   → controller 2, channel 2
- RearRight  → controller 2, channel 3

Default acceleration = 80, default deceleration = 300 (Motoron units).

## Code Style Guidelines

The project follows the rules in `.clang-format`:

- **Indent:** 4 spaces (never tabs).
- **Pointer/reference alignment:** Left (`auto* ptr`).
- **Brace style:** Attach (same-line braces).
- **Column limit:** 0 (no hard limit).
- **Short functions / blocks / ifs / loops:** allowed on a single line.
- **Consecutive assignments:** aligned.
- **Trailing comments:** aligned.
- **Preprocessor directives:** indented after `#`.
- **Namespace indentation:** none.

### Coding Conventions

1. **Headers:** always use `#pragma once`.
2. **Namespaces:** drivers and modules live in their own namespace (e.g., `MotorDriver`, `WifiDriver`, `RFID`, `literals`).
3. **Return type syntax:** prefer trailing return type for functions:
   ```cpp
   auto func() -> int;
   ```
4. **Constants:** use `constexpr` for compile-time values.
5. **Physics units:** dimensional quantities are expressed with the custom `literals::Quantity<M, L, T>` template. Unit literals are provided (e.g., `5.0_m_s`, `100_ms`, `1.0_kg`).
6. **Logging:** use the macros from `serial_logger.hpp`:
   ```cpp
   LOG_TRACE("val={}", 42);
   LOG_DEBUG("Speed is {}", speed);
   LOG_INFO("System begin");
   LOG_WARN("Low battery");
   LOG_ERROR("Motor fault");
   ```
   - Log level is controlled at compile time via `LOG_LEVEL` (default `LOG_LEVEL_DEBUG`).
   - `LOG_LEVEL_OFF` disables all logging code entirely.
   - `_START` variants print without a trailing newline.
   - `LOG_SECTION(str)` prints a banner.
7. **Singletons:** use the CRTP `singleton<T>` class and the `MAKE_SINGLETON(MyClass)` macro inside the class body.
8. **Global grid map:** `g_grid` is a 9×9 `std::array` of `grid_slot_t` (`Unknow`, `Fertile`, `Infertile`).

## Module Reference

### `motor_driver`
Wraps two `MotoronI2C` instances on `Wire1`. Provides:
- `begin()` — initialise both controllers with default accel/decel.
- `set_speed(controller, motor, speed)` — direct Motoron call.
- `set_chassis_speed(motor, speed)` / `set_chassis_speeds(...)` — chassis-oriented API.
- `stop_chassis()` / `stop_all()` — emergency stops.

### `rfid_driver`
Wraps `MFRC522_I2C` on `Wire`.
- `begin()` — initialises the reader and runs a self-test.
- `update()` — placeholder (returns `0`).

### `wifi_driver`
Wraps the Arduino `WiFi` library.
- `begin()` — connects using the hard-coded SSID / password in `wifi_pwd.hpp`.
- `is_connected()` / `local_ip()` — status helpers.

### `ir_driver`
Currently a placeholder (empty implementation).

### `tasks/robot_fsm`
Defines `Event_WifiData` for the TinyFSM state machine. The FSM file (`robot_fsm.cpp`) is empty at the moment; states and transitions have not been implemented yet.

## Testing & Quality

- **There are no automated unit tests in this repository.**
- Code formatting is enforced via `clang-format`.
- Recommended IDE: VS Code with the **PlatformIO IDE** extension (see `.vscode/extensions.json`).

## Security & Operational Notes

1. **Hard-coded credentials:** WiFi SSID and password are stored in plain text in `libraries/wifi_driver/wifi_pwd.hpp`. Do not commit this file to public repositories without redacting.
2. **DFU upload:** Uploading requires the board to be in DFU mode (usually by double-tapping the RESET button or by holding BOOT + RESET). The custom script polls `lsusb` for `0483:df11`.
3. **No CRC on Motoron:** the firmware calls `disableCrc()` on the Motoron controllers for performance/simplicity.
4. **Serial1 is the primary debug interface:** `printf()` and the logging macros all emit to `Serial1` thanks to `printf_redirect.cpp`.
