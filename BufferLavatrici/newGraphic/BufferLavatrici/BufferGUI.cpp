#include "BufferGUI.h"

namespace BufferGUI
{
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

    if (spriteCache[index] == nullptr)
    {
      return;
    }

    spriteCache[index]->setColorDepth(16);

    if (!spriteCache[index]->createSprite(bitmap.width, bitmap.height))
    {
      delete spriteCache[index];
      spriteCache[index] = nullptr;
      return;
    }

    spriteCache[index]->pushImage(0, 0, bitmap.width, bitmap.height, bitmap.data);
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

  void drawSprite(lgfx::LGFX_Device& lcd, GuiSpriteId spriteId, int16_t x, int16_t y)
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

  void drawSpriteTopRight(lgfx::LGFX_Device& lcd, GuiSpriteId spriteId, int16_t margin)
  {
    const GuiBitmap16& bitmap = getBitmap(spriteId);

    int16_t x = lcd.width() - bitmap.width - margin;
    int16_t y = 9;

    drawSprite(lcd, spriteId, x, y);
  }

  void drawWifiTopRight(lgfx::LGFX_Device& lcd, bool connected, int16_t margin)
  {
    GuiSpriteId spriteId = connected
      ? GuiSpriteId::WifiConnected
      : GuiSpriteId::WifiDisconnected;

    drawSpriteTopRight(lcd, spriteId, margin);
  }

