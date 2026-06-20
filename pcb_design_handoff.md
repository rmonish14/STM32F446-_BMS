# 🛠️ STM32F446RE BMS - PCB Design Handoff Document

This document translates the hand-drawn pinouts, wiring diagrams, and analog conditioning circuits into professional schematic design specifications and layout instructions for the PCB engineering team.

---

## 🔌 1. System Power Architecture & Rails

The BMS operates across three voltage domains. It is critical to isolate high-noise power paths from the MCU's analog reference.

1.  **V_BAT (Pack Voltage: 12.0V – 16.8V):** Direct connection to the 4S battery pack. Powers the main isolation relay, cell protection relays, cooling fan, and input to the onboard regulator.
2.  **5.0V Rail (Logic & Sensors):** Generated via a high-efficiency buck converter or robust LDO from `V_BAT`. Powers:
    *   16x2 LCD display backlight and backpack (`5V` / `GND`).
    *   MQ-7 Gas Sensor heater/logic (`5V` / `GND`).
    *   ACS758 current sensor (`5V` / `GND`).
    *   Active 5V warning Buzzer (optional, if using a 5V model).
3.  **3.3V Rail (MCU & Low-Voltage Logic):** Generated via a low-noise LDO (e.g., AP2112K-3.3) from the 5.0V rail. Powers:
    *   STM32F446RE MCU.
    *   ILI9341 TFT Display (`VCC` & `LED` backlight).
    *   MPU6050 Accelerometer (`3.3V`).
    *   DS18B20 Temperature Sensors (`3.3V`).
    *   ESP-01S Wi-Fi module (Requires a dedicated high-current 3.3V LDO, e.g., AMS1117-3.3, capable of handling $\ge 500\text{mA}$ current spikes).

---

## 📌 2. Pin Assignment Master Table

| STM32 Pin | LQFP64 Pin | Port/Mode | Target Component | Description / Signal Characteristics |
| :---: | :---: | :---: | :---: | :--- |
| **PC1** | **9** | `ADC1_IN11` | Cell 1 Tap | Cell 1 voltage divider tap ($0\text{V} - 4.2\text{V}$). |
| **PC2** | **10** | `ADC1_IN12` | Cell 2 Tap | Cell 2 voltage divider tap ($0\text{V} - 8.4\text{V}$). |
| **PC3** | **11** | `ADC1_IN13` | Cell 3 Tap | Cell 3 voltage divider tap ($0\text{V} - 12.6\text{V}$). |
| **PC4** | **24** | `ADC1_IN14` | Cell 4 Tap | Cell 4 voltage divider tap ($0\text{V} - 16.8\text{V}$). |
| **PC5** | **25** | `ADC1_IN15` | ACS758 Out | Current sensor output ($0.6\text{V} - 2.5\text{V}$). |
| **PC0** | **8** | `ADC1_IN10` | MQ-7 Out | Gas sensor analog output ($0\text{V} - 5\text{V}$). |
| **PB6** | **58** | `I2C1_SCL` | 16x2 LCD & MPU6050 | Shared I2C Clock line. |
| **PB7** | **59** | `I2C1_SDA` | 16x2 LCD & MPU6050 | Shared I2C Data line. |
| **PB8** | **61** | GPIO (1-Wire) | Temp Sensor 1 (DS18B20) | One-Wire bus for Temp 1. |
| **PB9** | **62** | GPIO (1-Wire) | Temp Sensor 2 (DS18B20) | One-Wire bus for Temp 2. |
| **PC10** | **51** | `USART3_TX` | ESP-01S RX | UART TX to Wi-Fi. |
| **PC11** | **52** | `USART3_RX` | ESP-01S TX | UART RX from Wi-Fi. |
| **PA5** | **21** | `SPI1_SCK` | ILI9341 SCK | SPI Clock to TFT display. |
| **PA7** | **23** | `SPI1_MOSI` | ILI9341 MOSI | SPI Data to TFT display. |
| **PA4** | **20** | GPIO Output | ILI9341 CS | Chip Select (Active Low). |
| **PA6** | **22** | GPIO Output | ILI9341 D/C | Data / Command control line. |
| **PA1** | **15** | GPIO Output | ILI9341 RST | Display Reset line (Active Low). |
| **PB0** | **26** | GPIO Output | Cell 1 Relay | Low-side driver for Cell 1 relay. |
| **PB1** | **27** | GPIO Output | Cell 2 Relay | Low-side driver for Cell 2 relay. |
| **PB4** | **56** | GPIO Output | Cell 3 Relay | Low-side driver for Cell 3 relay. |
| **PB5** | **57** | GPIO Output | Cell 4 Relay | Low-side driver for Cell 4 relay. |
| **PB12** | **33** | GPIO Output | Fan Relay | Low-side driver for cooling fan relay. |
| **PB13** | **34** | GPIO Output | Isolation Relay | Low-side driver for pack isolation relay. |
| **PB14** | **35** | GPIO Output | Green Status LED | System healthy status light. |
| **PB15** | **36** | GPIO Output | Red Warning LED | Alarm/Warning status light. |
| **PB10** | **29** | GPIO Output | Buzzer | Alarm Buzzer output. |
| **PA10** | **43** | GPIO Input | Page Button | Page selection button (Active Low). |
| **PA2** | **16** | `USART2_TX` | PC Serial Debug | Connects to PC debug console. |
| **PA3** | **17** | `USART2_RX` | PC Serial Debug | Connects to PC debug console. |
| **PC13** | **2** | GPIO Input | User Button 1 | Alternate page toggle button (Active Low). |

