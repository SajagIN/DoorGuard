// DoorGuard node N1: door state (A3144 Hall) and knock (piezo). ESP-12E, Arduino framework.
// Sends P1 DOOR_STATUS every 100 ms and P2 DOOR_EVENT on change or knock.
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <user_interface.h>
#include "../../../common/packet.h"

#define WIFI_CHANNEL 1
#define PIN_HALL   D5   // GPIO14: A3144 DO. Open-drain, internal pull-up, LOW = magnet present.
#define PIN_PIEZO  D7   // GPIO13: piezo through 10 kohm; 1 Mohm from this pin to GND.
#define KNOCK_MIN_GAP_MS 20
#define STATUS_PERIOD_MS 100

static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static volatile bool hallChanged = false;
static volatile bool knockPending = false;
static volatile uint32_t lastKnockMs = 0;

ICACHE_RAM_ATTR void hallIsr() { hallChanged = true; }

ICACHE_RAM_ATTR void piezoIsr() {
  uint32_t now = millis();
  if (now - lastKnockMs >= KNOCK_MIN_GAP_MS) {
    lastKnockMs = now;
    knockPending = true;
  }
}

static void sendEvent(uint8_t code) {
  DoorEvent e;
  e.h.type = PKT_DOOR_EVENT;
  e.h.node = NODE_DOOR;
  e.code = code;
  esp_now_send((uint8_t *)BCAST, (uint8_t *)&e, sizeof e);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_HALL, INPUT_PULLUP);
  pinMode(PIN_PIEZO, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), hallIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_PIEZO), piezoIsr, RISING);

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
  static bool lastOpen = (digitalRead(PIN_HALL) == HIGH);
  static uint8_t seq = 0;
  static uint32_t lastStatus = 0;

  bool open = digitalRead(PIN_HALL) == HIGH;   // magnet away = door open
  if (hallChanged) {
    hallChanged = false;
    if (open != lastOpen) {
      lastOpen = open;
      sendEvent(open ? EV_DOOR_OPEN : EV_DOOR_CLOSED);
    }
  }
  if (knockPending) {
    knockPending = false;
    sendEvent(EV_KNOCK);
  }
  uint32_t now = millis();
  if (now - lastStatus >= STATUS_PERIOD_MS) {
    lastStatus = now;
    DoorStatus s;
    s.h.type = PKT_DOOR_STATUS;
    s.h.node = NODE_DOOR;
    s.open = open ? 1 : 0;
    s.seq = seq++;
    s.flags = 0;
    esp_now_send((uint8_t *)BCAST, (uint8_t *)&s, sizeof s);
  }
}