  void clearSprite(lgfx::LGFX_Device& lcd, GuiSpriteId spriteId, int16_t x, int16_t y, uint16_t backgroundColor)
  {
    const GuiBitmap16& bitmap = getBitmap(spriteId);
    lcd.fillRect(x, y, bitmap.width, bitmap.height, backgroundColor);
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
  ///////////////////APP CHROME////////////////////////
  /////////////////////////////////////////////////////

  void drawAppChrome(lgfx::LGFX_Device& lcd, bool wifiConnected)
  {
    lcd.fillScreen(UI_COLOR_BG);

    lcd.fillRect(0, 0, lcd.width(), 38, UI_COLOR_HEADER);
    lcd.drawFastHLine(0, 38, lcd.width(), UI_COLOR_BORDER);

    lcd.setTextColor(UI_COLOR_TEXT, UI_COLOR_HEADER);
    lcd.setTextSize(2);
    lcd.setCursor(10, 6);
    lcd.print("Lavatrici");

    lcd.setTextSize(1);
    lcd.setTextColor(wifiConnected ? UI_COLOR_SUCCESS : UI_COLOR_DANGER, UI_COLOR_HEADER);
    lcd.setCursor(11, 27);
    lcd.print(wifiConnected ? "ONLINE" : "OFFLINE");

    drawWifiTopRight(lcd, wifiConnected, 8);
  }

  void drawTableFrame(lgfx::LGFX_Device& lcd)
  {
    lcd.drawRoundRect(
      TABLE_AREA_X - 2,
      TABLE_AREA_Y - 2,
      TABLE_AREA_W + 4,
      TABLE_AREA_H + 4,
      8,
      UI_COLOR_BORDER
    );
  }

  void drawSpriteError(lgfx::LGFX_Device& lcd)
  {
    lcd.setTextColor(TFT_RED, TFT_BLACK);
    lcd.setTextSize(2);
    lcd.setCursor(20, 90);
    lcd.println("ERRORE SPRITE");

    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.setCursor(20, 120);
    lcd.println("Memoria insufficiente");
    lcd.setCursor(20, 135);
    lcd.println("per tableSprite.");
  }

  /////////////////////////////////////////////////////
  ///////////////////GESTURES//////////////////////////
  /////////////////////////////////////////////////////

  SwipeState swipe(int prevY, int y)
  {
    // Stessa logica del tuo progetto: variazione positiva = DOWN, negativa = UP.
    if (y - prevY > 0)
      return DOWN;

    if (y - prevY < 0)
      return UP;

    if (y - prevY == 0)
      return STILL;

    return STILL;
  }

  /////////////////////////////////////////////////////
  ///////////////////TABLE/////////////////////////////
  /////////////////////////////////////////////////////

  void initTable(TableRow* rows)
  {
    for (int i = 0; i < MAX_ROWS; i++)
    {
      rows[i].x = START_TABLE_X;
      rows[i].y = START_TABLE_Y + (i * TABLE_ROW_HEIGHT);
      snprintf(rows[i].value, sizeof(rows[i].value), "Articolo %d", i);
    }
  }

  void swipeTable(TableRow* rows, SwipeState swipeState)
  {
    Serial.println(rows[0].y);

    if (swipeState == UP)
    {
      for (int i = 0; i < MAX_ROWS; i++)
      {
        rows[i].y -= SWIPE_MOVE;
      }
      return;
    }

    if (swipeState == DOWN && rows[0].y < START_TABLE_Y)
    {
      for (int i = 0; i < MAX_ROWS; i++)
      {
        rows[i].y += SWIPE_MOVE;
      }
      return;
    }
  }

  void clearTableArea(lgfx::LGFX_Device& lcd)
  {
    lcd.fillRect(TABLE_AREA_X, TABLE_AREA_Y, TABLE_AREA_W, TABLE_AREA_H, UI_COLOR_PANEL);
  }

  void drawTable(lgfx::LGFX_Device& lcd, TableRow* rows)
  {
    clearTableArea(lcd);

    for (int i = 0; i < MAX_ROWS; i++)
    {
      if (rows[i].y + TABLE_ROW_HEIGHT < TABLE_AREA_Y)
      {
        continue;
      }

      if (rows[i].y > TABLE_AREA_Y + TABLE_AREA_H)
      {
        continue;
      }

      lcd.fillRoundRect(rows[i].x + 4, rows[i].y + 5, TABLE_ROW_WIDTH - 8, TABLE_ROW_HEIGHT - 10, 8, UI_COLOR_CARD);
      lcd.drawRoundRect(rows[i].x + 4, rows[i].y + 5, TABLE_ROW_WIDTH - 8, TABLE_ROW_HEIGHT - 10, 8, UI_COLOR_BORDER);
      lcd.setTextColor(UI_COLOR_TEXT, UI_COLOR_CARD);
      lcd.setTextSize(2);
      lcd.setCursor(rows[i].x + 14, rows[i].y + 20);
      lcd.print(rows[i].value);
    }
  }

  void drawTableSprite(TableRow* rows, lgfx::LGFX_Sprite& tableSprite)
  {
    // Questo e' il punto piu' importante: lo sprite si pulisce da solo.
    // Non ridisegno l'intero display durante lo swipe.
    tableSprite.fillSprite(UI_COLOR_PANEL);

    for (int i = 0; i < MAX_ROWS; i++)
    {
      int localX = 0;
      int localY = rows[i].y - TABLE_AREA_Y;

      if (localY + TABLE_ROW_HEIGHT < 0)
      {
        continue;
      }

      if (localY > TABLE_AREA_H)
      {
        continue;
      }

      uint16_t cardColor = (i % 2 == 0) ? UI_COLOR_CARD : UI_COLOR_CARD_ALT;

      int cardX = localX + 5;
      int cardY = localY + 5;
      int cardW = TABLE_ROW_WIDTH - 10;
      int cardH = TABLE_ROW_HEIGHT - 10;

      // Card arrotondata al posto del rettangolo bianco originale.
      tableSprite.fillRoundRect(cardX, cardY, cardW, cardH, 8, cardColor);
      tableSprite.drawRoundRect(cardX, cardY, cardW, cardH, 8, UI_COLOR_BORDER);

      // Badge numerico a sinistra.
      tableSprite.fillRoundRect(cardX + 8, cardY + 12, 26, 26, 6, UI_COLOR_PRIMARY);

      char indexText[4];
      snprintf(indexText, sizeof(indexText), "%02d", i + 1);

      tableSprite.setTextColor(TFT_BLACK, UI_COLOR_PRIMARY);
      tableSprite.setTextSize(1);
      tableSprite.setCursor(cardX + 15, cardY + 21);
      tableSprite.print(indexText);

      // Testo principale.
      tableSprite.setTextColor(UI_COLOR_TEXT, cardColor);
      tableSprite.setTextSize(2);
      tableSprite.setCursor(cardX + 42, cardY + 10);
      tableSprite.print(rows[i].value);

      // Testo secondario.
      tableSprite.setTextColor(UI_COLOR_TEXT_MUTED, cardColor);
      tableSprite.setTextSize(1);
      tableSprite.setCursor(cardX + 44, cardY + 34);
      tableSprite.print("Swipe per scorrere");
    }

    // Scrollbar solo decorativa: non cambia la logica dello swipe.
    int totalContentH = MAX_ROWS * TABLE_ROW_HEIGHT;
    int maxOffset = totalContentH - TABLE_AREA_H;

    if (maxOffset > 0)
    {
      int currentOffset = START_TABLE_Y - rows[0].y;

      if (currentOffset < 0) currentOffset = 0;
      if (currentOffset > maxOffset) currentOffset = maxOffset;

      int barH = (TABLE_AREA_H * TABLE_AREA_H) / totalContentH;
      if (barH < 24) barH = 24;

      int barY = (currentOffset * (TABLE_AREA_H - barH)) / maxOffset;

      tableSprite.fillRoundRect(TABLE_AREA_W - 5, barY + 4, 3, barH - 8, 2, UI_COLOR_PRIMARY);
    }

    tableSprite.pushSprite(TABLE_AREA_X, TABLE_AREA_Y);
  }
}
