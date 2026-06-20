# STM32F446RE BMS - PCB Pin Mapping & Design Guide

This guide compiles all STM32 pin configurations, peripheral roles, voltage constraints, and interface circuits required to design a custom PCB for the Battery Management System.

---

## 📌 1. STM32 Pin Assignment Master Table

The following table lists all active pins on the STM32F446RE (LQFP64 package) used in the BMS firmware, including their physical IC pin numbers:

| Pin Name | LQFP64 Pin | Function / Mode | Connected Component | Description / Signal Characteristics |
|:---:|:---:|---|---|---|
| **PC1** | **9** | `ADC1_IN11` (Analog) | **Cell 1 Voltage Divider** | Battery Cell 1 tap voltage (0V to 4.2V scaled down). |
| **PC2** | **10** | `ADC1_IN12` (Analog) | **Cell 2 Voltage Divider** | Battery Cell 2 tap voltage (0V to 8.4V scaled down). |
| **PC3** | **11** | `ADC1_IN13` (Analog) | **Cell 3 Voltage Divider** | Battery Cell 3 tap voltage (0V to 12.6V scaled down). |
| **PC4** | **24** | `ADC1_IN14` (Analog) | **Cell 4 Voltage Divider** | Battery Cell 4 tap voltage (0V to 16.8V scaled down). |
| **PC5** | **25** | `ADC1_IN15` (Analog) | **ACS758 Sensor** | Current sensor analog voltage. Must be divided down to 3.3V range. |
| **PC0** | **8** | `ADC1_IN10` (Analog) | **MQ-7 Gas Sensor** | CO Gas sensor analog voltage. Must be divided down to 3.3V range. |
| **PB6** | **58** | `I2C1_SCL` (AF4) | **16x2 LCD & MPU6050** | I2C Clock line. Shared between local display and accelerometer. |
| **PB7** | **59** | `I2C1_SDA` (AF4) | **16x2 LCD & MPU6050** | I2C Data line. Shared between local display and accelerometer. |
| **PB8** | **61** | GPIO (One-Wire) | **DS18B20 Temp 1 (T1)** | Battery pack temperature sensor 1. |
| **PB9** | **62** | GPIO (One-Wire) | **DS18B20 Temp 2 (T2)** | Battery pack temperature sensor 2. |
| **PC10**| **51** | `USART3_TX` (AF7) | **ESP-01S RX** | UART TX line to Wi-Fi module (3.3V logic level). |
| **PC11**| **52** | `USART3_RX` (AF7) | **ESP-01S TX** | UART RX line from Wi-Fi module (3.3V logic level). |
| **PA5** | **21** | `SPI1_SCK` (AF5) | **ILI9341 SCK** | SPI Clock to TFT display. |
| **PA7** | **23** | `SPI1_MOSI` (AF5)| **ILI9341 MOSI**| SPI Data to TFT display. |
| **PA4** | **20** | GPIO (Output) | **ILI9341 CS** | Chip Select for TFT (Active Low). |
| **PA6** | **22** | GPIO (Output) | **ILI9341 D/C** | Data/Command select for TFT. |
| **PA1** | **15** | GPIO (Output) | **ILI9341 RST** | Hardware Reset for TFT (Active Low). |
| **PB12**| **33** | GPIO (Output) | **Cooling Fan Relay** | Controls the active cooling fan. Drive via MOSFET. |
| **PB13**| **34** | GPIO (Output) | **Isolation Relay** | Controls the main pack isolation relay. Drive via MOSFET. |
| **PB0** | **26** | GPIO (Output) | **Cell 1 Protection Relay** | Disconnects/protects Cell 1. Drive via MOSFET. |
| **PB1** | **27** | GPIO (Output) | **Cell 2 Protection Relay** | Disconnects/protects Cell 2. Drive via MOSFET. |
| **PB4** | **56** | GPIO (Output) | **Cell 3 Protection Relay** | Disconnects/protects Cell 3. Drive via MOSFET. |
| **PB5** | **57** | GPIO (Output) | **Cell 4 Protection Relay** | Disconnects/protects Cell 4. Drive via MOSFET. |
| **PB14**| **35** | GPIO (Output) | **Green Indicator LED** | Normal/Healthy state light (Active High). |
| **PB15**| **36** | GPIO (Output) | **Red Indicator LED** | Alarm/Warning/Critical state light (Active High). |
| **PB10**| **29** | GPIO (Output) | **Buzzer** | Active High warning/critical buzzer. |
| **PC13**| **2** | GPIO (Input) | **User Button 1** | Page toggle button (Active Low, external/internal pull-up). |
| **PA10**| **43** | GPIO (Input) | **User Button 2** | Page toggle button (Active Low, internal pull-up). |
| **PA2** | **16** | `USART2_TX` (AF7) | **PC Serial Debug** | Connects to virtual COM port via ST-LINK for logging. |
| **PA3** | **17** | `USART2_RX` (AF7) | **PC Serial Debug** | Connects to virtual COM port via ST-LINK for logging. |

