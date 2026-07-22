# 🌐 Smart Wireless Mesh Sensor Network

> **Academic Project Report · Embedded Systems (EC6304) · Semester 6**
> Department of Electrical and Information Engineering, University of Ruhuna, Sri Lanka
> *17th May 2026*

---

## 👥 Project Team

| Name | Index Number |
|------|-------------|
| Chathuranga H.A.G. | EG/2022/4975 |
| Deshabandu A.S. | EG/2022/4991 |
| Herath N.L. | EG/2022/5069 |
| Weerathunga T.B. | EG/2022/5407 |

---

## 📋 Table of Contents

- [Overview](#overview)
- [Problem Statement](#problem-statement)
- [Objectives](#objectives)
- [System Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Node Descriptions](#node-descriptions)
- [Technologies and Software](#technologies-and-software)
- [Communication Protocol](#communication-protocol)
- [Sensors and Specifications](#sensors-and-specifications)
- [Project Structure](#project-structure)
- [Data Packet Structure](#data-packet-structure)
- [Software Design and Flow](#software-design-and-flow)
- [Advantages and Limitations](#advantages-and-limitations)
- [Applications](#applications)
- [Conclusion](#conclusion)
- [References](#references)

---

## 📖 Overview

The **Smart Wireless Mesh Sensor Network** is a multi-node embedded system designed to perform reliable, real-time environmental monitoring over a distributed wireless mesh topology. Instead of a simple point-to-point or star configuration, nodes communicate through a **multi-hop relay chain**, allowing data to travel beyond the range of any single node.

Each node in the network is equipped with environmental sensors — temperature, humidity, gas/smoke, flame, and vibration. Data collected at each sensor node is wirelessly forwarded through intermediate relay nodes until it reaches a central **Base Station (Node C)**, where it is displayed on an LCD and/or PC serial monitor, with alerts generated for abnormal readings.

> **Technology Note:** While the project title references "PIC Microcontrollers," the implementation uses **ATmega328P** microcontrollers (the same chip found in Arduino Uno/Nano boards), programmed via the **Arduino IDE** in **Embedded C / Arduino C++**.

---

## ⚠️ Problem Statement

Traditional wireless communication systems used for environmental monitoring face several critical limitations:

- **Signal Attenuation:** Single-hop systems fail when nodes are placed far apart or behind obstacles.
- **Single Point of Failure:** Star topologies crash entirely if the central hub goes offline.
- **Limited Range:** Point-to-point RF communication is constrained to a few dozen metres in real-world environments.
- **Unreliable in Hostile Environments:** Industrial plants, agricultural fields, and disaster zones often have RF interference or physical obstructions.

This project solves these problems by implementing a **mesh relay architecture** where each node can forward data from its neighbors, effectively extending communication range and adding fault tolerance.

---

## 🎯 Objectives

### Main Objective
> Design and implement a smart wireless mesh sensor network capable of collecting environmental data from distributed sensor nodes and transmitting it reliably to a central monitoring station.

### Specific Objectives
- Enable communication between multiple sensor nodes across extended distances
- Extend communication range through **multi-hop transmission**
- Collect real-time environmental data: temperature, humidity, gas leakage, flame, and vibration
- Transmit sensor data wirelessly to a base station for monitoring and logging
- Generate **alerts** (via LEDs/buzzer) when abnormal conditions are detected
- Ensure reliable communication and fault tolerance within the network

---

## 🏗️ System Architecture

The system is organized into a **3-layer architecture** with a **linear mesh relay topology**:

```
+-------------------------------------------------------------+
|                    SYSTEM ARCHITECTURE                       |
|                                                              |
|  +----------+    RF Link     +----------+    RF Link    +----------+ |
|  |  NODE A  | -------------> |  NODE B  | ------------> |  NODE C  | |
|  | (Sensor) |                | (Relay)  |               |  (Base   | |
|  |          |                |          |               | Station) | |
|  +----------+                +----------+               +----------+ |
|                                                                        |
+-------------------------------------------------------------+
```

### Architecture Layers

| Layer | Description |
|-------|-------------|
| **Input Layer** | DHT11, MQ-2, Flame Sensor, Vibration Sensor — continuously collect environmental data |
| **Processing Layer** | ATmega328P microcontroller on each node — reads sensors, processes packets, routes data |
| **Output Layer** | Base station (Node C) connected to 16x2 LCD, PC serial monitor; generates buzzer/LED alerts |

### Communication Flow
```
NODE A  --[nRF24L01 @ 2.4GHz, Channel 76]-->  NODE B  --[nRF24L01]-->  NODE C
         Pipe Address: "NODEB"                           Pipe Address: "NODEC"
```

---

## 🔩 Hardware Components

| Component | Model / Spec | Function |
|-----------|-------------|----------|
| **Microcontroller** | ATmega328P (Arduino Uno/Nano) | Central processing unit for each sensor node |
| **Wireless RF Module** | nRF24L01 (2.4 GHz ISM band) | Wireless communication between nodes |
| **Temperature & Humidity Sensor** | DHT11 | Measures ambient temperature (°C) and relative humidity (%) |
| **Gas / Smoke Sensor** | MQ-2 | Detects LPG, propane, methane, smoke, and H2 gas leakage |
| **Flame Sensor** | IR Flame Sensor Module | Detects presence of fire (active-LOW output) |
| **Vibration Sensor** | SW-420 / Vibration Switch | Detects physical shocks and vibrations |
| **Display** | 16x2 LCD (I2C, addr 0x27) | Displays real-time sensor data in pages |
| **Voltage Regulator** | AMS1117-3.3V | Provides stable 3.3V supply to nRF24L01 modules |
| **Alert Peripherals** | Buzzer + LEDs | Visual/audible indicators for abnormal conditions |
| **Communication Bus (Radio)** | SPI | ATmega328P <-> nRF24L01 communication |
| **Communication Bus (LCD)** | I2C | ATmega328P <-> LCD module communication |

---

## 🔵 Node Descriptions

### Node A — Sensor Node (Transmitter)
> **Role:** Collects raw environmental data and transmits it one-hop to Node B.

**Sensors connected:**
- DHT11 → Pin 2 (Digital, reads temperature only)
- MQ-2 Gas Sensor → Pin A0 (Analog, 10-bit ADC)
- Vibration Sensor → Pin 3 (Digital)
- Flame Sensor → Pin 4 (Digital, active-LOW)
- Status LED → Pin 5

**Behavior:**
1. Reads all sensor values every loop iteration
2. Packages data into a `SensorData` struct with `nodeID = 1`
3. Transmits via nRF24L01 to pipe address **"NODEB"** at 250 Kbps
4. Flashes LED during transmission
5. Waits 1 second before next reading

**Data sent:**
```c
struct SensorData {
  byte nodeID;      // Always 1 for Node A
  float temp;       // Temperature from DHT11 (degrees C)
  int   gas;        // Raw ADC value from MQ-2 (0-1023)
  int   vibration;  // 0 or 1 from vibration switch
  int   flame;      // 0 (fire) or 1 (no fire)
};
```

---

### Node B — Relay Node (Transceiver)
> **Role:** Acts as an intermediate relay — collects its own sensor data AND forwards aggregated data to Node C.

**Sensors connected:**
- DHT11 → Pin 2 (Digital, reads temperature AND humidity)
- MQ-2 Gas Sensor → Pin A0 (Analog, with exponential smoothing filter)
- Vibration Sensor → Pin 3 (Digital)
- Flame Sensor → Pin 4 (Digital, active-LOW)
- Status LED → Pin 6

**Behavior:**
1. 20-second warm-up delay at startup for MQ-2 gas sensor stabilization
2. Reads DHT11 (temperature + humidity), MQ-2, vibration, and flame sensors
3. Applies **exponential smoothing** to the gas ADC reading: `gasSmooth = (gasSmooth * 3 + newValue) / 4`
4. Logs all readings to Serial monitor (9600 baud) with threshold warnings
5. Packages data into `SensorData` struct with `nodeID = 2`
6. Transmits via nRF24L01 to pipe address **"NODEC"** with retry settings (15 retries, 15-step delay)
7. 1.5-second interval between transmissions

**Gas Threshold Alert:** > 400 ADC units triggers a "Gas Level HIGH!" serial warning.

**Data sent:**
```c
struct SensorData {
  byte  nodeID;     // Always 2 for Node B
  float temp;       // Temperature (degrees C)
  float hum;        // Humidity (%)
  int   gas;        // Smoothed ADC value from MQ-2
  int   vibration;  // 0 or 1
  int   flame;      // 0 (fire) or 1 (no fire)
};
```

---

### Node C — Base Station (Receiver)
> **Role:** Central receiver and display hub. Listens for incoming RF data and presents it on LCD and Serial monitor.

**Peripherals connected:**
- nRF24L01 RF Module → SPI (Pins 9, 10 for CE/CSN)
- 16x2 LCD → I2C at address 0x27 (uses `LiquidCrystal_I2C` library)
- Status LED → Pin 6

**Behavior:**
1. Initializes in **listening mode** on pipe address **"NODEC"** at 250 Kbps
2. Continuously polls `radio.available()` for incoming packets
3. On receiving data:
   - Flashes LED
   - Prints full sensor report to Serial monitor
   - Updates LCD with latest values
4. LCD cycles through **2 display pages** every 2 seconds:
   - **Page 0:** `T: <temp>  H: <hum>` / `G: <gas>`
   - **Page 1:** `V: <vibration>` / `F: <flame>`

---

## 💻 Technologies and Software

| Category | Technology | Details |
|----------|-----------|---------|
| **Programming Language** | Embedded C / Arduino C++ | Standard Arduino `.ino` sketches and pure Embedded C `.c` files |
| **Development Environment** | Arduino IDE | For compiling and uploading to ATmega328P boards |
| **Target Platform** | Arduino Uno / Nano (ATmega328P) | 8-bit AVR microcontroller @ 16 MHz |
| **Alternative Target** | STM32F103 (Blue Pill) | STM32 HAL implementations provided in `.c` files |
| **RF Library** | RF24 (TMRh20 fork) | nRF24L01 driver library for Arduino |
| **DHT Library** | DHT Sensor Library (Adafruit) | DHT11 / DHT22 driver |
| **LCD Library** | LiquidCrystal_I2C | I2C-based 16x2 LCD control |
| **Wire Library** | Wire (built-in Arduino) | I2C communication |
| **SPI Library** | SPI (built-in Arduino) | SPI communication for RF24 |
| **Version Control** | Git / GitHub | Source code repository |

### GitHub Repository
```
https://github.com/nethminalakshan/Smart-Wireless-Mesh-Sensor-Network-using-Pic-Microcontrollers.git
```

---

## 📡 Communication Protocol

### RF Module: nRF24L01+
The **nRF24L01** is a single-chip 2.4 GHz transceiver used as the wireless backbone of this mesh network.

| Parameter | Value |
|-----------|-------|
| Frequency Band | 2.4 GHz ISM |
| Channel | 76 (`radio.setChannel(76)`) |
| Data Rate | **250 Kbps** (`RF24_250KBPS`) — longer range, better penetration |
| Power Level | Low (`RF24_PA_LOW`) — suitable for short-range indoor use |
| Addressing | 5-byte pipe addresses: `"NODEB"`, `"NODEC"` |
| Node B Retries | 15 retries with 15-step delay |
| Interface | SPI — CE pin 9, CSN pin 10 on Arduino |
| Power Supply | **3.3V** (via AMS1117 regulator — NOT 5V tolerant!) |

### Transmission Path
```
Node A  --(TX)-->  "NODEB"  --(RX)-- Node B
Node B  --(TX)-->  "NODEC"  --(RX)-- Node C (Base Station)
```

### Bus Protocols Summary

| Bus | Used For | Pins (Arduino) |
|-----|---------|---------------|
| **SPI** | ATmega328P <-> nRF24L01 | 9 (CE), 10 (CSN), 11 (MOSI), 12 (MISO), 13 (SCK) |
| **I2C** | ATmega328P <-> LCD | A4 (SDA), A5 (SCL) |
| **Single-Wire** | ATmega328P <-> DHT11 | Digital Pin 2 |
| **Analog** | ATmega328P <-> MQ-2 | A0 (10-bit ADC) |
| **Digital GPIO** | ATmega328P <-> Flame/Vibration | Pins 3, 4 |

---

## 🔬 Sensors and Specifications

### DHT11 — Temperature and Humidity Sensor
| Parameter | Value |
|-----------|-------|
| Temperature Range | 0–50 degrees C |
| Temperature Accuracy | ±2 degrees C |
| Humidity Range | 20–90% RH |
| Humidity Accuracy | ±5% RH |
| Interface | Single-wire digital (custom protocol) |
| Operating Voltage | 3.3V–5V |
| Used In | Node A (temp only), Node B (temp + humidity) |

### MQ-2 — Gas and Smoke Sensor
| Parameter | Value |
|-----------|-------|
| Detectable Gases | LPG, Propane, Methane, Smoke, H2, Alcohol |
| Output | Analog (0–5V → 0–1023 ADC) |
| Warm-up Time | ~20 seconds (enforced in Node B startup) |
| Threshold | > 400 ADC units = HIGH gas alert |
| Smoothing | Exponential moving average applied in Node B |

### Flame Sensor (IR-based)
| Parameter | Value |
|-----------|-------|
| Detection Range | Up to 1 metre |
| Output | Digital (active-LOW: 0 = fire detected) |
| Wavelength Sensitivity | ~760–1100 nm (IR spectrum) |

### Vibration Sensor (SW-420 / equivalent)
| Parameter | Value |
|-----------|-------|
| Output | Digital (HIGH = vibration detected) |
| Sensitivity | Adjustable via onboard potentiometer |

---

## 📁 Project Structure

```
Senseor node project/
│
├── Arduino/                        # Arduino IDE sketches (primary implementation)
│   ├── a_ard.ino                   # Node A - Sensor node (transmitter)
│   ├── b_ard.ino                   # Node B - Relay node (transceiver)
│   └── c_ard.ino                   # Node C - Base station (receiver + display)
│
├── Node_A/
│   └── Node_A.c                    # Embedded C equivalent of Node A (STM32 ready)
│
├── Node_B/
│   └── Node_B.c                    # Embedded C equivalent of Node B (STM32 ready)
│
├── Node_C/
│   └── Node_c.c                    # Embedded C equivalent of Node C (STM32 ready)
│
├── Smart Wireless Mesh Sensor Network using Pic Microcontrollers/
│   ├── Node_A.c                    # (Git-versioned source)
│   ├── Node_B.c
│   └── Node_c.c
│
├── 2026_16_Wireless_Mesh_Sensor_Network_Report.pdf   # Full project report
└── README.md                       # This file
```

### File Descriptions

| File | Description |
|------|-------------|
| `Arduino/a_ard.ino` | Node A Arduino sketch — reads DHT11 (temp), MQ-2 (gas), vibration, flame; transmits to Node B |
| `Arduino/b_ard.ino` | Node B Arduino sketch — reads all 4 sensors + humidity; applies gas smoothing; relays to Node C |
| `Arduino/c_ard.ino` | Node C Arduino sketch — receives data; displays on 16x2 I2C LCD with 2-page rotation |
| `Node_A.c` | Embedded C port for bare-metal MCUs (e.g., STM32F103) — no Arduino framework dependency |
| `Node_B.c` | Embedded C port with full DHT11 bit-banging, ADC, SPI, UART implementations |
| `Node_c.c` | Embedded C port with I2C LCD driver and RF24 receiver implementation |

---

## 📦 Data Packet Structure

All nodes share a common struct for data transmission. The struct must be **identical** across all nodes for correct deserialization:

```c
// Node A -> Node B packet
struct SensorData {
  byte  nodeID;     // Node identifier (1 = Node A, 2 = Node B)
  float temp;       // Temperature in degrees C
  int   gas;        // MQ-2 ADC value (0-1023)
  int   vibration;  // 0 = no vibration, 1 = detected
  int   flame;      // 1 = no fire, 0 = FIRE DETECTED (active-LOW)
};

// Node B -> Node C packet (expanded)
struct SensorData {
  byte  nodeID;     // 2 = Node B
  float temp;       // Temperature in degrees C
  float hum;        // Relative humidity in %
  int   gas;        // Smoothed MQ-2 ADC value
  int   vibration;  // 0 or 1
  int   flame;      // 0 or 1 (active-LOW: 0 means fire)
};
```

---

## 🔄 Software Design and Flow

### Node A — Operation Flow
```
START -> GPIO_Init -> DHT_Init -> SPI_Init -> RF24_Init (TX mode to "NODEB")
   |
[LOOP]
   |
Read DHT11 (temp) -> Read MQ-2 ADC -> Read Vibration -> Read Flame
   |
Pack into SensorData struct (nodeID=1)
   |
LED ON -> radio.write() -> delay(100ms) -> LED OFF -> delay(1000ms)
   |
[REPEAT]
```

### Node B — Operation Flow
```
START -> Init all peripherals -> warm-up delay (20s for MQ-2)
   |
[LOOP]
   |
Read DHT11 (temp + hum) -> Read MQ-2 -> Read Vibration -> Read Flame
   |
Apply gas smoothing: gasSmooth = (gasSmooth*3 + gasValue) / 4
   |
Serial print readings + threshold alerts
   |
Pack into SensorData struct (nodeID=2)
   |
LED ON -> radio.write() to "NODEC" -> delay(50ms) -> LED OFF -> delay(1500ms)
   |
[REPEAT]
```

### Node C — Operation Flow
```
START -> GPIO_Init -> SPI_Init -> I2C_Init -> LCD_Init -> RF24_Init (RX mode on "NODEC")
   |
[LOOP]
   |
radio.available()? --- NO ---> [Check LCD page timer]
      |                                |
     YES                         Update LCD (page 0 or page 1)
      |                                |
radio.read() -> data struct       delay(50ms)
      |                                |
LED ON                            LED OFF
Print to Serial                        |
Update LCD                        [REPEAT]
      |
[REPEAT]
```

---

## ✅ Advantages and ❌ Limitations

### Advantages
- **Improved Reliability** — Mesh relay eliminates single-hop range limits
- **Extended Range** — Multi-hop transmission significantly increases effective network coverage
- **Fault Tolerance** — Even if one node temporarily fails, the system can potentially re-route
- **Low Cost** — Uses readily available, inexpensive hardware (Arduino + nRF24L01)
- **Scalable Design** — Additional relay nodes can be added to extend range further
- **Real-time Monitoring** — Continuous sensor readings with alert generation

### Limitations
- **RF Interference** — 2.4 GHz band may experience interference from Wi-Fi, Bluetooth devices
- **Increased Power Consumption** — More nodes = higher total system power draw
- **Limited Throughput** — 250 Kbps is much slower compared to Wi-Fi or Zigbee alternatives
- **Linear Topology** — Current implementation is a chain (A->B->C), not a full mesh
- **Environmental Obstacles** — Walls, metal structures reduce RF signal quality
- **DHT11 Accuracy** — DHT11 has limited precision; DHT22 would improve accuracy

---

## 🌍 Applications

This project is applicable in a wide range of real-world monitoring scenarios:

| Domain | Use Case |
|--------|---------|
| **Industrial Safety** | Factory floor monitoring for gas leaks, fire, and abnormal vibrations in machinery |
| **Agriculture** | Monitoring temperature, humidity, and soil conditions across large fields |
| **Smart Buildings** | HVAC, fire safety, and structural health monitoring in commercial buildings |
| **Disaster Management** | Early-warning systems for fires, gas leaks, and structural disturbances |
| **Security Systems** | Vibration-based intrusion detection combined with environmental monitoring |
| **Environmental Research** | Multi-point environmental data logging for research stations |

---

## 🔮 Future Work

- **IoT Integration** — Connect Node C to the internet via ESP8266/ESP32 Wi-Fi module for cloud data logging
- **Full Mesh Topology** — Implement true mesh routing (any node can relay to any other node)
- **Mobile Dashboard** — Real-time web/mobile dashboard for remote monitoring
- **Battery Optimization** — Implement deep sleep modes between sensor readings for battery-powered deployment
- **Advanced Sensors** — Integrate CO2, pressure, or PM2.5 particulate sensors
- **OTA Updates** — Over-the-air firmware updates for deployed nodes

---

## 🏁 Conclusion

The Smart Wireless Mesh Sensor Network successfully demonstrates how low-cost embedded hardware can be combined with wireless RF communication to create a reliable, multi-hop environmental monitoring system. The 3-node chain architecture proves the viability of mesh relay techniques for extending sensor network coverage beyond single-hop limitations. The project was developed as part of Semester 6 coursework at the Department of Electrical and Information Engineering, University of Ruhuna.

---

## 📄 References

1. Nordic Semiconductor — nRF24L01+ Product Specification v1.0
2. Aosong Electronics — DHT11 Humidity and Temperature Sensor Datasheet
3. Arduino Reference Documentation — https://www.arduino.cc/reference
4. TMRh20 — RF24 Library for Arduino, GitHub
5. Microchip Technology — ATmega328P Datasheet
6. 2026_16_Wireless_Mesh_Sensor_Network_Report.pdf — Project Report, University of Ruhuna, 2026

---

*Smart Wireless Mesh Sensor Network — Department of Electrical and Information Engineering, University of Ruhuna, 2026*
