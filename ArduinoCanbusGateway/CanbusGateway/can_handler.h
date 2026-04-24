#pragma once
#include <Arduino_CAN.h>
#include "gateway_types.h"

bool readCanFrame(CanDataRaw& data);
uint16_t extractToUint16(uint64_t raw, uint8_t startBit, uint8_t length);
void insertBits(uint8_t* data, uint64_t value, uint8_t startBit, uint8_t length);