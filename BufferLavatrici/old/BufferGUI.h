#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "BufferGUIElement.h"

namespace BufferGUI
{
  /////////////////////////////////////////////////////
  ///////////////////WIFI SPRITE///////////////////////
  /////////////////////////////////////////////////////
  enum class GuiSpriteId : uint8_t
  {
    WifiConnected = 0,
    WifiDisconnected = 1,

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

  void clearSprite(
    lgfx::LGFX_Device& lcd,
    GuiSpriteId spriteId,
    int16_t x,
    int16_t y,
    uint16_t backgroundColor = TFT_BLACK
  );

  void destroy();

  int16_t getSpriteWidth(GuiSpriteId spriteId);
  int16_t getSpriteHeight(GuiSpriteId spriteId);
  /////////////////////////////////////////////////////
  ///////////////////GESTURES//////////////////////////
  /////////////////////////////////////////////////////
  SwipeState swipe(int prevY, int y);

  /////////////////////////////////////////////////////
  ///////////////////TABLE/////////////////////////////
  /////////////////////////////////////////////////////

  void initTable(TableRow* rows);
  void drawTable(lgfx::LGFX_Device& lcd, TableRow* rows);
  void swipeTable(TableRow* rows, SwipeState swipeState);
  void clearTableArea(lgfx::LGFX_Device& lcd);
}