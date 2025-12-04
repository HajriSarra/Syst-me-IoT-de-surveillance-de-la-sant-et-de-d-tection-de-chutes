IoT Health Monitoring & Fall Detection

ESP32-based IoT system for real-time patient monitoring. Tracks vital signs and provides alerts via a web dashboard and Telegram bot.

Features

Heart rate (BPM) and SpO₂ monitoring (MAX30105)

Body temperature measurement (MLX90614)

Fall detection capability

Web dashboard with live updates and PDF export

Telegram bot for remote monitoring and alerts

Hardware

ESP32 DevKit

MAX30105 sensor

MLX90614 infrared sensor

Optional: DS18B20 temperature sensor, LED/buzzer

Software & Libraries

Arduino IDE

Adafruit MLX90614, MAX30105, DallasTemperature, OneWire

UniversalTelegramBot, ArduinoJson, WiFi

Setup

Clone the repo and open main.ino in Arduino IDE

Configure WiFi, Telegram Bot token, and chat ID

Connect sensors to ESP32 (I2C: SDA 21, SCL 22; DS18B20: GPIO 4)

Upload code and open web dashboard via ESP32 IP