### 🔌 Power & System Support Connections (LQFP64)
*   **VDD (3.3V Digital Power):** Pins 19, 32, 48, 64 (require a 100nF decoupling capacitor close to each pin).
*   **VSS (Digital Ground):** Pins 18, 31, 47, 63.
*   **VDDA (3.3V Analog Power):** Pin 13 (analog supply for ADC. Filter with a ferrite bead and decoupling capacitor).
*   **VSSA (Analog Ground):** Pin 12 (analog ground return path. Connect to system ground at a single star point).
*   **VBAT (Backup Power):** Pin 1 (connect to VDD if unused).
*   **BOOT0 (Boot Select):** Pin 60 (pull down to GND using a 10k resistor).
*   **NRST (Reset):** Pin 7 (connect to 3.3V via 10k pull-up and 100nF capacitor to GND).
*   **VCAP_1 (Internal Regulator Cap):** Pin 30 (must connect to a 4.7µF low-ESR ceramic capacitor to GND).

---

## 🎨 3. Schematic Sub-Circuit Specifications

### A. Cell Voltage Measurement (PC1, PC2, PC3, PC4)
The cells are stacked in series (up to 16.8V). To measure them with the STM32's 0V - 3.3V ADC range, we use precise voltage dividers with low-pass RC filters:

```
Cell Tap (Cx) ───[ R_top ]───┬───[ R_filter ]─── Analog Pin (PCx)
                             │
                         [R_bottom] ───[ C_filter ]
                             │               │
                         STM32 GND ────── STM32 GND
```

#### Divider Values (use 0.1% or 1% tolerance resistors):
*   **Cell 1 (Max 4.2V):** $R_{\text{top}} = 4.48\text{ k}\Omega$, $R_{\text{bottom}} = 10\text{ k}\Omega$.
*   **Cell 2 (Max 8.4V):** $R_{\text{top}} = 21.0\text{ k}\Omega$, $R_{\text{bottom}} = 10\text{ k}\Omega$.
*   **Cell 3 (Max 12.6V):** $R_{\text{top}} = 31.5\text{ k}\Omega$, $R_{\text{bottom}} = 10\text{ k}\Omega$.
*   **Cell 4 (Max 16.8V):** $R_{\text{top}} = 45.1\text{ k}\Omega$, $R_{\text{bottom}} = 10\text{ k}\Omega$.
*   **Low-pass Filter:** Place $R_{\text{filter}} = 1\text{ k}\Omega$ and $C_{\text{filter}} = 100\text{nF}$ (ceramic capacitor) as close to the STM32 pins as possible to filter out high-frequency noise from switching cells.

---

### B. Current Sensor (ACS758 to PC5)
The ACS758 current sensor runs on **5.0V** and outputs a voltage centered around $2.5\text{V}$ (range $0\text{V} - 5\text{V}$).
*   **Voltage Divider:** Scale this down to the $0\text{V} - 2.5\text{V}$ range to protect the STM32 input using a symmetric divider: $R_{\text{top}} = 10\text{ k}\Omega$ and $R_{\text{bottom}} = 10\text{ k}\Omega$.
*   **Low-pass Filter:** Place a $100\text{nF}$ filter capacitor across $R_{\text{bottom}}$ to stabilize the ADC readings.

---

