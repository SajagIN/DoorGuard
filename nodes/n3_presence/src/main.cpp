// DoorGuard node N3: proximity and ambient light from the APDS-9930. ESP-12E, Arduino framework.
// Sends P4 PROX every 10 ms and P5 ALS every 1 s.
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <user_interface.h>
#include <Wire.h>
#include "../../../common/packet.h"

#define WIFI_CHANNEL 1
#define PIN_SDA  D2      // GPIO4
#define PIN_SCL  D1      // GPIO5
#define APDS_ADDR 0x39
#define PROX_PERIOD_MS 10
#define ALS_PERIOD_MS 1000

static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static bool apdsWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(APDS_ADDR);
  Wire.write(0x80 | reg);                 // command bit
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static int apdsRead8(uint8_t reg) {
  Wire.beginTransmission(APDS_ADDR);
  Wire.write(0x80 | reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom((uint8_t)APDS_ADDR, (uint8_t)1) != 1) return -1;
  return Wire.read();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  int id = apdsRead8(0x12);               // ID register, APDS-9930 = 0x39
  if (id != 0x39) Serial.printf("APDS-9930 id=0x%02X (expected 0x39)\n", id);
  apdsWrite(0x01, 0xDB);                  // ATIME: about 103 ms ALS integration
  apdsWrite(0x0E, 0x87);                  // PPULSE: 8 pulses
  apdsWrite(0x0F, 0x00);                  // CONTROL: 100 mA LED drive, 1x gain
  apdsWrite(0x00, 0x01);                  // ENABLE: power on
  delay(3);
  apdsWrite(0x00, 0x07);                  // ENABLE: power on, ALS on, proximity on

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
  static uint32_t lastProx = 0, lastAls = 0;
  uint32_t now = millis();

  if (now - lastProx >= PROX_PERIOD_MS) {
    lastProx = now;
    int p = apdsRead8(0x1C);              // PDATA
    if (p >= 0) {
      Prox pr;
      pr.h.type = PKT_PROX;
      pr.h.node = NODE_PRESENCE;
      pr.prox = (uint8_t)p;
      esp_now_send((uint8_t *)BCAST, (uint8_t *)&pr, sizeof pr);
    }
  }

  if (now - lastAls >= ALS_PERIOD_MS) {
    lastAls = now;
    int lo = apdsRead8(0x14);             // CDATAL
    int hi = apdsRead8(0x15);             // CDATAH
    if (lo >= 0 && hi >= 0) {
      Als a;
      a.h.type = PKT_ALS;
      a.h.node = NODE_PRESENCE;
      a.als = (uint16_t)((hi << 8) | lo);
      a.flags = 0x01;                     // bit 0: sample valid
      esp_now_send((uint8_t *)BCAST, (uint8_t *)&a, sizeof a);
    }
  }
}
