
# **DOCUMENTATION — Calm Handheld Computer**

*Open-source hardware and firmware for a screenless, local-first rhythmic communication device*

---

## **Table of Contents**

1. [Overview](#overview)
2. [Design Intent](#1-design-intent)
3. [How the Device Works](#2-how-the-device-works)
4. [Hardware](#3-hardware)
    3.1 [Bill of Materials](#31-bill-of-materials-laskakit--dratek)
    3.2 [Power](#32-power)
    3.3 [Rotary Encoder Wiring](#33-rotary-encoder-wiring-ky-040)
    3.4 [Vibration Driver](#34-vibration-motor-driver)
5. [Enclosure](#4-enclosure)
6. [Software](#5-software)
7. [Assembly Steps](#6-assembly-steps)
8. [Intended Use](#7-intended-use)
9. [Licensing](#licensing)

---

## **Overview**

The Calm Handheld Computer is a small, tactile object for non-verbal communication within a real-life social group.
No display. No notifications. No algorithms.

The interface is a rotary encoder; the language is rhythm.

Each rotation generates a sequence of pulses.
Each sequence is broadcast to all other devices via ESP-NOW.
Each receiving device vibrates through the same rhythm.

The result is a minimal, embodied communication system built around quiet presence.

[Arduino firmware](/firmware_v1)

---

# **1. Design Intent**

### **Calm computing**

Feedback stays peripheral.
The device never interrupts; it only produces soft pulses.

### **Belonging**

Designed for small groups.
Rhythmic “vocabularies” emerge naturally between people who know each other.

### **Hyperlocal design**

Nothing is general-purpose.
The device lives inside specific social spaces and shared contexts.
It avoids the patterns of commercial social media entirely.

---

# **2. How the Device Works**

### **Input model: rotation → sequence**

The KY-040 encoder produces a clean “click” per detent.
Each detent becomes an event.
A full sequence is confirmed by pressing the encoder switch.

The firmware captures:

* timing between pulses
* direction (optional in interpretation)

### **Output model: vibration pulse**

Each received pulse triggers a short vibration.
Feedback is immediate and ephemeral.

### **Rhythmic communication**

Instead of long/short signals, users create **patterns**:

* pulse count
* tempo
* spacing
* direction changes
* call-and-response sequences

Meaning emerges socially.

### **Group model**

All devices receive all broadcasts.
There is no addressing or pairing.
The group itself is the channel.

---

# **3. Hardware**

## **3.1 Bill of Materials (Laskakit / Dratek)**

| Component                          | Qty | Purpose                                   |
| ---------------------------------- | --- | ----------------------------------------- |
| **Seeed Studio XIAO ESP32-C6**     | 1   | Microcontroller (Wi-Fi radio for ESP-NOW) |
| **KY-040 rotary encoder**          | 1   | Primary input (rotation + press)          |
| **Vibrační minimotor 1027 3V**     | 1   | Haptic output                             |
| **YJ502040 Li-pol 3.7V / 320 mAh** | 1   | Power supply                              |
| **DIOTEC BC337-40BK (NPN)**        | 1   | Motor driver                              |
| **SEMTECH 1N4001W diode**          | 1   | Flyback protection                        |
| **1 kΩ metal film resistor**       | 1   | Transistor base resistor                  |
| Optional flat vibration motor      | —   | Alternate form factor                     |
| Wires, heat-shrink                 | —   |                                           |
| USB-C cable                        | 1   | Charging and programming                  |
| Hand-shaped enclosure              | 1   | Clay, cardboard, tape, resin, etc.        |

---

## **3.2 Power**

The XIAO ESP32-C6 includes everything needed:

* Li-Po connects directly to the on-board battery port
* USB-C powers and charges
* 3.3V regulator included

No external charging circuitry required.

---

## **3.3 Rotary Encoder Wiring (KY-040)**

| KY-040 | XIAO |
| ------ | ---- |
| +      | 3V3  |
| GND    | GND  |
| CLK    | D2   |
| DT     | D3   |
| SW     | D4   |

Quadrature is handled in software.

---

## **3.4 Vibration Motor Driver**

BC337 used as a low-side switch:

```
3.3V --- Motor +  
Motor – --- Diode (stripe toward motor +) --- BC337 collector  
BC337 emitter ---- GND  
BC337 base ---- 1 kΩ ---- GPIO D9  
```

GPIO **D9** controls all vibration.
The diode absorbs inductive kick.

---

# **4. Enclosure**

The enclosure is part of the interaction.
It should feel like a small, quiet object that fits naturally into a hand.

Key points:

* Warm and familiar shape
* Low visual presence
* Smooth edges, organic form
* USB-C access maintained
* Add vent holes above the port for airflow and moisture control

Vibration strength depends on how the motor couples to the shell.
Test before sealing.

---

# **5. Software**

### **ESP-NOW only**

No BLE.
No Wi-Fi network modes.

Devices broadcast sequences to all known peers stored in flash.

### **Sequence → packet**

A recorded sequence contains:

* pulse timestamps (ms)
* direction values
* sequence length

Direction does not affect vibration by default.

### **Packet propagation**

ESP-NOW delivers packets quickly within room/apartment distance.

### **Receiving**

Each sequence triggers:

* vibration pulses matching the rhythm
* LED feedback

There is no message queue or reliable ordering; this is intentional.

---

# **6. Assembly Steps**

### 1. Solder the XIAO pins

Use headers or direct wiring.

### 2. Wire the battery

Route the leads cleanly beneath the board.

### 3. Wire the rotary encoder

Connect CLK, DT, SW, +, and GND.

### 4. Build the motor driver

Assemble BC337 + 1k resistor + diode + motor.
Test with a simple [pulse sketch](/parts/motor-test).

### 5. Flash the firmware

Verify correct pulse detection and vibration.

### 6. Test multiple devices

Confirm they receive each other’s sequences.

### 7. Build the enclosure

Shape, embed, close, let dry or cure.

---

# **7. Intended Use**

This is not a utility tool or productivity device.
It is a small social instrument for:

* presence signaling
* rhythmic exchanges
* shared rituals
* silent group communication
* meditative practices
* collective awareness

Meaning comes from the group, not the device.

---

# **Licensing**

The Calm Handheld Computer is released under three strongly reciprocal open-source licenses:

**Hardware**
Licensed under **CERN-OHL-S v2**.
Any modified or redistributed hardware **must** be released under the same license.
See `LICENSE-HARDWARE`.

**Firmware**
Licensed under **GPLv3**.
Any modified firmware or forks **must** remain free and open-source.
See `LICENSE`.

**Documentation**
Licensed under **CC BY-SA 4.0**.
You may share, modify, and translate the documentation, but derivatives **must** use the same license.

---
