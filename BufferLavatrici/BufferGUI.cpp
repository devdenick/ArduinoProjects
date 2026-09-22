#include "BufferGUI.h"
#include "BufferGUIElement.h"

namespace BufferGUI
{
  /////////////////////////////////////////////////////
  ///////////////////SMALL ICON SPRITES////////////////
  /////////////////////////////////////////////////////
  // Questi sprite sono solo 16x16: tenerli in RAM interna è più sensato
  // che metterli in PSRAM. La PSRAM va usata per buffer grandi, non ovunque.
  static LGFX_Sprite* spriteCache[(uint8_t)GuiSpriteId::Count] = { nullptr };
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
    const uint8_t index = toIndex(spriteId);

    if (spriteCache[index] != nullptr)
      return;

    const GuiBitmap16& bitmap = getBitmap(spriteId);

    LGFX_Sprite* sprite = new LGFX_Sprite(&lcd);
    if (sprite == nullptr)
      return;

    // Esplicito: sprite piccoli in RAM interna.
    sprite->setPsram(false);
    sprite->setColorDepth(16);

    void* buffer = sprite->createSprite(bitmap.width, bitmap.height);
    if (buffer == nullptr)
    {
      delete sprite;
      return;
    }

    sprite->setSwapBytes(true);
    sprite->pushImage(0, 0, bitmap.width, bitmap.height, bitmap.data);

