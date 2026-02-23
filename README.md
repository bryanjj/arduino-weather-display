# Arduino Weather Display

An Arduino UNO R4 WiFi project that fetches the current weather from the [Open-Meteo API](https://open-meteo.com/) and displays the temperature on the built-in 12x8 LED matrix.

## What it does

- Connects to WiFi and queries the Open-Meteo forecast API every 60 seconds
- Displays the current temperature as two large pixel-art digits
- Shows an arrow indicator positioned vertically between the daily low and high:
  - **Up arrow** — temperature is rising
  - **Down arrow** — temperature is falling

## Hardware

- [Arduino UNO R4 WiFi](https://store.arduino.cc/products/uno-r4-wifi)

## Dependencies

Install these via the Arduino Library Manager:

- [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

## Setup

1. Copy `arduino_secrets.h.example` to `arduino_secrets.h`
2. Fill in your WiFi credentials and location coordinates
3. Open `weather2.ino` in the Arduino IDE and upload to your board

## Configuration

In `weather2.ino` you can adjust:

- `REFRESH_SECS` — how often to poll the API (default: 60)
- `DISPLAY_DEGREE` — show a degree symbol dot (0 or 1)
- `unit` — temperature unit (`"fahrenheit"` or `"celsius"`)
- `tz` — timezone (URL-encoded, e.g. `"America%2FLos_Angeles"`)
