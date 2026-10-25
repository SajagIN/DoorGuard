// DoorGuard node N2: tilt from the MPU6050 accelerometer. ESP-12E, Arduino framework.
// Sends P3 MOTION every 10 ms with the tilt of X and Y from vertical, in centi-degrees.
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <user_interface.h>
#include <Wire.h>
#include <math.h>
#include "../../../common/packet.h"

#define WIFI_CHANNEL 1
#define PIN_SDA  D2      // GPIO4
#define PIN_SCL  D1      // GPIO5
#define MPU_ADDR 0x68    // AD0 tied to GND
#define MOTION_PERIOD_MS 10

static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static bool mpuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool readAccel(int16_t &ax, int16_t &ay, int16_t &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);                       // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6) != 6) return false;
  ax = (int16_t)((Wire.read() << 8) | Wire.read());
  ay = (int16_t)((Wire.read() << 8) | Wire.read());
  az = (int16_t)((Wire.read() << 8) | Wire.read());
  return true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  mpuWrite(0x6B, 0x00);                   // PWR_MGMT_1: wake the MPU6050

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_channel(WIFI_CHANNEL);
  if (esp_now_init() != 0) {
    Serial.println("esp-now init failed");
    while (true) delay(1000);
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_add_peer((uint8_t *)BCAST, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);
}

void loop() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < MOTION_PERIOD_MS) return;
  last = now;

  int16_t ax, ay, az;
  if (!readAccel(ax, ay, az)) return;     // skip the sample; the controller sees the gap

  Motion m;
  m.h.type = PKT_MOTION;
  m.h.node = NODE_MOTION;
  m.tilt_x_c = (int16_t)lround(atan2((double)ax, (double)az) * 180.0 / M_PI * 100.0);
  m.tilt_y_c = (int16_t)lround(atan2((double)ay, (double)az) * 180.0 / M_PI * 100.0);
  m.flags = 0x01;                         // bit 0: sample valid
  esp_now_send((uint8_t *)BCAST, (uint8_t *)&m, sizeof m);
}
