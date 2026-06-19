# STM32F446RE-based Battery Management System (BMS)

A robust, edge-based Battery Management System built on the **STM32F446RE Nucleo** platform. This project integrates precise 4-cell voltage measurement, bidirectional current sensing, dynamic State of Charge (SoC) estimation, a 16x2 local status LCD, and Wi-Fi telemetry using an ESP-01S module.

---

## 🚀 Key Features

*   **⚡ Dynamic Charge/Discharge State Machine:** Leverages the bidirectional nature of the current sensor (1.241V idle baseline). Instantly detects charging (sensor drops below baseline) and discharging (sensor rises above baseline) without lag or noise filtering issues.
*   **🔋 Precise SoC Estimation:** Calibrated to a 4-S Lithium-ion pack configuration where **14.8V = 0%** and **16.8V = 100%**. Calculations are mathematically rounded to ensure accurate percentages.
*   **📺 16x2 Character LCD (I2C):** Uses a PCF8574 I2C adapter with auto-address detection (`0x27` / `0x3F` write addresses `0x4E` / `0x7E`). Displays real-time cell voltages and automatically calculates/displays the battery pack's S configuration (4S, 3S, etc.). Displays a clear `NO CELL CONNECTED` screen when no cells are detected.
*   **📶 ESP-01S Wi-Fi Telemetry:** Connects autonomously via AT commands to a local network (SSID: `MONISH`, Pass: `12345678`) using USART3 on the ST Morpho header.
*   **🎨 TFT Display Dashboard:** Highlights battery telemetry (voltages, current, temperature, vibration, SoC, and status) along with real-time Wi-Fi connectivity feedback (`WIFI: CONNECTED` in green, `WIFI: DISCONNECTED` in amber).

---

## 🔌 Hardware Connections & Pin Mappings

### 1. ESP-01S Wi-Fi Module (USART3)
Due to `PB11` being internally dedicated to `VCAP1` (power regulation capacitor) on the LQFP64 package of the STM32F446RE, `USART3` is remapped to `PC10` and `PC11` on the ST Morpho header:

| ESP-01S Pin | STM32 Pin / Power | Morpho Header Pin | Description |
| :---: | :---: | :---: | --- |
| **VCC** | **3.3V** | CN7 Pin 16 / CN6 Pin 4 | 3.3V stable power supply |
| **GND** | **GND** | CN7 Pin 8 / CN7 Pin 22 | Ground reference |
| **RX** | **PC10** (TX) | CN7 Pin 1 | USART3_TX (AF7 Alternate Function) |
| **TX** | **PC11** (RX) | CN7 Pin 2 | USART3_RX (AF7 Alternate Function) |
| **EN (CH_PD)** | **3.3V** | CN7 Pin 16 / CN6 Pin 4 | Chip Enable (Pulled High) |
| **RST** | **3.3V** | CN7 Pin 16 / CN6 Pin 4 | Reset (Pulled High) |

### 2. 16x2 LCD with PCF8574 I2C Backpack (I2C1)
Connected to the standard I2C1 pins (shared with MPU6050):

| LCD/Backpack Pin | STM32 Pin | Arduino / Morpho Pin | Description |
| :---: | :---: | :---: | --- |
| **VCC** | **5V** | CN6 Pin 5 (5V) | 5V Power Supply |
| **GND** | **GND** | CN6 Pin 6 / CN7 Pin 8 | Ground reference |
| **SDA** | **PB7** | Arduino D12 / CN5 Pin 3 | I2C1 SDA (AF4) |
| **SCL** | **PB6** | Arduino D10 / CN5 Pin 2 | I2C1 SCL (AF4) |

### 3. Sensors, Relays & Interfaces
*   **Cell Voltages:** Cell 1 to 4 tap voltage dividers connected to ADC1 channels 11, 12, 13, and 14 on pins **PC1, PC2, PC3, and PC4**.
*   **Current Sensor:** ACS712-20A analog current sensor connected to ADC1 channel 15 on pin **PC5**.
*   **Carbon Monoxide (CO):** MQ-7 gas sensor analog output connected to ADC1 channel 10 on pin **PC0**.
*   **Temperature (T1 & T2):** DS18B20 One-Wire digital sensors connected to pins **PB8 and PB9** respectively (require external $4.7\text{ k}\Omega$ pull-up resistors).
*   **Vibration Sensor (VIB):** MPU6050 6-DOF IMU module connected on the I2C1 bus (shared SCL **PB6** and SDA **PB7**).
*   **Relays:** Active cooling relay controlled by **PB12**; main pack isolation relay controlled by **PB13** (driven via low-side MOSFET drivers).
*   **User Controls:** Screen page toggle buttons on **PC13** (onboard blue button) and **PA10** (external page cycle button).


---

## 🛠️ How to Compile & Flash

1.  **Open with STM32CubeIDE:**
    *   Import this project folder directly into your workspace (`File -> Import -> General -> Existing Projects into Workspace`).
2.  **Configuration (.ioc):**
    *   The `Think360_Caterpiller.ioc` file can be opened with STM32CubeMX to view pinouts and clock tree layouts (configured for high-speed external clock).
3.  **Build:**
    *   Select the build configuration (Debug/Release) and click the **Hammer** build icon.
    *   The compiler produces zero warnings and zero errors.
4.  **Flash:**
    *   Connect the Nucleo-F446RE board via USB.
    *   Flash the project using the precompiled binary in `Debug/Think360_Caterpiller.elf` or run a debug session inside the IDE.

---

## 📊 Telemetry Output Example
During runtime, the debug serial output on `USART2` (115,200 Baud) prints detailed BMS updates:

```log
[DEBUG] RAW ADC: 3385, 3233, 3524, 3565, CUR:1517 | PIN: 2.742V, 2.619V, 2.855V, 2.888V, CUR:1.229V, MQ7:0.327V
[CALC ] Pack: 16.06V | Curr: -0.23A | SoC: 63% | Status: DISCHARGING | Time: 26h 49m to empty | T1: 28.5C | T2: 28.3C | VIB: 0.03G | CO: 1.30PPM | C1: 3.97V | C2: 4.16V | C3: 3.80V | C4: 4.12V

Testing ESP-01S UART...
AT
OK
Connecting to WiFi AP: MONISH...
AT+CWJAP="MONISH","12345678"
WIFI CONNECTED
WIFI GOT IP
OK
Connected to MONISH successfully!
```
