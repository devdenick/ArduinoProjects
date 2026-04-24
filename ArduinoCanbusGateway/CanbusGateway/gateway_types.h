#pragma once
#include <Arduino.h>

#define MAX_MAPS 100

struct CanDataRaw {
  uint32_t canId;
  uint8_t dlc;
  uint8_t data[8];
  uint64_t raw;
};

typedef struct {
  uint32_t canID;
  uint8_t startBit;
  uint8_t length;
  uint16_t regIndex;
  uint8_t source; // 0 CAN, 1 PLC
} JSONSignal;

union FloatBytes {
  float value;
  uint8_t bytes[4];
};