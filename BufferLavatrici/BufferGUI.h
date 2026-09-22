#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "BufferGUIElement.h"

namespace BufferGUI
{
  /////////////////////////////////////////////////////
  ///////////////////WIFI/MQTT ICONS///////////////////
  /////////////////////////////////////////////////////
  enum class GuiSpriteId : uint8_t
  {
    WifiConnected = 0,
    WifiDisconnected = 1,
    MqttConnected = 2,
    MqttDisconnected = 3,
    Count
  };

  void begin(lgfx::LGFX_Device& lcd);

  void drawSprite(
    lgfx::LGFX_Device& lcd,
    GuiSpriteId spriteId,
    int16_t x,
    int16_t y
  );

  void drawSpriteTopRight(
    lgfx::LGFX_Device& lcd,
    GuiSpriteId spriteId,
    int16_t margin = 1
  );

  void drawWifiTopRight(
    lgfx::LGFX_Device& lcd,
    bool connected,
    int16_t margin = 1
  );

  void drawMqttTopRight(
    lgfx::LGFX_Device& lcd,
    bool connected,
    int16_t margin = 22
  );

  void destroy();

  /////////////////////////////////////////////////////
  ///////////////////HEADER////////////////////////////
  /////////////////////////////////////////////////////
  void drawHeader(
    lgfx::LGFX_Device& lcd,
    bool wifiConnected,
    bool mqttConnected,
    const String& macAddr,
    int cobotMission
  );

  /////////////////////////////////////////////////////
  ///////////////////TABLE/////////////////////////////
  /////////////////////////////////////////////////////
  void initTable(TableRow* rows);

  void drawTable(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount,
    int selectedCard = -1,
    bool longEnough = false
  );

  void redrawTableRow(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount,
    int rowIndex,
    bool selected,
    bool longEnough
  );

  // Sposta la tabella di deltaY pixel seguendo il dito.
  // Ritorna il delta realmente applicato dopo il clamp ai limiti.
  int scrollTableByPixels(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount,
    int deltaY
  );

  int tableRowHitbox(
    TableRow* rows,
    int x,
    int y,
    int rowsCount
  );

  void clearTableArea(lgfx::LGFX_Device& lcd);
  void clearTableRowsData(TableRow* rows);

  /////////////////////////////////////////////////////
  ///////////////////WAITING PANEL/////////////////////
  /////////////////////////////////////////////////////
  void drawWaitingPanel(
    lgfx::LGFX_Device& lcd,
    const String& waitingMessage
  );

  void clearWaitingPanel(lgfx::LGFX_Device& lcd);
}
