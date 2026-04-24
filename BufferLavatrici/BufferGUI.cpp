#include "BufferGUI.h"
#include "BufferGuiElement.h"

namespace BufferGUI
{
  using namespace BufferGUIElements;

  /////////////////////////////////////////////////////
  ///////////////////WIFI SPRITE///////////////////////
  /////////////////////////////////////////////////////
  static LGFX_Sprite* spriteCache[(uint8_t)GuiSpriteId::Count];
  static bool initialized = false;

  static uint8_t toIndex(GuiSpriteId spriteId)
  {
    return static_cast<uint8_t>(spriteId);
  }

  static const GuiBitmap16& getBitmap(GuiSpriteId spriteId)
  {
    return BITMAPS[toIndex(spriteId)];
  }

  static void createSpriteIfNeeded(lgfx::LGFX_Device& lcd, GuiSpriteId spriteId)
  {
    uint8_t index = toIndex(spriteId);

    if (spriteCache[index] != nullptr)
    {
      return;
    }

    const GuiBitmap16& bitmap = getBitmap(spriteId);

    spriteCache[index] = new LGFX_Sprite(&lcd);
    spriteCache[index]->setColorDepth(16);

    spriteCache[index]->createSprite(
      bitmap.width,
      bitmap.height
    );

    spriteCache[index]->pushImage(
      0,
      0,
      bitmap.width,
      bitmap.height,
      bitmap.data
    );
  }

  void begin(lgfx::LGFX_Device& lcd)
  {
    if (!initialized)
    {
      for (uint8_t i = 0; i < (uint8_t)GuiSpriteId::Count; i++)
      {
        spriteCache[i] = nullptr;
      }

      initialized = true;
    }

    createSpriteIfNeeded(lcd, GuiSpriteId::WifiConnected);
    createSpriteIfNeeded(lcd, GuiSpriteId::WifiDisconnected);
  }

  void drawSprite(
    lgfx::LGFX_Device& lcd,
    GuiSpriteId spriteId,
    int16_t x,
    int16_t y
  )
  {
    if (!initialized)
    {
      begin(lcd);
    }

    createSpriteIfNeeded(lcd, spriteId);

    uint8_t index = toIndex(spriteId);

    if (spriteCache[index] == nullptr)
    {
      return;
    }

    spriteCache[index]->pushSprite(x, y);
  }

  void drawSpriteTopRight(
    lgfx::LGFX_Device& lcd,
    GuiSpriteId spriteId,
    int16_t margin
  )
  {
    const GuiBitmap16& bitmap = getBitmap(spriteId);

    int16_t x = lcd.width() - bitmap.width - margin;
    int16_t y = margin;

    drawSprite(lcd, spriteId, x, y);
  }

  void drawWifiTopRight(
    lgfx::LGFX_Device& lcd,
    bool connected,
    int16_t margin
  )
  {
    GuiSpriteId spriteId = connected
      ? GuiSpriteId::WifiConnected
      : GuiSpriteId::WifiDisconnected;

    drawSpriteTopRight(lcd, spriteId, margin);
  }

  void clearSprite(
    lgfx::LGFX_Device& lcd,
    GuiSpriteId spriteId,
    int16_t x,
    int16_t y,
    uint16_t backgroundColor
  )
  {
    const GuiBitmap16& bitmap = getBitmap(spriteId);

    lcd.fillRect(
      x,
      y,
      bitmap.width,
      bitmap.height,
      backgroundColor
    );
  }

  int16_t getSpriteWidth(GuiSpriteId spriteId)
  {
    return getBitmap(spriteId).width;
  }

  int16_t getSpriteHeight(GuiSpriteId spriteId)
  {
    return getBitmap(spriteId).height;
  }

  void destroy()
  {
    if (!initialized)
    {
      return;
    }

    for (uint8_t i = 0; i < (uint8_t)GuiSpriteId::Count; i++)
    {
      if (spriteCache[i] != nullptr)
      {
        spriteCache[i]->deleteSprite();
        delete spriteCache[i];
        spriteCache[i] = nullptr;
      }
    }

    initialized = false;
  }
  
  /////////////////////////////////////////////////////
  ///////////////////GESTURES//////////////////////////
  /////////////////////////////////////////////////////
  
  bool upSwipe(lgfx::LGFX_Device& lcd);
}