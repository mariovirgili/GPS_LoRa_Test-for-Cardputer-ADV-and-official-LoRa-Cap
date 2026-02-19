# 📡 M5Cardputer LoRa & GPS Test Firmware v1.1

A robust, standalone firmware designed for the **M5Stack Cardputer** equipped with the **Official LoRa868 Module (SX1262 + ATGM336H)**.

**Note:** This is a dedicated hardware diagnostic and field testing tool. It is *not* intended to be a substitute for mesh networking firmwares like Meshtastic. Its primary purpose is to validate hardware integrity, test LoRa range with different Spreading Factors, check GPS lock capabilities, and visualize radio spectrum activity.

---

## ⚡ Key Features (Updated in v1.1)

### 🛠️ Boot Diagnostics & Frequency Selection
* **Hardware Self-Test:** On startup, the firmware probes the SPI bus to verify the SX1262 chip is alive and responding. This immediately tells you if your hardware is fried, badly seated, or working perfectly.
* **Worldwide Frequencies:** After passing the hardware check, you can select your regional operating frequency (**433, 868, 915, or 923 MHz**). *(Note: The official M5Stack module antenna is tuned for 868MHz; using other frequencies works in software but will have reduced physical range).*

### 🌍 GPS Navigator & Power Control
* Real-time display of **Latitude, Longitude, Altitude, and Speed (km/h)** (2-decimal precision).
* Satellite count and UTC Time synchronization.
* **GPS Power Toggle:** Press `P` to turn the GPS module ON or OFF, saving precious battery life when you only need to use the radio.

### 💬 LoRa Terminal & Chat
* **Dual-Mode Interface:** Seamlessly switch between typing regular messages and sending quick commands.
* **GeoBeacon:** One-press transmission of your current GPS coordinates.
* **Range Test (Ping):** Send a ping packet to test signal reach.
* **Smart Feedback:** The top header provides visual confirmation (`SENDING PING...`, `SENDING GEO...`, `TX: SENDING...`).

### 📉 LoRa RF Sniffer
* Visual **RSSI Spectrum Analyzer**.
* Real-time scrolling graph to detect radio activity, bursts, and noise floor in your selected frequency band.

---

## 📖 User Manual & Controls

### Global Navigation
* **`G`**: Switch to **GPS Monitor**.
* **`L`**: Switch to **LoRa Terminal**.
* **`S`**: Switch to **RSSI Sniffer**.
* **`H`**: Open **On-Screen Help**.
* **`P`**: **Toggle GPS Power ON/OFF**.
* **`TAB`**: Cycle **Spreading Factor (SF)** (SF7, SF9, SF12).

### ⌨️ LoRa Terminal Controls (Dual-Stage)
1. **Typing Mode (Default):** Type freely and press **`ENTER`** to send. Press **`ESC`** (or **`\``**) to enter Command Mode.
2. **Command Mode (Red Border):** * **`SPACE`**: Send **PING** (Range Test).
   * **`ENTER`**: Send **GeoBeacon** (GPS Coordinates).
   * *Press any letter key:* Return to Typing Mode.
   * *Press `ESC` again:* Exit app to GPS Monitor.

---

## 🔌 Hardware & Pinout (Hardcoded)
* **Device:** M5Stack Cardputer (ESP32-S3)
* **Module:** M5Stack LoRa868 Cap

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

## 💻 Installation (PlatformIO)

Ensure your `platformio.ini` matches these stable settings:

```ini
[env:m5stack-stamps3]
platform = espressif32@6.6.0
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
    -DARDUINO_USB_MODE=1
```

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
