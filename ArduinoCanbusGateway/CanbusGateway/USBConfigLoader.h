#ifndef USB_CONFIG_LOADER_H
#define USB_CONFIG_LOADER_H

#include <Arduino.h>
#include <Arduino_PortentaMachineControl.h>
#include <Arduino_USBHostMbed5.h>
#include <ArduinoJson.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>
#include "gateway_types.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class USBConfigLoader {
public:
  USBConfigLoader();

  bool begin(uint32_t timeoutMs = 10000);
  bool loadConfig(const char* fileName = "config.json", size_t jsonCapacity = 32768);

  const JSONSignal* getMappings() const;
  JSONSignal* getMappings();
  uint16_t getMapCount() const;

  void printMappings(Stream& out) const;
  bool isMounted() const;

private:
  USBHostMSD _msd;
  mbed::FATFileSystem _usb;
  mbed::DigitalOut _otg;

  JSONSignal _mappingTable[MAX_MAPS];
  uint16_t _mapCount;
  bool _mounted;

  bool mountUsb(uint32_t timeoutMs);
  bool fileExists(const char* path) const;
  bool buildUsbPath(const char* fileName, char* outPath, size_t outSize) const;
  bool loadJsonFile(const char* path, DynamicJsonDocument& doc);
  bool buildMappingTableFromJson(DynamicJsonDocument& doc);

  uint32_t parseCanId(JsonVariantConst v) const;
};

#endif