# DoorGuard pin-to-pin wiring

Parts in the kit: ESP32-WROOM DevKit (controller), three ESP-12E boards (N1, N2, N3), A3144 Hall module with magnet, piezo element, MPU6050 breakout, APDS-9930 breakout, 16x2 LCD with PCF8574 I2C backpack, 30x10 breadboard, female-to-female and female-to-male jumper wires.

**Resistors:** none in the kit. Only the parts in the "Required additions" table below need resistors.

**Common ground:** every GND pin (controller, all three nodes, LCD, sensors, buzzer, piezo) goes to one ground rail. Without it the I2C and sensor readings are unreliable.

## Controller: ESP32-WROOM DevKit

| ESP32 pin | Connected to | Notes |
|---|---|---|
| 3V3 | LCD backpack VCC | LCD runs at 3.3 V, so its SDA and SCL stay at 3.3 V (no level shifter) |
| GPIO21 (SDA) | LCD backpack SDA | I2C, default ESP32 pin |
| GPIO22 (SCL) | LCD backpack SCL | I2C, default ESP32 pin |
| GPIO33 | ARM button, one leg | Other leg to GND. Internal pull-up (INPUT_PULLUP). Pressed = LOW. |
| GPIO26 | Active buzzer module + (ALARM output) | Or LED anode through 220 ohm, LED cathode to GND |
| GND | LCD GND, ARM button, buzzer -, ground rail | |
| USB | 5 V supply for the controller | |

## Node N1 (ESP-12E): door

| ESP-12E pin (GPIO) | Connected to | Notes |
|---|---|---|
| D5 (GPIO14) | A3144 DO (output) | Internal pull-up. Magnet near = LOW = closed. Magnet away = HIGH = open. |
| D7 (GPIO13) | Piezo + through 10 kohm (series) | Also 1 Mohm from D7 to GND. Interrupt on rising edge. |
| VIN (5 V) | A3144 VCC | Check the A3144 datasheet supply range before power-up |
| GND | A3144 GND, piezo -, ground rail | |

The magnet glues to the door leaf. The A3144 mounts on the frame, and the magnet must line up with it.

## Node N2 (ESP-12E): motion

| ESP-12E pin (GPIO) | Connected to | Notes |
|---|---|---|
| D2 (GPIO4, SDA) | MPU6050 SDA | I2C at 400 kHz |
| D1 (GPIO5, SCL) | MPU6050 SCL | |
| 3V3 | MPU6050 VCC | 3.3 V supply |
| GND | MPU6050 GND, ground rail | AD0 to GND (address 0x68). INT not connected. |

## Node N3 (ESP-12E): presence

| ESP-12E pin (GPIO) | Connected to | Notes |
|---|---|---|
| D2 (GPIO4, SDA) | APDS-9930 SDA | I2C at 400 kHz |
| D1 (GPIO5, SCL) | APDS-9930 SCL | |
| 3V3 | APDS-9930 VCC | 3.3 V supply |
| GND | APDS-9930 GND, ground rail | INT not connected. |

## Required additions (not in the kit)

| Part | Value | Where | Why |
|---|---|---|---|
| Resistor | 10 kohm | N1, piezo + to D7 | Limits current into the pin when the piezo spikes |
| Resistor | 1 Mohm | N1, D7 to GND | Bleeds the piezo charge so the input does not float |
| Resistor | 220 ohm | Only if an LED is used on GPIO26 | Limits LED current |
| Tactile switch | - | Controller, GPIO33 to GND | ARM button. A jumper wire to GND also works. |
| Active buzzer module | - | Controller, GPIO26 | ALARM output (self-driven, no resistor) |
| Resistor pack | 4.7 kohm x 2 | Only if the MPU6050, APDS-9930 or LCD module lacks I2C pull-ups | Pull SDA and SCL to 3.3 V |

Check the MPU6050, APDS-9930 and LCD breakouts for onboard pull-ups before adding any. Most breakouts have them.

## Pins avoided

ESP-12E boot-strap pins GPIO0, GPIO2, GPIO12 and GPIO15 are not used. Those pins affect boot mode and flash voltage.
