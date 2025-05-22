# Myoelectric Prosthetic Hand

This project uses an ESP32 microcontroller to control a prosthetic hand based on muscle signals (EMG). It supports different modes, capacitive touch feedback, servo motor control, and power-saving with deep sleep functionality.

## 🧠 Features

- **EMG-Based Control**: Converts muscle activity into servo angles to open/close the hand.
- **Multi-Mode Operation**: Toggle between full-hand and finger-specific control.
- **Touch Feedback**: Capacitive touch sensors trigger vibration feedback via a motor.
- **Power Management**: Deep sleep mode triggered via a power button with wake-on-press.
- **Startup Animation**: Visual indication of power-on through servo animation.
- **Button Debouncing**: Reliable toggling of power and mode states.

## ⚙️ Hardware Components

- ESP32 Dev Module
- 4x Servo Motors (MG90 or similar)
- 1x EMG Sensor (e.g., MyoWare)
- 2x Capacitive Touch Sensors (connected to `T4` and `T9`)
- 1x Vibration Motor (controlled via digital pin)
- Power Button + Mode Button + LEDs
- External Power Supply

## 🧾 Pin Configuration

| Component        | ESP32 Pin |
|------------------|-----------|
| Power Button     | GPIO 18   |
| Power LED        | GPIO 10   |
| Mode Button      | GPIO 15   |
| Mode LED         | GPIO 21   |
| EMG Sensor       | GPIO 34   |
| Touch Sensor 1   | T4 (GPIO 4)  |
| Touch Sensor 2   | T9 (GPIO 32) |
| Vibration Motor  | GPIO 13   |
| Servo 1 (Thumb)  | GPIO 14   |
| Servo 2 (Index)  | GPIO 27   |
| Servo 3 (Middle) | GPIO 26   |
| Servo 4 (Ring/Pinky) | GPIO 25 |

## 🧪 EMG to Servo Mapping

EMG readings are sampled and smoothed using a sliding window. The average is mapped to servo angles using:

```cpp
angle = map(avgValue, 50, 400, 180, 0);  // Adjust thresholds as needed
