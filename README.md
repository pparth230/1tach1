# Mushi

3-DOF pan-tilt platform controlled via ESP32 web interface.

## Structure

- `arduino/` — Arduino sketches for ESP32
- `pi/` — Raspberry Pi code (camera integration, coming soon)

## Hardware

- ESP32 Dev Module
- PCA9685 PWM servo driver board (I2C address 0x40)
- 2x MG996R servos — differential parallel mechanism (tilt: pitch + roll)
- 1x MG996R servo — pan (channel 2)

## Servo Mapping

| Channel | Role  | Default | Range     |
|---------|-------|---------|-----------|
| 0       | Right | 90°     | 65°–105°  |
| 1       | Left  | 90°     | 65°–105°  |
| 2       | Pan   | 90°     | 50°–130°  |

## Web Interface

A built-in web UI is served directly from the ESP32:

- **Tilt joystick** — controls pitch and roll via differential servo mixing (±20°)
- **Pan slider** — controls pan left/right (±40° from center, 50°–130°)
- Live angle readout for all axes

WebSocket message format: `"x,y,pan"` (all values −1.0 to 1.0)

## Usage

1. Flash `arduino/Mushi/Mushi.ino` to ESP32
2. Connect to WiFi: **ServoControl** / `servo1234`
3. Open browser → `192.168.4.1`
4. Use the joystick to control tilt, slider for pan
