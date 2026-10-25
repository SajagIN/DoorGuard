// DoorGuard controller configuration: ESP32-WROOM DevKit.
#pragma once

// ---- Pins (ESP32 GPIO numbers) ----
#define PIN_I2C_SDA   21    // LCD PCF8574 backpack SDA. LCD runs from 3.3 V, so no level shifter.
#define PIN_I2C_SCL   22    // LCD PCF8574 backpack SCL
#define PIN_ARM_BTN   33    // ARM button: tactile switch or jumper wire to GND. INPUT_PULLUP, LOW = pressed.
#define PIN_ACT       26    // ALARM output: active buzzer module (+) or LED anode via 220 ohm

// ---- LCD ----
#define LCD_ADDR      0x27  // PCF8574 backpack address. Check with the I2C scan; 0x3F on some backpacks.
#define LCD_COLS      16
#define LCD_ROWS      2

// ---- Serial log ----
#define LOG_BAUD      921600

// ---- ESP-NOW ----
#define WIFI_CHANNEL  1     // controller and nodes must use the same channel

// ---- Timing and FSM (Final Report sections 4.2 and 4.3) ----
#define TICK_MS              10     // FSM tick period (task T1)
#define HOLD_MS              2000   // ARM and DISARM hold time
#define KNOCK_WINDOW_MS      3000   // two knocks inside this window = knock_2
#define TILT_LIMIT_CDEG      500    // 5 degrees, in centi-degrees, from the armed baseline
#define HEARTBEAT_TIMEOUT_MS 300    // a node silent for longer is a fault
#define INIT_TIMEOUT_MS      1000   //@C4 self-test timeout before FAULT
