#include "can_handler.h"

bool readCanFrame(CanDataRaw& data) {
  if (!CAN.available()) {
    return false;
  }

  CanMsg const msg = CAN.read();

  data.canId = msg.isStandardId() ? msg.getStandardId() : msg.getExtendedId();
  data.dlc = msg.data_length;
  data.raw = 0;

  for (uint8_t i = 0; i < msg.data_length; i++) {
    data.data[i] = msg.data[i];
    data.raw |= ((uint64_t)msg.data[i] << (8 * i));
  }

  return true;
}

uint16_t extractToUint16(uint64_t raw, uint8_t startBit, uint8_t length) {
  if (length == 0 || length > 16) {
    return 0;
  }

  uint64_t mask;
  if (length == 16) {
    mask = 0xFFFF;
  } else {
    mask = (1ULL << length) - 1;
  }

  return (uint16_t)((raw >> startBit) & mask);
}

void insertBits(uint8_t* data, uint64_t value, uint8_t startBit, uint8_t length) {
  if (length == 0) return;
  if ((uint16_t)startBit + length > 64) return;

  for (uint8_t i = 0; i < length; i++) {
    uint8_t bitValue = (value >> i) & 0x01;

    uint8_t absoluteBit = startBit + i;
    uint8_t byteIndex = absoluteBit / 8;
    uint8_t bitIndex = absoluteBit % 8;

    if (bitValue)
      data[byteIndex] |= (1 << bitIndex);
    else
      data[byteIndex] &= ~(1 << bitIndex);
  }
}