    spriteCache[index] = sprite;
  }

  void begin(lgfx::LGFX_Device& lcd)
  {
    if (!initialized)
    {
      for (uint8_t i = 0; i < (uint8_t)GuiSpriteId::Count; i++)
        spriteCache[i] = nullptr;

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
      begin(lcd);

    createSpriteIfNeeded(lcd, spriteId);

    const uint8_t index = toIndex(spriteId);
    if (spriteCache[index] != nullptr)
      spriteCache[index]->pushSprite(x, y);
  }

  void drawSpriteTopRight(
    lgfx::LGFX_Device& lcd,
    GuiSpriteId spriteId,
    int16_t margin
  )
  {
    const GuiBitmap16& bitmap = getBitmap(spriteId);
    const int16_t x = lcd.width() - bitmap.width - margin;
    const int16_t y = 1;
    drawSprite(lcd, spriteId, x, y);
  }

  void drawWifiTopRight(
    lgfx::LGFX_Device& lcd,
    bool connected,
    int16_t margin
  )
  {
    drawSpriteTopRight(
      lcd,
      connected ? GuiSpriteId::WifiConnected : GuiSpriteId::WifiDisconnected,
      margin
    );
  }

  void drawMqttTopRight(
    lgfx::LGFX_Device& lcd,
    bool connected,
    int16_t margin
  )
  {
    drawSpriteTopRight(
      lcd,
      connected ? GuiSpriteId::MqttConnected : GuiSpriteId::MqttDisconnected,
      margin
    );
  }

  void destroy()
  {
    if (!initialized)
      return;

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
  ///////////////////HEADER////////////////////////////
  /////////////////////////////////////////////////////
  void drawHeader(
    lgfx::LGFX_Device& lcd,
    bool wifiConnected,
    bool mqttConnected,
    const String& macAddr,
    int cobotMission
  )
  {
    lcd.startWrite();

    lcd.fillRect(0, 0, lcd.width(), HEADER_HEIGHT, UI_COLOR_HEADER);
    lcd.drawFastHLine(0, HEADER_HEIGHT, lcd.width(), UI_COLOR_BORDER);

    drawWifiTopRight(lcd, wifiConnected, 1);
    drawMqttTopRight(lcd, mqttConnected, 22);

    lcd.setTextColor(UI_COLOR_TEXT, UI_COLOR_HEADER);
    lcd.setTextSize(2);
    lcd.setCursor(10, 6);
    lcd.print("Buffer Lavaggi");

    lcd.setTextSize(1);
    lcd.setTextColor(
      wifiConnected ? UI_COLOR_SUCCESS : UI_COLOR_DANGER,
      UI_COLOR_HEADER
    );
    lcd.setCursor(11, 27);
    lcd.print(wifiConnected ? String("ONLINE ") + macAddr : "OFFLINE");

    lcd.setTextColor(TFT_BLUE, UI_COLOR_HEADER);
    lcd.setCursor(300, 27);
    lcd.print(String("Mission:") + cobotMission);

    lcd.endWrite();
  }

  /////////////////////////////////////////////////////
  ///////////////////TABLE INTERNAL////////////////////
  /////////////////////////////////////////////////////

  static bool rowIntersectsArea(const TableRow& row)
  {
    return row.y + TABLE_ROW_HEIGHT > TABLE_AREA_Y &&
           row.y < TABLE_AREA_Y + TABLE_AREA_H;
  }

  static bool rowIntersectsClip(const TableRow& row, int clipY, int clipH)
  {
    const int clipBottom = clipY + clipH;
    return row.y + TABLE_ROW_HEIGHT > clipY && row.y < clipBottom;
  }

  static void drawTableRowInternal(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int index,
    int selectedCard,
    bool longEnough
  )
  {
    const uint16_t cardColor =
      (index == selectedCard && longEnough)
        ? UI_COLOR_CARD_SELECTED_LONG_ENOUGH
        : (index == selectedCard)
          ? UI_COLOR_CARD_SELECTED
          : ((index % 2 == 0) ? UI_COLOR_CARD : UI_COLOR_CARD_ALT);

    const int cardX = rows[index].x + 5;
    const int cardY = rows[index].y + 5;
    const int cardW = TABLE_ROW_WIDTH - 10;
    const int cardH = TABLE_ROW_HEIGHT - 10;

    lcd.fillRoundRect(cardX, cardY, cardW, cardH, 8, cardColor);
    lcd.drawRoundRect(cardX, cardY, cardW, cardH, 8, UI_COLOR_BORDER);

    lcd.fillRoundRect(cardX + 8, cardY + 12, 26, 26, 6, UI_COLOR_PRIMARY);

    lcd.setTextColor(TFT_BLACK, UI_COLOR_PRIMARY);
    lcd.setTextSize(2);
    lcd.setCursor(cardX + 16, cardY + 18);
    lcd.print(rows[index].linea);

    lcd.setTextColor(UI_COLOR_TEXT, cardColor);
    lcd.setTextSize(1.5f);
    lcd.setCursor(cardX + 42, cardY + 10);
    lcd.print(rows[index].nome);

    lcd.setTextColor(UI_COLOR_TEXT_MUTED, cardColor);
    lcd.setTextSize(1.2f);
    lcd.setCursor(cardX + 44, cardY + 34);
    lcd.print(rows[index].lotto);
  }

  static void drawRowsInClip(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount,
    int clipY,
    int clipH,
    int selectedCard,
    bool longEnough
  )
  {
    if (clipH <= 0)
      return;

    lcd.setClipRect(
      TABLE_AREA_X,
      clipY,
      TABLE_CONTENT_W,
      clipH
    );

    for (int i = 0; i < rowsCount; i++)
    {
      if (rowIntersectsClip(rows[i], clipY, clipH))
        drawTableRowInternal(lcd, rows, i, selectedCard, longEnough);
    }

    lcd.clearClipRect();
  }

  static void drawScrollbar(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount
  )
  {
    const int scrollX = TABLE_AREA_X + TABLE_CONTENT_W;
    const int scrollZoneW = TABLE_AREA_W - TABLE_CONTENT_W;

    // Pulisce sempre la zona scrollbar per evitare residui durante copyRect.
    lcd.fillRect(
      scrollX,
      TABLE_AREA_Y,
      scrollZoneW,
      TABLE_AREA_H,
      UI_COLOR_PANEL
    );

    if (rowsCount <= 0)
      return;

    const int totalContentH = rowsCount * TABLE_ROW_HEIGHT;
    const int maxOffset = totalContentH - TABLE_AREA_H;

    if (maxOffset <= 0)
      return;

    int currentOffset = START_TABLE_Y - rows[0].y;
    if (currentOffset < 0) currentOffset = 0;
    if (currentOffset > maxOffset) currentOffset = maxOffset;

    int barH = (TABLE_AREA_H * TABLE_AREA_H) / totalContentH;
    if (barH < 28) barH = 28;
    if (barH > TABLE_AREA_H - 8) barH = TABLE_AREA_H - 8;

    const int trackH = TABLE_AREA_H - 8;
    const int barTravel = trackH - barH;
    const int barY = TABLE_AREA_Y + 4 +
      ((barTravel > 0) ? (currentOffset * barTravel) / maxOffset : 0);

    lcd.fillRoundRect(
      TABLE_AREA_X + TABLE_AREA_W - 5,
      barY,
      3,
      barH,
      2,
      UI_COLOR_PRIMARY
    );
  }

  /////////////////////////////////////////////////////
  ///////////////////TABLE PUBLIC//////////////////////
  /////////////////////////////////////////////////////

  void initTable(TableRow* rows)
  {
    for (int i = 0; i < MAX_ROWS; i++)
    {
      rows[i].x = START_TABLE_X;
      rows[i].y = START_TABLE_Y + (i * TABLE_ROW_HEIGHT);
      snprintf(rows[i].nome, sizeof(rows[i].nome), "Articolo %d", i);
    }
  }

  void drawTable(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount,
    int selectedCard,
    bool longEnough
  )
  {
    lcd.startWrite();

    lcd.fillRect(
      TABLE_AREA_X,
      TABLE_AREA_Y,
      TABLE_AREA_W,
      TABLE_AREA_H,
      UI_COLOR_PANEL
    );

    lcd.setClipRect(
      TABLE_AREA_X,
      TABLE_AREA_Y,
      TABLE_CONTENT_W,
      TABLE_AREA_H
    );

    for (int i = 0; i < rowsCount; i++)
    {
      if (rowIntersectsArea(rows[i]))
        drawTableRowInternal(lcd, rows, i, selectedCard, longEnough);
    }

    lcd.clearClipRect();
    drawScrollbar(lcd, rows, rowsCount);

    lcd.endWrite();
  }

  void redrawTableRow(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount,
    int rowIndex,
    bool selected,
    bool longEnough
  )
  {
    if (rowIndex < 0 || rowIndex >= rowsCount)
      return;

    if (!rowIntersectsArea(rows[rowIndex]))
      return;

    lcd.startWrite();

    lcd.setClipRect(
      TABLE_AREA_X,
      TABLE_AREA_Y,
      TABLE_CONTENT_W,
      TABLE_AREA_H
    );

    // Pulisce soltanto la banda della riga interessata.
    lcd.fillRect(
      TABLE_AREA_X,
      rows[rowIndex].y,
      TABLE_CONTENT_W,
      TABLE_ROW_HEIGHT,
      UI_COLOR_PANEL
    );

    drawTableRowInternal(
      lcd,
      rows,
      rowIndex,
      selected ? rowIndex : -1,
      longEnough
    );

    lcd.clearClipRect();
    lcd.endWrite();
  }

  int scrollTableByPixels(
    lgfx::LGFX_Device& lcd,
    TableRow* rows,
    int rowsCount,
    int deltaY
  )
  {
    if (rowsCount <= 0 || deltaY == 0)
      return 0;

    const int totalContentH = rowsCount * TABLE_ROW_HEIGHT;
    if (totalContentH <= TABLE_AREA_H)
      return 0;

    // Limite superiore: la prima riga non deve scendere oltre START_TABLE_Y.
    const int maxDeltaDown = START_TABLE_Y - rows[0].y;

    // Limite inferiore: il fondo dell'ultima riga non deve salire oltre il fondo viewport.
    const int lastBottom = rows[rowsCount - 1].y + TABLE_ROW_HEIGHT;
    const int tableBottom = START_TABLE_Y + TABLE_AREA_H;
    const int maxDeltaUp = tableBottom - lastBottom; // valore <= 0

    int applied = deltaY;
    if (applied > maxDeltaDown) applied = maxDeltaDown;
    if (applied < maxDeltaUp) applied = maxDeltaUp;

    if (applied == 0)
      return 0;

    // Evita richieste assurde in un singolo frame.
    if (applied >= TABLE_AREA_H) applied = TABLE_AREA_H - 1;
    if (applied <= -TABLE_AREA_H) applied = -(TABLE_AREA_H - 1);

    const int shift = abs(applied);

    lcd.startWrite();

    // Spostiamo direttamente i pixel già presenti nel framebuffer RGB.
    // In questo modo durante lo swipe non ricostruiamo e non pushiamo
    // uno sprite enorme 440x700 ad ogni frame.
    if (applied < 0)
    {
      // Contenuto verso l'alto.
      lcd.copyRect(
        TABLE_AREA_X,
        TABLE_AREA_Y,
        TABLE_CONTENT_W,
        TABLE_AREA_H - shift,
        TABLE_AREA_X,
        TABLE_AREA_Y + shift
      );
    }
    else
    {
      // Contenuto verso il basso.
      lcd.copyRect(
        TABLE_AREA_X,
        TABLE_AREA_Y + shift,
        TABLE_CONTENT_W,
        TABLE_AREA_H - shift,
        TABLE_AREA_X,
        TABLE_AREA_Y
      );
    }

    // Aggiorna la posizione logica delle righe con lo stesso identico delta.
    for (int i = 0; i < rowsCount; i++)
      rows[i].y += applied;

    // Ridisegna soltanto la striscia appena entrata nella viewport.
    int exposedY;
    if (applied < 0)
      exposedY = TABLE_AREA_Y + TABLE_AREA_H - shift;
    else
      exposedY = TABLE_AREA_Y;

    lcd.fillRect(
      TABLE_AREA_X,
      exposedY,
      TABLE_CONTENT_W,
      shift,
      UI_COLOR_PANEL
    );

    drawRowsInClip(
      lcd,
      rows,
      rowsCount,
      exposedY,
      shift,
      -1,
      false
    );

    drawScrollbar(lcd, rows, rowsCount);

    lcd.endWrite();

    return applied;
  }

  int tableRowHitbox(TableRow* rows, int x, int y, int rowsCount)
  {
    if (
      x < TABLE_AREA_X ||
      x >= TABLE_AREA_X + TABLE_CONTENT_W ||
      y < TABLE_AREA_Y ||
      y >= TABLE_AREA_Y + TABLE_AREA_H
    )
      return -1;

    for (int i = 0; i < rowsCount; i++)
    {
      const int cardX = rows[i].x + 5;
      const int cardY = rows[i].y + 5;
      const int cardW = TABLE_ROW_WIDTH - 10;
      const int cardH = TABLE_ROW_HEIGHT - 10;

      if (
        x >= cardX && x < cardX + cardW &&
        y >= cardY && y < cardY + cardH
      )
        return i;
    }

    return -1;
  }

  void clearTableArea(lgfx::LGFX_Device& lcd)
  {
    lcd.fillRect(
      TABLE_AREA_X,
      TABLE_AREA_Y,
      TABLE_AREA_W,
      TABLE_AREA_H,
      UI_COLOR_BG
    );
  }

  void clearTableRowsData(TableRow* rows)
  {
    for (int i = 0; i < MAX_ROWS; i++)
    {
      rows[i].id = -1;
      rows[i].nome[0] = '\0';
      rows[i].startTimestamp[0] = '\0';
      rows[i].lotto[0] = '\0';
      rows[i].linea = '\0';
    }
  }

  /////////////////////////////////////////////////////
  ///////////////////WAITING PANEL/////////////////////
  /////////////////////////////////////////////////////
  // Anche qui niente sprite grande: il box statico viene disegnato una volta,
  // poi ogni refresh cambia soltanto la piccola zona dei puntini.
  static bool waitingPanelVisible = false;
  static String lastWaitingMessage = "";
  static uint8_t waitingDots = 0;

  void drawWaitingPanel(
    lgfx::LGFX_Device& lcd,
    const String& waitingMessage
  )
  {
    const int panelX = WAITING_PANEL_X + WAITING_PANEL_BOX_MARGIN;
    const int panelY = WAITING_PANEL_Y + WAITING_PANEL_BOX_MARGIN;
    const int panelW = WAITING_PANEL_BOX_WIDTH;
    const int panelH = WAITING_PANEL_BOX_HEIGHT;

    lcd.startWrite();

    if (!waitingPanelVisible || lastWaitingMessage != waitingMessage)
    {
      lcd.fillRect(
        WAITING_PANEL_X,
        WAITING_PANEL_Y,
        WAITING_PANEL_WIDTH,
        WAITING_PANEL_HEIGHT,
        UI_COLOR_BG
      );

      lcd.fillRoundRect(panelX, panelY, panelW, panelH, 8, UI_COLOR_CARD);
      lcd.drawRoundRect(panelX, panelY, panelW, panelH, 8, UI_COLOR_BORDER);

      lcd.setTextColor(UI_COLOR_TEXT, UI_COLOR_CARD);
      lcd.setTextSize(2);
      lcd.setCursor(panelX + 21, panelY + 15);
      lcd.print(waitingMessage);

      waitingPanelVisible = true;
      lastWaitingMessage = waitingMessage;
    }

    // Pulisce e aggiorna solo l'area dei puntini.
    const int dotsX = panelX + 80;
    const int dotsY = panelY + 40;
    const int dotsW = panelW - 95;
    const int dotsH = 24;

    lcd.fillRect(dotsX, dotsY, dotsW, dotsH, UI_COLOR_CARD);

    String dots;
    for (uint8_t i = 0; i < waitingDots; i++)
      dots += ".";

    waitingDots = (waitingDots + 1) % 4;

    lcd.setTextColor(UI_COLOR_TEXT_MUTED, UI_COLOR_CARD);
    lcd.setTextSize(2);
    lcd.setCursor(dotsX, dotsY);
    lcd.print(dots);

    lcd.endWrite();
  }

  void clearWaitingPanel(lgfx::LGFX_Device& lcd)
  {
    lcd.fillRect(
      WAITING_PANEL_X,
      WAITING_PANEL_Y,
      WAITING_PANEL_WIDTH,
      WAITING_PANEL_HEIGHT,
      UI_COLOR_BG
    );

    waitingPanelVisible = false;
    lastWaitingMessage = "";
    waitingDots = 0;
  }
}
