# DoorGuard

Door-state guard. An ESP32 controller (ESP32-WROOM DevKit) receives ESP-NOW packets from three ESP-12E sensor nodes, runs the state machine, drives the LCD and the ALARM output, and writes a CSV log on serial.

- `controller/`: ESP32 firmware (PlatformIO). Build: `pio run -d controller`
- `nodes/n1_door/`: ESP-12E, A3144 Hall door state and piezo knock. Build: `pio run -d nodes/n1_door`
- `nodes/n2_motion/`: ESP-12E, MPU6050 tilt. Build: `pio run -d nodes/n2_motion`
- `nodes/n3_presence/`: ESP-12E, APDS-9930 proximity and ambient light. Build: `pio run -d nodes/n3_presence`
- `common/packet.h`: ESP-NOW packet formats shared by all boards.
- `docs/wiring.md`: pin-to-pin wiring for every board.

Channel 1 is used by every board. The nodes send to the broadcast address; each packet carries a node ID.
