#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

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
    int16_t margin = 6
  );

  void drawWifiTopRight(
    lgfx::LGFX_Device& lcd,
    bool connected,
    int16_t margin = 6
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
  bool upSwipe(lgfx::LGFX_Device& lcd);
}