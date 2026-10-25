// DoorGuard ESP-NOW packet formats. Shared by the controller and all sensor nodes.
// Every packet starts with a 2-byte header: type, node. Sizes match the Final Report table 4.3a.
#pragma once
#include <stdint.h>

enum PktType : uint8_t {
  PKT_DOOR_STATUS = 1,  // P1, node N1, 5 B, every 100 ms
  PKT_DOOR_EVENT  = 2,  // P2, node N1, 3 B, on change or knock (min gap 20 ms)
  PKT_MOTION      = 3,  // P3, node N2, 7 B, every 10 ms
  PKT_PROX        = 4,  // P4, node N3, 3 B, every 10 ms
  PKT_ALS         = 5,  // P5, node N3, 5 B, every 1 s
};

enum NodeId : uint8_t {
  NODE_DOOR     = 1,  // ESP-12E: A3144 Hall + piezo
  NODE_MOTION   = 2,  // ESP-12E: MPU6050
  NODE_PRESENCE = 3,  // ESP-12E: APDS-9930
};

enum EventCode : uint8_t {
  EV_DOOR_OPEN   = 1,
  EV_DOOR_CLOSED = 2,
  EV_KNOCK       = 3,
};

#pragma pack(push, 1)
struct PktHdr    { uint8_t type; uint8_t node; };
struct DoorStatus{ PktHdr h; uint8_t open; uint8_t seq; uint8_t flags; };           // 5 B
struct DoorEvent { PktHdr h; uint8_t code; };                                       // 3 B
struct Motion    { PktHdr h; int16_t tilt_x_c; int16_t tilt_y_c; uint8_t flags; };  // 7 B, centi-degrees
struct Prox      { PktHdr h; uint8_t prox; };                                       // 3 B
struct Als       { PktHdr h; uint16_t als; uint8_t flags; };                        // 5 B
#pragma pack(pop)

static_assert(sizeof(DoorStatus) == 5, "P1 must be 5 bytes");
static_assert(sizeof(DoorEvent) == 3, "P2 must be 3 bytes");
static_assert(sizeof(Motion) == 7, "P3 must be 7 bytes");
static_assert(sizeof(Prox) == 3, "P4 must be 3 bytes");
static_assert(sizeof(Als) == 5, "P5 must be 5 bytes");
