#include "BufferGUI.h"
#include "BufferGuiElement.h"

namespace BufferGUI
{
  using namespace BufferGUI;

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

    spriteCache[index]->setSwapBytes(true); 

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
    createSpriteIfNeeded(lcd, GuiSpriteId::MqttConnected);
    createSpriteIfNeeded(lcd, GuiSpriteId::MqttDisconnected);
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

  void drawMqttTopRight(
  lgfx::LGFX_Device& lcd,
  bool connected,
  int16_t margin
  )
  {
    GuiSpriteId spriteId = connected
      ? GuiSpriteId::MqttConnected
      : GuiSpriteId::MqttDisconnected;

    drawSprite(lcd, spriteId, 205, 1);
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
  
  SwipeState swipe(int prevY, int y){
    if(y > (prevY + SWIPE_SENS))
      return DOWN;
    else if(y < (prevY - SWIPE_SENS))
      return UP;
    else
      return STILL;
  }

  /////////////////////////////////////////////////////
  ///////////////////HEADER////////////////////////////
  /////////////////////////////////////////////////////

  void drawHeader(lgfx::LGFX_Device& lcd, bool wifiConnected, bool mqttConnected, String macAddr){
    lcd.fillRect(0, 0, lcd.width(), HEADER_HEIGHT, UI_COLOR_HEADER);
    drawWifiTopRight(lcd, wifiConnected);
    drawMqttTopRight(lcd, mqttConnected);

    lcd.drawFastHLine(0, 38, lcd.width(), UI_COLOR_BORDER);

    lcd.setTextColor(UI_COLOR_TEXT, UI_COLOR_HEADER);
    lcd.setTextSize(2);
    lcd.setCursor(10, 6);
    lcd.print("Buffer Lavaggi");

    lcd.setTextSize(1);
    lcd.setTextColor(wifiConnected ? UI_COLOR_SUCCESS : UI_COLOR_DANGER, UI_COLOR_HEADER);
    lcd.setCursor(11, 27);
    String online = "ONLINE "+macAddr;
    String offline = "OFFLINE";
    lcd.print(wifiConnected ? online : offline);

  }

  /////////////////////////////////////////////////////
  ///////////////////TABLE/////////////////////////////
  /////////////////////////////////////////////////////

  void initTable(TableRow* rows){
    for(int i = 0; i < MAX_ROWS; i++){
      rows[i].x = START_TABLE_X;
      rows[i].y = START_TABLE_Y + (i * TABLE_ROW_HEIGHT);
      snprintf(rows[i].nome, sizeof(rows[i].nome), "Articolo %d", i);
    }
  }

  void swipeTable(TableRow* rows, SwipeState swipeState, int rowsCount){
    if(rowsCount * TABLE_ROW_HEIGHT < TABLE_AREA_H)//no need to swipe, few rows
      return;
    if(swipeState == UP && rows[rowsCount-1].y + TABLE_ROW_HEIGHT > (START_TABLE_Y + TABLE_AREA_H)){
      for(int i = 0; i < rowsCount; i++){
        rows[i].y -= SWIPE_MOVE;
      }
      return;
    }
    if(swipeState == DOWN && rows[0].y < START_TABLE_Y){
      for(int i = 0; i < rowsCount; i++){
        rows[i].y += SWIPE_MOVE;
      }
      return;
    }
  }

  void drawTableSprite(TableRow* rows, lgfx::LGFX_Sprite& tableSprite,int rowsCount, int selectedCard)
  {
    tableSprite.fillSprite(UI_COLOR_PANEL);

    for (int i = 0; i < rowsCount; i++)
    {
      //local sprite coords
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

      uint16_t cardColor;
      if(i == selectedCard)
        cardColor = UI_COLOR_CARD_SELECTED;
      else
        cardColor = (i % 2 == 0) ? UI_COLOR_CARD : UI_COLOR_CARD_ALT;

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
      tableSprite.setTextSize(1.5);
      tableSprite.setCursor(cardX + 42, cardY + 10);
      tableSprite.print(rows[i].nome);

      // Testo secondario.
      tableSprite.setTextColor(UI_COLOR_TEXT_MUTED, cardColor);
      tableSprite.setTextSize(1);
      tableSprite.setCursor(cardX + 44, cardY + 34);
      tableSprite.print(rows[i].startTimestamp);
    }

    // Scrollbar solo decorativa: non cambia la logica dello swipe.
    int totalContentH = rowsCount * TABLE_ROW_HEIGHT;
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

  int tableRowHitbox(TableRow* rows, int x, int y, int rowsCount){
    for(int i = 0; i < rowsCount; i++){
      int cardX = rows[i].x + 5;
      int cardWidth = TABLE_ROW_WIDTH - 10;
      int cardY = rows[i].y + 5;
      int cardHeight = TABLE_ROW_HEIGHT - 10;
      if(x >= cardX && x <= cardX + cardWidth && y >= cardY && y <= cardY + cardHeight && y >= TABLE_AREA_Y){
        return i;
      }
    }
    return -1;
  }

  void clearTable(lgfx::LGFX_Device& lcd)
  {
    lcd.fillRect(TABLE_AREA_X, TABLE_AREA_Y, TABLE_AREA_W, TABLE_AREA_H, UI_COLOR_PANEL);
  }

  void clearTableArea(lgfx::LGFX_Device& lcd)
  {
    lcd.fillRect(TABLE_AREA_X, TABLE_AREA_Y, TABLE_AREA_W, TABLE_AREA_H, UI_COLOR_BG);
  }

  void clearTableRowsData(TableRow* rows)
  {
    for (int i = 0; i < BufferGUI::MAX_ROWS; i++) {
      rows[i].id = -1;
      rows[i].nome[0] = '\0';
      rows[i].startTimestamp[0] = '\0';
    }
  }

  void drawWaitingPanel(lgfx::LGFX_Sprite& waitingPanelSprite, String waitingMessage)
  {
    waitingPanelSprite.fillSprite(UI_COLOR_PANEL);

    //local sprite coords
    int panelX = WAITING_PANEL_BOX_MARGIN;
    int panelY = WAITING_PANEL_BOX_MARGIN;
    int panelW = WAITING_PANEL_BOX_WIDTH;
    int panelH = WAITING_PANEL_BOX_HEIGHT;

    waitingPanelSprite.fillRoundRect(panelX, panelY, panelW, panelH, 8, UI_COLOR_CARD);
    waitingPanelSprite.drawRoundRect(panelX, panelY, panelW, panelH, 8, UI_COLOR_BORDER);

    String waiting = "";
    for(int i = 0; i < waitingCounter; i++){
      waiting = waiting + ".";
    }
    waitingCounter = (waitingCounter + 1) % 4;

    // Testo principale.
    waitingPanelSprite.setTextColor(UI_COLOR_TEXT, UI_COLOR_CARD);
    waitingPanelSprite.setTextSize(2);
    waitingPanelSprite.setCursor(panelX + 21, panelY + 15);
    waitingPanelSprite.print(waitingMessage);
    waitingPanelSprite.setTextColor(UI_COLOR_TEXT_MUTED, UI_COLOR_CARD);
    waitingPanelSprite.setTextSize(2);
    waitingPanelSprite.setCursor(panelX + 87, panelY + 40);
    waitingPanelSprite.print(waiting);


    waitingPanelSprite.pushSprite(WAITING_PANEL_X, WAITING_PANEL_Y);
  }

  void clearWaitingPanel(lgfx::LGFX_Sprite& waitingPanelSprite){
    waitingPanelSprite.fillSprite(UI_COLOR_BG);
    waitingPanelSprite.pushSprite(WAITING_PANEL_X, WAITING_PANEL_Y);
  }

  
}