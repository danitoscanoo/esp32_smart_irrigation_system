# 💧 ESP32 Smart Irrigation System (Telegram & WebServer)

![ESP32](https://img.shields.io/badge/Hardware-ESP32-blue?style=for-the-badge&logo=espressif)
![C++](https://img.shields.io/badge/Language-C++-00599C?style=for-the-badge&logo=c%2B%2B)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge&logo=github)

## Project Overview
This repository contains a professional-grade, automated irrigation system designed to optimize water usage for home gardens. Controlled by an **ESP32**, the system manages three independent watering lines and features smart weather awareness to prevent unnecessary irrigation during rainfall. 

The project goes beyond simple timers by integrating **Telegram Bot control**, an internal **Web Server for real-time logging**, and robust system management including **NTP time synchronization** and a **Hardware Watchdog**.

## Key Features
- **Triple-Zone Control:** Independent management of 3 parallel solenoid valves via a relay module (GPIO 27, 32, 33).
- **Smart Rain Detection:** Reads a digital rain sensor (GPIO 34) and keeps a 24-hour history to inhibit scheduled watering if it has rained recently.
- **Telegram Bot Integration:** Fully controllable via smartphone. Send commands like `/valvola1_on` to manually trigger zones or check system logs remotely.
- **Local Web Server:** Hosts a dynamically updating webpage (Port 80) to monitor system logs and debugging info in real-time.
- **High Reliability:** Implements a Watchdog Timer (WDT) to automatically reboot the ESP32 in case of freezes or connectivity drops.
- **Single Power Source Design:** Integrated 240V to 5V (2A) transformer to power the ESP32, relays, and valves simultaneously.

## Bill of Materials (BOM)
| Component | Quantity | Description |
| :--- | :--- | :--- |
| **ESP32 Dev Board** | 1 | Main microcontroller with WiFi capabilities. |
| **Rain Sensor Module** | 1 | Digital output rain sensor module. |
| **3-Channel Relay Module** | 1 | 5V relay module (or a 4-channel board using 3 ports). |
| **5V Solenoid Valves** | 3 | Normally Closed (NC) valves. |
| **AC-DC Converter** | 1 | 240V AC to 5V DC (Min. 2A) step-down module. |

## Pinout & Wiring
| Component | Pin Type | ESP32 GPIO |
| :--- | :--- | :--- |
| **Relay 1 (Zone 1)** | Digital Out | `GPIO 27` |
| **Relay 2 (Zone 2)** | Digital Out | `GPIO 32` |
| **Relay 3 (Zone 3)** | Digital Out | `GPIO 33` |
| **Rain Sensor** | Digital In | `GPIO 34` |
| **System Power** | 5V & GND | `VIN / 5V` & `GND` |

--------------------------------------------------------------------------------------------------------------------------------------------------------------------

## ⚙️ How to Install and Run

### Prerequisites
* **Hardware:** ESP32 Development Board, relay module, solenoid valves, rain sensor, and power supply (see the BOM above).
* **Software:** [Arduino IDE](https://www.arduino.cc/en/software) (recommended for beginners) or [PlatformIO](https://platformio.org/).
* **ESP32 Core:** Ensure you have the ESP32 board manager installed in your IDE.

### Step-by-Step Guide

1. **Clone the Repository**
   Open your terminal and run the following command:
   ```bash
   git clone [https://github.com/YourUsername/esp32-smart-irrigation.git](https://github.com/YourUsername/esp32-smart-irrigation.git)
   ```

2. **Install Required Libraries**
   Open the Arduino IDE, navigate to *Sketch > Include Library > Manage Libraries*, and install the following dependencies:
   * `NTPClient` (by Fabrice Weinberg)
   * `UniversalTelegramBot` (by Brian Lough)
   * `ArduinoJson` (by Benoit Blanchon)

3. **Configure Your Credentials (Crucial Step)**
   To keep your network and bot credentials secure, this project uses a separate secrets file.
   * Create a new file named `secrets.h` in the same folder as your `.ino` sketch.
   * Add the following code and replace the placeholders with your actual data:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   #define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
   String CHAT_ID = "YOUR_TELEGRAM_CHAT_ID";
   ```
   * *Note: If you are forking or contributing to this repo, make sure `secrets.h` is included in your `.gitignore` file.*

4. **Hardware Setup**
   Wire your components according to the electrical schematic provided in the section above. Double-check all connections, especially the 5V and GND lines.

5. **Compile and Upload**
   * Connect your ESP32 to your computer via USB.
   * In Arduino IDE, go to *Tools > Board* and select **DOIT ESP32 DEVKIT V1** (or your specific model).
   * Select the correct COM port under *Tools > Port*.
   * Click the **Upload** button.

6. **Monitor and Test**
   Once the upload is complete, open the **Serial Monitor** and set the baud rate to `115200`. You should see the boot logs, successful WiFi connection, NTP synchronization, and the Telegram bot initialization.

------------------------------------------------------------------------------------------------------------------------------------------------------------------

## 🤝 Contributing
This is a personal portfolio project, but suggestions, bug reports, or pull requests are always welcome!
