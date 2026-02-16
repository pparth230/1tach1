# 1tach1

2-DOF parallel tilt platform controlled via ESP32 web interface.

## Structure

- `arduino/` — Arduino sketches for ESP32
- `pi/` — Raspberry Pi code (camera integration, coming soon)

## Hardware

- ESP32 Dev Module
- PCA9685 PWM servo driver board
- 2x MG996R servos (differential parallel mechanism)

## Usage

1. Flash `arduino/1tach1/1tach1.ino` to ESP32
2. Connect to WiFi: **ServoControl** / `servo1234`
3. Open browser → `192.168.4.1`
4. Use the joystick to control tilt
