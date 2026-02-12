# 🚀 Initial Release v1.0

This is the first stable release of the **M5Cardputer LoRa & GPS Test Firmware**.
Designed to be a standalone field diagnostic tool for the **Official M5Stack LoRa868 Module** (SX1262 + ATGM336H).

### 🌟 Key Features
* **Boot Diagnostics:** Automatic hardware check for LoRa and GPS chips on startup.
* **Power Control:** Toggle GPS power on/off (Press `P`) to save battery.
* **Precision GPS Monitor:** Real-time coordinates, altitude, speed, and UTC time.
* **Dual-Mode LoRa Terminal:**
    * *Chat Mode:* Type and send text messages.
    * *Command Mode:* Quick shortcuts for Range Testing (`PING`) and Geo-Location sharing (`GEO-BEACON`).
* **RF Sniffer:** Visual spectrum analyzer for 868MHz band.
* **Dynamic SF:** Switch Spreading Factors (SF7-SF12) on the fly.

### 📦 Hardware
* Device: M5Stack Cardputer
* Module: M5Stack LoRa868 Cap

### ⬇️ Installation
Download the source code and compile with PlatformIO.

Old release README.md
# 📡 Initial version v0.9 - M5Cardputer LoRa & GPS Field Tester
Vibe coded with Gemini Pro

**M5Cardputer LoRa & GPS Field Tester v0.9** is a robust, all-in-one firmware designed for the **M5Stack Cardputer** equipped with the **Official LoRa868 Module (SX1262 + ATGM336H)**.

Its primary purpose is to act as a **Field Testing & Diagnostics Tool** to validate the capabilities of the hardware cap. It allows users to verify GPS signal acquisition, test LoRa range with different Spreading Factors, and visualize radio spectrum activity.

---

## ⚡ Features

### 1. 🌍 GPS Navigator Mode
* Real-time display of **Latitude, Longitude, Altitude, and Speed (km/h)**.
* Satellite count and UTC Time synchronization.
* **Fix Status:** Visual indication of fix acquisition vs. searching.
* **Data Logging:** Logs NMEA data to USB Serial every 5 seconds.

### 2. 💬 LoRa Terminal & Chat
* **Real-time Chat:** Type messages using the Cardputer keyboard.
* **Dual-Mode Interface:** seamless switching between Typing and Command modes.
* **GeoBeacon:** One-press transmission of current GPS coordinates.
* **Range Test (Ping):** Sends a ping packet with current SF info to test signal reach.
* **Receive Log:** Displays RSSI (Signal Strength) and SNR (Signal-to-Noise Ratio) for incoming packets.

### **Due to lack of LoRa Clients in my zone, I need testers for the LoRa Terminal!**

### 3. 📉 LoRa Sniffer
* Visual **RSSI Spectrum Analyzer**.
* Real-time graph to detect radio activity and noise floor in the 868MHz band.
* Helps verify antenna performance and environmental interference.

---

## 🛠 Hardware Requirements

* **M5Stack Cardputer** (ESP32-S3)
* **M5Stack LoRa868 Module** (The "Cap" with SX1262 LoRa + ATGM336H GPS)

### 🔌 Pinout Configuration (Hardcoded)
| Component | Pin Function | GPIO |
| :--- | :--- | :--- |
| **LoRa (SX1262)** | CS | 5 |
| | RST | 3 |
| | IRQ | 4 |
| | BUSY | 6 |
| | SCK | 40 |
| | MISO | 39 |
| | MOSI | 14 |
| **GPS (ATGM336H)** | RX (ESP32) | 15 |
| | TX (ESP32) | 13 |

---

## 📖 User Manual & Controls

### Global Navigation
* **`G`**: Switch to **GPS Monitor**.
* **`L`**: Switch to **LoRa Terminal**.
* **`S`**: Switch to **RSSI Sniffer**.
* **`H`**: Open **On-Screen Help**.
* **`TAB`**: Cycle **Spreading Factor (SF)**.
    * *SF 7:* Fast speed, lower range.
    * *SF 9:* Balanced (Default).
    * *SF 12:* Slow speed, maximum range (Long distance).

### ⌨️ LoRa Terminal Controls
The terminal features a **Two-Stage Logic** to handle both typing and quick commands:

1.  **Typing Mode (Default):**
    * Type freely on the keyboard.
    * **`ENTER`**: Send the typed message.
    * **`ESC`** (or **`\``**): Switch to **Command Mode**.

2.  **Command Mode (Red Border):**
    * **`SPACE`**: Send **PING** (Range Test).
    * **`ENTER`**: Send **GeoBeacon** (Current GPS Coordinates).
    * *Press any letter key:* Return to Typing Mode.
    * *Press `ESC` again:* Exit to Main Menu.

---

## 💻 Installation (PlatformIO)

1.  Clone this repository.
2.  Open the folder in **VS Code** with the **PlatformIO** extension installed.
3.  Ensure your `platformio.ini` matches the board settings:

```ini
[env:m5stack-stamps3]
platform = espressif32
board = m5stack-stamps3
framework = arduino
lib_deps = 
    m5stack/M5Cardputer@^1.0.3
    m5stack/M5Unified@^0.2.5
    h2zero/NimBLE-Arduino@^1.4.1
    jgromes/RadioLib@^6.6.0
    mikalhart/TinyGPSPlus@^1.0.3
monitor_speed = 115200
build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -DARDUINO_USB_CDC_ON_BOOT=1
```

4. Build and Upload to your Cardputer.

### 📄 License
MIT License

Copyright (c) 2024 Mario Virgili

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
