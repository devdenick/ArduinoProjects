#include "USBConfigLoader.h"

USBConfigLoader::USBConfigLoader()
  : _usb("usb"), _otg(PB_14, 0), _mapCount(0), _mounted(false) {
}

bool USBConfigLoader::begin(uint32_t timeoutMs) {
  return mountUsb(timeoutMs);
}

bool USBConfigLoader::mountUsb(uint32_t timeoutMs) {
  if (_mounted) {
    return true;
  }

  uint32_t start = millis();
  while (!_msd.connect()) {
    delay(250);

    if (millis() - start > timeoutMs) {
      return false;
    }
  }

  int err = _usb.mount(&_msd);
  if (err) {
    return false;
  }

  _mounted = true;
  return true;
}

bool USBConfigLoader::isMounted() const {
  return _mounted;
}

bool USBConfigLoader::fileExists(const char* path) const {
  struct stat st;
  return stat(path, &st) == 0;
}

bool USBConfigLoader::buildUsbPath(const char* fileName, char* outPath, size_t outSize) const {
  if (!fileName || !outPath || outSize == 0) {
    return false;
  }

  int written = snprintf(outPath, outSize, "/usb/%s", fileName);
  return written > 0 && (size_t)written < outSize;
}

uint32_t USBConfigLoader::parseCanId(JsonVariantConst v) const {
  if (v.is<const char*>()) {
    return strtoul(v.as<const char*>(), nullptr, 0);
  }
  return v.as<uint32_t>();
}

bool USBConfigLoader::loadJsonFile(const char* path, DynamicJsonDocument& doc) {
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    return false;
  }

  fseek(fp, 0, SEEK_END);
  long fileSize = ftell(fp);
  rewind(fp);

  if (fileSize <= 0) {
    fclose(fp);
    return false;
  }

  char* buffer = (char*)malloc(fileSize + 1);
  if (!buffer) {
    fclose(fp);
    return false;
  }

  size_t bytesRead = fread(buffer, 1, fileSize, fp);
  fclose(fp);

  buffer[bytesRead] = '\0';

  DeserializationError err = deserializeJson(doc, buffer);
  free(buffer);

  return !err;
}

bool USBConfigLoader::buildMappingTableFromJson(DynamicJsonDocument& doc) {
  _mapCount = 0;

  JsonVariant messagesVar = doc["messages"];
  if (messagesVar.isNull()) {
    messagesVar = doc["Messages"];
  }

  if (messagesVar.isNull() || !messagesVar.is<JsonArray>()) {
    return false;
  }

  JsonArray messages = messagesVar.as<JsonArray>();

  for (JsonObject msg : messages) {
    uint32_t canId = 0;

    if (!msg["canId"].isNull()) {
      canId = parseCanId(msg["canId"]);
    } else if (!msg["CanId"].isNull()) {
      canId = parseCanId(msg["CanId"]);
    } else if (!msg["CanIdHex"].isNull()) {
      canId = parseCanId(msg["CanIdHex"]);
    }

    JsonVariant fieldsVar = msg["fields"];
    if (fieldsVar.isNull()) {
      fieldsVar = msg["Fields"];
    }

    if (fieldsVar.isNull() || !fieldsVar.is<JsonArray>()) {
      continue;
    }

    JsonArray fields = fieldsVar.as<JsonArray>();

    for (JsonObject field : fields) {
      if (_mapCount >= MAX_MAPS) {
        return false;
      }

      _mappingTable[_mapCount].canID = canId;

      _mappingTable[_mapCount].startBit =
          field["StartBit"] | field["startBit"] | 0;

      _mappingTable[_mapCount].length =
          field["Length"] | field["length"] | 0;

      if (!field["ModbusRegister"].isNull()) {
        if (field["ModbusRegister"].is<const char*>()) {
          _mappingTable[_mapCount].regIndex =
              (uint16_t)strtoul(field["ModbusRegister"], nullptr, 10);
        } else {
          _mappingTable[_mapCount].regIndex =
              field["ModbusRegister"].as<uint16_t>();
        }
      } else if (!field["modbusRegister"].isNull()) {
        _mappingTable[_mapCount].regIndex =
            field["modbusRegister"].as<uint16_t>();
      } else {
        _mappingTable[_mapCount].regIndex = 0;
      }

      if (!field["Source"].isNull()) {
        _mappingTable[_mapCount].source = field["Source"].as<uint8_t>();
      } else if (!field["source"].isNull()) {
        _mappingTable[_mapCount].source = field["source"].as<uint8_t>();
      } else {
        const char* io = field["IO"] | field["io"] | "CAN";
        _mappingTable[_mapCount].source = (strcmp(io, "PLC") == 0) ? 1 : 0;
      }

      _mapCount++;
    }
  }

  return true;
}

bool USBConfigLoader::loadConfig(const char* fileName, size_t jsonCapacity) {
  if (!_mounted) {
    return false;
  }

  char path[128];
  if (!buildUsbPath(fileName, path, sizeof(path))) {
    return false;
  }

  if (!fileExists(path)) {
    return false;
  }

  DynamicJsonDocument doc(jsonCapacity);

  if (!loadJsonFile(path, doc)) {
    return false;
  }

  return buildMappingTableFromJson(doc);
}

const JSONSignal* USBConfigLoader::getMappings() const {
  return _mappingTable;
}

JSONSignal* USBConfigLoader::getMappings() {
  return _mappingTable;
}

uint16_t USBConfigLoader::getMapCount() const {
  return _mapCount;
}

void USBConfigLoader::printMappings(Stream& out) const {
  out.println("=== MAPPING TABLE ===");

  for (uint16_t i = 0; i < _mapCount; i++) {
    out.print(i);
    out.print(" | canID: 0x");
    out.print(_mappingTable[i].canID, HEX);
    out.print(" | startBit: ");
    out.print(_mappingTable[i].startBit);
    out.print(" | length: ");
    out.print(_mappingTable[i].length);
    out.print(" | regIndex: ");
    out.print(_mappingTable[i].regIndex);
    out.print(" | source: ");
    out.println(_mappingTable[i].source == 1 ? "PLC" : "CAN");
  }
}