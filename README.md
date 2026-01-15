# ESP32 Wake-Up Light (Sunlight Emulation)

This project is an **ESP32-based wake-up light** designed to simulate a natural sunrise in order to mitigate the lack of daylight during Swedish winters. The light gradually increases brightness and color temperature to support wakefulness and circadian rhythm.

The system is implemented using **ESP-IDF** and **FreeRTOS**, with a focus on real-time behavior, modular embedded architecture, and reliable wireless control.

---

## Features
- Gradual sunrise-style light transitions
- Bluetooth Low Energy (BLE) control via mobile client
- Persistent configuration using Non-Volatile Storage (NVS)
- Modular, production-style firmware architecture
- Custom logging system for debugging and validation

---

## Technical Overview

- **Platform:** ESP32  
- **Framework:** ESP-IDF  
- **RTOS:** FreeRTOS  
- **Language:** C  

The firmware is structured around multiple FreeRTOS tasks and uses **queue-based inter-task communication** to ensure deterministic, non-blocking behavior.

### Bluetooth Low Energy
- Custom **GATT server**
- GAP and GATT event handling
- MTU configuration
- ESP32 Bluedroid stack
- Real-time control of light behavior (timing, brightness, transitions)

### Lighting & Control
- Uses **DALI lighting control protocol**
- Implemented structured control logic, addressing concepts, and timing constraints
- Researched and tuned **exponential dimming curves** so perceived brightness increases linearly

### Reliability & Resource Management
- Persistent settings via **NVS**
- Custom logging system used across all subsystems
- Tuned task stack sizes and priorities
- Clear separation of concerns between Bluetooth handling, data processing, shared queues, LED control, and system coordination

---

## Skills & Concepts Demonstrated
- Embedded systems development
- Real-time systems and FreeRTOS
- ESP32 and ESP-IDF
- BLE (GATT/GAP)
- RTOS task design and synchronization
- Lighting control concepts (DALI)
- LED and light spectrum research
- Memory and resource optimization
- Modular firmware architecture
- Production-style embedded C development

---

## Status
This project was developed as part of an embedded systems course and serves as a practical demonstration of real-time firmware design, wireless communication, and lighting control concepts.
