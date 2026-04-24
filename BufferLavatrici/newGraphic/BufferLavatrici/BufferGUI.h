#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "BufferGUIElement.h"

namespace BufferGUI
{
  /////////////////////////////////////////////////////
  ///////////////////WIFI SPRITE///////////////////////
  /////////////////////////////////////////////////////

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
    int16_t margin = 8
  );

  void drawWifiTopRight(
    lgfx::LGFX_Device& lcd,
    bool connected,
    int16_t margin = 8
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
  ///////////////////APP CHROME////////////////////////
  /////////////////////////////////////////////////////

  void drawAppChrome(lgfx::LGFX_Device& lcd, bool wifiConnected);
  void drawTableFrame(lgfx::LGFX_Device& lcd);
  void drawSpriteError(lgfx::LGFX_Device& lcd);

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
  void drawTableSprite(TableRow* rows, lgfx::LGFX_Sprite& tableSprite);
}