### 🔌 Power and System Pins Reference (LQFP64)
*   **VDD (3.3V Digital Power):** Pins 19, 32, 48, 64 (connect a 100nF decoupling capacitor to each pin).
*   **VSS (Digital Ground):** Pins 18, 31, 47, 63.
*   **VDDA (3.3V Analog Power):** Pin 13 (analog supply for ADC. Filter with a ferrite bead and decoupling capacitor).
*   **VSSA (Analog Ground):** Pin 12 (analog ground return path. Connect to system ground at a single star point).
*   **VBAT (Backup Power):** Pin 1 (connect to VDD if unused).
*   **BOOT0 (Boot Select):** Pin 60 (pull down to GND using a 10k resistor).
*   **NRST (Reset):** Pin 7 (connect to 3.3V via 10k pull-up and 100nF capacitor to GND).
*   **VCAP_1 (Internal Regulator Cap):** Pin 30 (must connect to a 4.7µF low-ESR ceramic capacitor to GND).


---

## ⚡ 2. Signal Conditioning & External Circuit Guidelines

### A. Cell Voltage Measurement (Resistor Divider Networks)
Since the ADC inputs are restricted to **0V - 3.3V**, you must scale the cell tap voltages using voltage dividers. The firmware is configured so that **all 4 cell taps use identical voltage dividers** to allow any tap to read up to 16.8V max. The scale factor is calculated as:

$$V_{tap} = V_{pin} \times \text{Scale Factor}$$

*   **Uniform Cell Taps 1, 2, 3, and 4 (Max 16.8V - 18.0V range):** `SCALE_B1..B4 = 5.5455`
    *   *Ratio:* $R_{\text{top}}/R_{\text{bottom}} = 4.5455$
    *   *Suggested resistors:* $R_{\text{top}} = 100.0\text{ k}\Omega$ (1% tolerance), $R_{\text{bottom}} = 22.0\text{ k}\Omega$ (1% tolerance).

> [!WARNING]
> Use 1% precision metal film resistors for the dividers. Calibrate scaling factors in software if your specific resistor choices vary slightly from these target values to maintain ADC accuracy.

---

### B. I2C Bus Pull-Ups (PB6 / PB7)
The I2C1 bus has both the **16x2 LCD** (using PCF8574 Backpack) and the **MPU6050** Accelerometer connected.
*   **Pull-up Resistors:** Connect a $4.7\text{ k}\Omega$ pull-up resistor from `PB6` to `3.3V` and from `PB7` to `3.3V`.
*   **Level Shifting (Important):** The PCF8574 operates at **5V** while the MPU6050 and STM32 operate at **3.3V**. Ensure you use a bi-directional I2C logic level shifter (such as BSS138-based level shifting circuits) to bridge the 3.3V side (STM32, MPU6050) and the 5V side (PCF8574).

---

### C. One-Wire Bus Pull-Ups (PB8 / PB9)
The DS18B20 temperature sensors require a pull-up resistor to function in normal power mode.
*   Connect a $4.7\text{ k}\Omega$ pull-up resistor between the data line and `3.3V` for each sensor (`PB8` and `PB9`).

---

### D. Current Sensor Signal Conditioning (PC5)
The ACS712 current sensor is powered by **5V** and outputs an analog voltage in the range of **0V - 5V** (centered around $2.5\text{V}$ for 0A current).
*   **Voltage Divider:** You must step down the ACS712 output to the **0V - 3.3V** range. Use a symmetric divider: $R_{\text{top}} = 10\text{ k}\Omega$ and $R_{\text{bottom}} = 10\text{ k}\Omega$.
*   This cuts the voltage in half ($0\text{V} - 2.5\text{V}$ range, centered around $1.25\text{V}$). This matches the baseline value of `ACS712_ZERO_VOLTAGE = 1.241f` and sensitivity of `0.050f` (50mV/A) declared in the code.

---

### E. Gas Sensor Signal Conditioning (PC0)
The MQ-7 carbon monoxide sensor requires a **5V** heater cycle and outputs an analog signal up to **5V**.
*   **Voltage Divider:** Like the ACS712, scale the output down to the safe 3.3V ADC range using a $10\text{ k}\Omega$ / $10\text{ k}\Omega$ resistor divider before feeding it to `PC0`.

---

### F. ESP-01S Wi-Fi Module Power (PC10 / PC11)
The ESP-01S operates strictly at **3.3V** logic levels.
*   **Power Supply:** Do **NOT** power the ESP-01S directly from the STM32's onboard LDO output. Wi-Fi modules generate current spikes up to $300\text{mA}$ during transmissions, which can cause brownouts and reset the MCU. Design a dedicated regulator (e.g. `AMS1117-3.3` powered by your main 5V rail) with decoupling capacitors ($10\mu\text{F}$ tantalum + $100\text{nF}$ ceramic) close to the ESP-01S module.

---

### G. Relay Drivers (PB12 / PB13)
The GPIO pins of the STM32 can source/sink at most **25mA** and cannot drive inductive relay coils directly.
*   **Driver Circuit:** Implement a low-side N-channel MOSFET driver (e.g. `2N7002` or `BSS138`) or a Darlington transistor pair (ULN2003) to switch the relays.
*   **Flyback Diodes:** Place a flyback diode (e.g. `1N4148` or `1N4007`) in parallel with each relay coil to suppress inductive voltage spikes when switching off.

---

### H. Buzzer Driver (PB10)
Similar to relays, a buzzer (especially active buzzers that require 5V or 12V and draw substantial current) should not be driven directly from the GPIO pin.
*   **Driver Circuit:** Use a small NPN transistor (e.g., MMBT3904) or N-channel MOSFET (e.g., BSS138) to switch the buzzer from the 5V rail.
*   **Flyback Diode:** If using an electromagnetic buzzer, include a flyback diode (e.g., 1N4148) in parallel with the buzzer terminal to suppress voltage spikes.
