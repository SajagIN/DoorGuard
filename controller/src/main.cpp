// DoorGuard controller (ESP32-WROOM DevKit, Arduino framework).
// Receives ESP-NOW packets from three ESP-12E nodes, runs the system FSM, drives the LCD,
// the ALARM output and the CSV log on serial.
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "../../common/packet.h"

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// ---------- ESP-NOW receive queue (filled in the Wi-Fi task, drained in loop) ----------
#define RXQ 16
struct RxItem { uint8_t len; uint8_t data[16]; };
static RxItem rxq[RXQ];
static uint8_t rxHead = 0, rxTail = 0;
static portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
  if (len <= 0 || len > 16) return;
  portENTER_CRITICAL_ISR(&rxMux);
  uint8_t next = (rxHead + 1) % RXQ;
  if (next != rxTail) {            // queue full: drop the packet
    rxq[rxHead].len = (uint8_t)len;
    memcpy(rxq[rxHead].data, data, len);
    rxHead = next;
  }
  portEXIT_CRITICAL_ISR(&rxMux);
}

bool popRx(RxItem &out) {
  bool ok = false;
  portENTER_CRITICAL(&rxMux);
  if (rxTail != rxHead) {
    out = rxq[rxTail];
    rxTail = (rxTail + 1) % RXQ;
    ok = true;
  }
  portEXIT_CRITICAL(&rxMux);
  return ok;
}

// ---------- State ----------
enum State { S_INIT, S_DISARMED, S_ARMED, S_ALARM, S_FAULT, S_SAFE };
static const char *STATE_NAME[] = {"INIT", "DISARMED", "ARMED", "ALARM", "FAULT", "SAFE"};
static State st = S_INIT;
static uint32_t stateSince = 0;

// Evidence and latest node data
static bool doorOpen = false;
static int16_t tiltX = 0, tiltY = 0, baseX = 0, baseY = 0;
static bool tiltSeen = false;
static uint8_t prox = 0;
static uint16_t als = 0;
static uint8_t knockCount = 0;
static uint32_t firstKnockMs = 0;
static uint32_t lastSeen[4] = {0, 0, 0, 0};   // index = NodeId (1..3), 0 = never seen

static bool fresh(uint8_t node, uint32_t now) {
  return lastSeen[node] != 0 && (now - lastSeen[node]) <= HEARTBEAT_TIMEOUT_MS;
}


static void setState(State s, uint32_t now) {
  st = s;
  stateSince = now;
}

// ---------- Packet handling ----------
void handlePacket(const uint8_t *d, uint8_t len) {
  if (len < sizeof(PktHdr)) return;
  PktHdr h;
  memcpy(&h, d, sizeof h);
  uint32_t now = millis();
  if (h.node >= NODE_DOOR && h.node <= NODE_PRESENCE) lastSeen[h.node] = now;

  switch (h.type) {
    case PKT_DOOR_STATUS: {
      DoorStatus p;
      if (len != sizeof p) break;
      memcpy(&p, d, sizeof p);
      doorOpen = p.open != 0;
      break;
    }
    case PKT_DOOR_EVENT: {
      DoorEvent p;
      if (len != sizeof p) break;
      memcpy(&p, d, sizeof p);
      if (p.code == EV_KNOCK) {
        if (knockCount == 0 || now - firstKnockMs > KNOCK_WINDOW_MS) {
          firstKnockMs = now;
          knockCount = 0;
        }
        knockCount++;
      }
      break;
    }
    case PKT_MOTION: {
      Motion p;
      if (len != sizeof p) break;
      memcpy(&p, d, sizeof p);
      tiltX = p.tilt_x_c;
      tiltY = p.tilt_y_c;
      tiltSeen = true;
      break;
    }
    case PKT_PROX: {
      Prox p;
      if (len != sizeof p) break;
      memcpy(&p, d, sizeof p);
      prox = p.prox;
      break;
    }
    case PKT_ALS: {
      Als p;
      if (len != sizeof p) break;
      memcpy(&p, d, sizeof p);
      als = p.als;
      break;
    }
    default:
      break;
  }
}

// ---------- Arm / disarm button (hold HOLD_MS) ----------
void checkButton(uint32_t now) {
  static bool wasDown = false, fired = false;
  static uint32_t downSince = 0;
  bool pressed = digitalRead(PIN_ARM_BTN) == LOW;
  if (pressed && !wasDown) {
    downSince = now;
    fired = false;
  } else if (pressed && !fired && (now - downSince) >= HOLD_MS) {
    fired = true;
    if (st == S_DISARMED) {
      baseX = tiltX;                    // tilt baseline taken at arming
      baseY = tiltY;
      setState(S_ARMED, now);
    } else if (st == S_ARMED || st == S_ALARM) {
      setState(S_DISARMED, now);
    }
  }
  wasDown = pressed;
}

// ---------- FSM tick (task T1, every TICK_MS) ----------
void fsmTick(uint32_t now) {

  bool hallEv = doorOpen;
  bool knockEv = knockCount >= 2 && (now - firstKnockMs) <= KNOCK_WINDOW_MS;
  bool tiltEv = tiltSeen && (abs(tiltX - baseX) > TILT_LIMIT_CDEG || abs(tiltY - baseY) > TILT_LIMIT_CDEG);

  switch (st) {
    case S_INIT:
      if (fresh(NODE_DOOR, now) && fresh(NODE_MOTION, now) && fresh(NODE_PRESENCE, now)) {
        setState(S_DISARMED, now);
      }
      break;
    case S_DISARMED:
      break;                            // waits for the ARM hold
    case S_ARMED:
      if (hallEv || tiltEv || knockEv) {
        setState(S_ALARM, now);
      }
      break;
    case S_ALARM:
      break;                            // waits for the DISARM hold
  }
}

void applyOutputs() {
  bool alarm = (st == S_ALARM) || (st == S_SAFE);
  digitalWrite(PIN_ACT, alarm ? HIGH : LOW);
}

// ---------- LCD (every 200 ms) ----------
void lcdUpdate(uint32_t now) {
  static uint32_t last = 0;
  if (now - last < 200) return;
  last = now;
  char l0[17], l1[17];
  snprintf(l0, sizeof l0, "%-8s %s", STATE_NAME[st], doorOpen ? "OPEN" : "CLOSED");
  snprintf(l1, sizeof l1, "tilt%+4d K%u", (tiltX - baseX) / 100, (unsigned)knockCount);
  lcd.setCursor(0, 0);
  lcd.print(l0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(l0);
  lcd.setCursor(0, 1);
  lcd.print(l1);
}

void setup() {
  Serial.begin(LOG_BAUD);
  pinMode(PIN_ARM_BTN, INPUT_PULLUP);
  pinMode(PIN_ACT, OUTPUT);
  digitalWrite(PIN_ACT, LOW);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    lcd.setCursor(0, 0);
    lcd.print("ESP-NOW FAIL");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onRecv);

  Serial.println("ms,kind,state,detail");
  Serial.printf("controller MAC %s\n", WiFi.macAddress().c_str());
  setState(S_INIT, millis());
}

void loop() {
  RxItem it;
  while (popRx(it)) handlePacket(it.data, it.len);

  static uint32_t nextTick = 0;
  uint32_t now = millis();
  if (now >= nextTick) {
    nextTick = now + TICK_MS;
    checkButton(now);
    fsmTick(now);
    applyOutputs();
  }
  lcdUpdate(now);
}
