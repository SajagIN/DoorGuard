// DoorGuard controller, commit 1: LCD bring-up, I2C scan and ARM button check.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include "config.h"

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

static void scanI2C() {
  Serial.println("I2C scan:");
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("  found 0x%02X\n", a);
  }
}

void setup() {
  Serial.begin(LOG_BAUD);
  pinMode(PIN_ARM_BTN, INPUT_PULLUP);
  pinMode(PIN_ACT, OUTPUT);
  digitalWrite(PIN_ACT, LOW);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  scanI2C();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("DoorGuard boot");
}

void loop() {
  char b[17];
  snprintf(b, sizeof b, "up %6lu s", (unsigned long)(millis() / 1000));
  lcd.setCursor(0, 1);
  lcd.print(b);
  Serial.printf("ARM button raw=%d\n", digitalRead(PIN_ARM_BTN));
  delay(1000);
}