### C. Shared I2C Bus (PB6, PB7)
The I2C1 bus is shared between the **3.3V MPU6050** and the **5V 16x2 LCD backpack (PCF8574)**.
*   **Logic Level Shifter:** You **MUST** implement a bi-directional logic level shifter (e.g., using BSS138 N-channel MOSFETs) between the STM32 (3.3V side) and the LCD backpack (5V side).
*   **MPU6050 Address pin (AD0):** Connect `AD0` directly to `GND` to hardcode the I2C address to `0x68`.
*   **Pull-up Resistors:** Connect $4.7\text{ k}\Omega$ pull-up resistors to `3.3V` on the 3.3V side, and to `5V` on the 5V side.

---

### D. DS18B20 Temp Sensors (PB8, PB9)
*   Connect a $4.7\text{ k}\Omega$ pull-up resistor from each data line (`PB8` and `PB9`) to `3.3V`.

---

### E. Gas Sensor (MQ-7 to PC0)
The MQ-7 outputs a $0\text{V} - 5\text{V}$ analog signal.
*   **Voltage Divider:** Scale it down using $R_{\text{top}} = 10\text{ k}\Omega$ and $R_{\text{bottom}} = 10\text{ k}\Omega$ (gives $0\text{V} - 2.5\text{V}$ to the STM32 pin `PC0`).
*   **Low-pass Filter:** Add a $100\text{nF}$ capacitor in parallel with $R_{\text{bottom}}$.

---

### F. ESP-01S Wi-Fi Module (PC10, PC11)
The ESP-01S has a high peak current consumption ($\sim 300\text{mA}$) during transmissions.
*   **Power:** Feed it from a dedicated 3.3V LDO. Do **NOT** power it directly from the STM32 MCU internal regulator.
*   **Hardware Setup:**
    *   Connect `EN` (CH_PD) to `3.3V` through a $10\text{ k}\Omega$ pull-up resistor.
    *   Connect `RST` to `3.3V` through a $10\text{ k}\Omega$ pull-up resistor.
    *   Place a $10\mu\text{F}$ tantalum capacitor and a $100\text{nF}$ ceramic decoupling capacitor directly across the ESP-01S VCC and GND pins.

---

### G. Relays, Fan, & Buzzer Low-Side Drivers
For all relays (`PB0`, `PB1`, `PB4`, `PB5`, `PB12`, `PB13`) and the Buzzer (`PB10`):
*   **MOSFET Switch:** Implement a low-side N-channel MOSFET switch (e.g., 2N7002 or BSS138) to switch the load.
*   **Gate Resistors:** Place a $100\,\Omega$ resistor in series with the gate and a $10\text{ k}\Omega$ pull-down resistor from gate to ground to prevent floating gates.
*   **Protection Diode:** Place a flyback diode (e.g., 1N4148 or 1N4007) in parallel with each relay coil / electromagnetic buzzer coil to protect against inductive voltage spikes when turning off.

```
V_BAT/5V (Coil/Buzzer) ─────┬──────────────┐
                            │             [D] (Flyback Diode)
                        [Load] (Coil)      │
                            │ ─────────────┘
                            ├─── Drain
STM32 Pin ───[ 100R ]───────┼─── Gate (Low-Side N-Ch MOSFET)
                            └─── Source ─── GND
                                   │
                              [ 10k Pull-down ]
                                   │
                                  GND
```

---

## 📐 4. PCB Layout & Grounding Guidelines

1.  **Grounding System (Star Ground topology):**
    *   Create separate ground planes/traces: **Power Ground (PGND)** for relays, fan, and battery power paths; **Analog Ground (AGND)** for the voltage dividers, gas sensor divider, and current sensor divider; and **Digital Ground (DGND)** for the MCU, ESP-01S, and TFT.
    *   Tie all ground planes together at a single point (Star Ground) near the main battery input connector to prevent switching noise on PGND from shifting the reference voltage of the ADC.
2.  **Analog Routing:**
    *   Keep voltage divider traces from the battery cells to the MCU as short as possible. Route them far away from the noisy switching lines of the relays, fan, and I2C lines.
    *   Place the $1\text{ k}\Omega$ resistor and $100\text{nF}$ filter capacitors within 5mm of the STM32's ADC pins.
3.  **High-Current Paths:**
    *   Make sure the main battery connections and current flow paths through the ACS758 current sensor are designed with wide copper traces/pours (at least 60-80 mil for 15-20A, or use 2oz copper weight) to minimize resistance and heat generation.
