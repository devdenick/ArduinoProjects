#include "LGFX_4D_43CT.h"
#include <WiFi.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <ElegantOTA.h>
#include <WebServer.h>

// ============================================================
// BUFFER LAVAGGI - DEMO GRAFICA
// gen4-ESP32-43CT + LovyanGFX
//
// Nessun WiFi/MQTT reale: gli stati sono simulati.
// Serve come base grafica da integrare nel progetto vero.
// ============================================================

LGFX_4D_43CT lcd;

// Sprite dedicato all'icona AMR + Cobot.
// La disegniamo qui e poi la copiamo sul display con un solo pushSprite().
// Questo evita artefatti durante lo scroll parziale della lista.
LGFX_Sprite amrCobotSprite(&lcd);
int amrCobotSpriteSize = 0;

// Sprite dedicato al marquee del nome articolo.
// Il testo viene composto OFF-SCREEN e trasferito in un colpo solo,
// evitando il lampeggio dovuto a fillRect() + drawString() sul display.
LGFX_Sprite articleNameSprite(&lcd);
int articleNameSpriteW = 0;
int articleNameSpriteH = 0;

// Sprite dedicato alla barra animata della finestra di scaricamento AMR.
// Serve a mantenere l'animazione fluida senza flicker.
LGFX_Sprite bufferUnloadBarSprite(&lcd);
bool bufferUnloadBarSpriteReady = false;

// ============================================================
// COLORI
// ============================================================

uint16_t COL_BG;
uint16_t COL_SURFACE;
uint16_t COL_CARD;
uint16_t COL_CARD_ALT;
uint16_t COL_BORDER;
uint16_t COL_TEXT;
uint16_t COL_MUTED;
uint16_t COL_BLUE;
uint16_t COL_BLUE_SOFT;
uint16_t COL_GREEN;
uint16_t COL_ORANGE;
uint16_t COL_RED;
uint16_t COL_DARK_BUTTON;

// ============================================================
// DIMENSIONI DISPLAY
// ============================================================

static constexpr int SCREEN_W = 480;
static constexpr int SCREEN_H = 800;

// Header
static constexpr int HEADER_X = 12;
static constexpr int HEADER_Y = 12;
static constexpr int HEADER_W = 456;
static constexpr int HEADER_H = 84;

// Logo chip nell'header
static constexpr int CHIP_LOGO_CX = 44;
static constexpr int CHIP_LOGO_CY = 54;
static constexpr int CHIP_LOGO_RADIUS = 22;

// Riga MAC / missione
static constexpr int INFO_X = 12;
static constexpr int INFO_Y = 104;
static constexpr int INFO_W = 456;
static constexpr int INFO_H = 54;

// Riquadro Missione
static constexpr int MISSION_X = 344;
static constexpr int MISSION_Y = 114;
static constexpr int MISSION_W = 110;
static constexpr int MISSION_H = 34;

// Summary
static constexpr int SUMMARY_Y = 166;
static constexpr int SUMMARY_H = 82;
static constexpr int SUMMARY_GAP = 8;
static constexpr int SUMMARY_CARD_W = 146;

// Lista
static constexpr int LIST_X = 12;
static constexpr int LIST_Y = 164;
static constexpr int LIST_W = 456;
static constexpr int UI_LIST_H = 557;

static constexpr int LIST_INNER_X = LIST_X + 10;
static constexpr int LIST_INNER_Y = LIST_Y + 10;
static constexpr int LIST_INNER_W = LIST_W - 34; // spazio scrollbar
static constexpr int LIST_INNER_H = UI_LIST_H - 20;

static constexpr int SCROLLBAR_X = LIST_X + LIST_W - 14;
static constexpr int SCROLLBAR_Y = LIST_Y + 12;
static constexpr int SCROLLBAR_W = 5;
static constexpr int SCROLLBAR_H = UI_LIST_H - 24;

// Righe
static constexpr int ROW_H = 150;
static constexpr int ROW_GAP = 8;
static constexpr int ROW_STEP = ROW_H + ROW_GAP;
static constexpr int ROW_W = LIST_INNER_W;
static constexpr int ROW_NUMBER_W = (ROW_H * 70) / 100;
static constexpr int ROW_NUMBER_H = (ROW_H * 69) / 100;
static constexpr int ROW_NUMBER_OFFSET_X = 12;
static constexpr int ROW_NUMBER_OFFSET_Y = (ROW_H-ROW_NUMBER_H)/2;
static constexpr int ROW_TITLE_OFFSET_X = ROW_NUMBER_OFFSET_X + ROW_NUMBER_W + 20;
static constexpr int ROW_TITLE_OFFSET_Y = ROW_NUMBER_OFFSET_Y + 15;
static constexpr int ROW_TEXT_OFFSET_X = ROW_NUMBER_OFFSET_X + ROW_NUMBER_W + 20;
static constexpr int ROW_TEXT_OFFSET_Y = ROW_NUMBER_OFFSET_Y+ROW_NUMBER_H - 25;
static constexpr int MAX_ROWS = 50;

// Bottoni
static constexpr int BUTTON_Y = 729;
static constexpr int BUTTON_H = 62;
static constexpr int BUTTON_GAP = 10;
static constexpr int BUTTON_LEFT_X = 12;
static constexpr int BUTTON_LEFT_W = 174;
static constexpr int BUTTON_RIGHT_X = BUTTON_LEFT_X + BUTTON_LEFT_W + BUTTON_GAP;
static constexpr int BUTTON_RIGHT_W = 480 - BUTTON_RIGHT_X - 12;

// Footer
static constexpr int FOOTER_Y = 756;


// ============================================================
// DATI DEMO
// ============================================================

enum ItemState
{
  STATE_READY,
  STATE_WAITING,
  STATE_RUNNING
};

struct DemoItem
{
  const char* name;
  const char* code;
  int quantity;
  ItemState state;
};

typedef struct {
    char nome[128]; // aumentato per non troncare i nomi lunghi
    char startTimestamp[32];
    int id;
    char linea;
    char lotto[32];
    int articoloId;
  } Article;

Article articles[MAX_ROWS];
int rowCount = 0;

DemoItem items[] =
{
  {"Articolo 01", "A001", 120, STATE_READY},
  {"Articolo 02", "A002", 85,  STATE_WAITING},
  {"Articolo 03", "A003", 64,  STATE_RUNNING},
  {"Articolo 04", "A004", 40,  STATE_READY},
  {"Articolo 05", "A005", 32,  STATE_WAITING},
  {"Articolo 06", "A006", 18,  STATE_READY},
  {"Articolo 07", "A007", 75,  STATE_WAITING},
  {"Articolo 08", "A008", 91,  STATE_READY},
  {"Articolo 09", "A009", 54,  STATE_RUNNING},
  {"Articolo 10", "A010", 27,  STATE_READY},
  {"Articolo 11", "A011", 66,  STATE_WAITING},
  {"Articolo 12", "A012", 44,  STATE_READY}
};

static constexpr int ITEM_COUNT = sizeof(items) / sizeof(items[0]);

//wifi
WiFiClient wifiClient;
const char* ssid = "ZFIOT";
const char* password = "XGWgxXcyLiyaMY4n9YDg9CKC";//CC:8D:A2:0C:17:70 pass duCDhviViQXeEmgU3ARQHkRE, CC:8D:A2:0C:17:34 pass GwGSXud3jbgfjWuxdXiaKc6S, CC-8D-A2-0C-11-F0  pass XGWgxXcyLiyaMY4n9YDg9CKC, CC-8D-A2-0C-11-F4 pass CFKzDgkp73zgC5vmbPtJTzfk 
String macAddress = "";
bool wifiConnected = false;

//mqtt
const char broker[] = "10.18.129.41";
int        port     = 1883;
MqttClient mqttClient(wifiClient);
char notifyTopic[] = "flowrack/buffer/connected";
char selectTopic[] = "flowrack/buffer/selected";
char readListTopic[] = "flowrack/buffer/update";
bool mqttConnected = false;
bool listUpdated = false;

int bufferMission = 999;

int selectedItem = -1;
int scrollOffset = 0;

// ============================================================
// TOUCH
// ============================================================

bool touchDown = false;
bool touchMoved = false;
int touchStartX = 0;
int touchStartY = 0;
int lastTouchX = 0;
int lastTouchY = 0;
unsigned long lastScrollFrame = 0;

static constexpr int TAP_MOVE_TOLERANCE = 12;
static constexpr int SCROLL_FRAME_MS = 20;

// Scorrimento orizzontale automatico dei nomi articolo troppo lunghi.
static constexpr unsigned long ARTICLE_NAME_SCROLL_FRAME_MS = 33;
static constexpr unsigned long ARTICLE_NAME_START_PAUSE_MS = 500;
static constexpr unsigned long ARTICLE_NAME_END_PAUSE_MS = 900;
static constexpr int ARTICLE_NAME_SCROLL_SPEED_PX_S = 50;

unsigned long lastArticleNameScrollFrame = 0;
unsigned long articleNameScrollEpoch = 0;

// Ultima posizione marquee realmente disegnata per ogni riga.
// Evita push identici consecutivi.
int articleNameLastOffset[MAX_ROWS];

// Long press sul riquadro Missione
static constexpr unsigned long MISSION_LONG_PRESS_MS = 5000;
static constexpr int MISSION_LONG_PRESS_MOVE_TOLERANCE = 20;

// Long press sull'header
static constexpr unsigned long HEADER_LONG_PRESS_MS = 2000;
static constexpr int HEADER_LONG_PRESS_MOVE_TOLERANCE = 20;

bool missionLongPressCandidate = false;
unsigned long missionLongPressStart = 0;
int missionLongPressLastProgress = -1;

bool headerLongPressCandidate = false;
unsigned long headerLongPressStart = 0;
int headerPressLastProgress = -1;

// Finestra informazioni ESP32
bool espInfoOpen = false;
bool espInfoTouchDown = false;
bool espInfoIgnoreUntilRelease = false;
int espInfoTouchStartX = 0;
int espInfoTouchStartY = 0;
int espInfoLastTouchX = 0;
int espInfoLastTouchY = 0;

static constexpr int ESP_INFO_X = 24;
static constexpr int ESP_INFO_Y = 108;
static constexpr int ESP_INFO_W = 432;
static constexpr int ESP_INFO_H = 584;

static constexpr int ESP_INFO_CLOSE_X = 126;
static constexpr int ESP_INFO_CLOSE_Y = 620;
static constexpr int ESP_INFO_CLOSE_W = 228;
static constexpr int ESP_INFO_CLOSE_H = 52;

// Editor missione
bool missionEditorOpen = false;
bool missionEditorTouchDown = false;
bool missionEditorIgnoreUntilRelease = false;
int missionEditorTouchStartX = 0;
int missionEditorTouchStartY = 0;
int missionEditorLastTouchX = 0;
int missionEditorLastTouchY = 0;

String missionEditorValue = "";
String missionEditorMessage = "";

static constexpr int MISSION_MAX_DIGITS = 9;

static constexpr int EDITOR_X = 30;
static constexpr int EDITOR_Y = 170;
static constexpr int EDITOR_W = 420;
static constexpr int EDITOR_H = 500;

static constexpr int EDITOR_DISPLAY_X = 58;
static constexpr int EDITOR_DISPLAY_Y = 238;
static constexpr int EDITOR_DISPLAY_W = 364;
static constexpr int EDITOR_DISPLAY_H = 64;

static constexpr int KEYPAD_X = 58;
static constexpr int KEYPAD_Y = 320;
static constexpr int KEY_W = 112;
static constexpr int KEY_H = 55;
static constexpr int KEY_GAP_X = 12;
static constexpr int KEY_GAP_Y = 9;

static constexpr int EDITOR_ACTION_Y = 590;
static constexpr int EDITOR_ACTION_W = 172;
static constexpr int EDITOR_ACTION_H = 52;
static constexpr int EDITOR_ACTION_GAP = 20;

// ============================================================
// FINESTRA SCARICAMENTO BUFFER AMR
// ============================================================

bool bufferUnloadOpen = false;
bool bufferUnloadTouchDown = false;

int bufferUnloadTouchStartX = 0;
int bufferUnloadTouchStartY = 0;
int bufferUnloadLastTouchX = 0;
int bufferUnloadLastTouchY = 0;

unsigned long bufferUnloadAnimationStart = 0;
unsigned long bufferUnloadLastFrame = 0;

static constexpr int BUFFER_UNLOAD_X = 12;
static constexpr int BUFFER_UNLOAD_Y = 108;
static constexpr int BUFFER_UNLOAD_W = 456;
static constexpr int BUFFER_UNLOAD_H = 608;

static constexpr int BUFFER_UNLOAD_LOGO_SIZE = 150;
static constexpr int BUFFER_UNLOAD_LOGO_X =
  (SCREEN_W - BUFFER_UNLOAD_LOGO_SIZE) / 2;
static constexpr int BUFFER_UNLOAD_LOGO_Y = 225;

static constexpr int BUFFER_UNLOAD_BAR_X = 70;
static constexpr int BUFFER_UNLOAD_BAR_Y = 470;
static constexpr int BUFFER_UNLOAD_BAR_W = 340;
static constexpr int BUFFER_UNLOAD_BAR_H = 24;

static constexpr int BUFFER_UNLOAD_BUTTON_X = 92;
static constexpr int BUFFER_UNLOAD_BUTTON_Y = 620;
static constexpr int BUFFER_UNLOAD_BUTTON_W = 296;
static constexpr int BUFFER_UNLOAD_BUTTON_H = 64;

static constexpr unsigned long BUFFER_UNLOAD_ANIM_FRAME_MS = 33;
static constexpr unsigned long BUFFER_UNLOAD_ANIM_PERIOD_MS = 1800;

// ============================================================
// TOAST
// ============================================================

String toastMessage = "";
unsigned long toastUntil = 0;

// ============================================================
// FUNCTIONS
// ============================================================

///////////////////////UTILS///////////////////////
int clampInt(int value, int minValue, int maxValue);
int maxScrollOffset();
void drawText(int x, int y, const String& text, uint16_t color, const lgfx::IFont* font = &fonts::Font2, float size = 1.0f, textdatum_t datum = textdatum_t::top_left);
void drawTextCentered(int x, int y, int w, int h, const String& text, uint16_t color, const lgfx::IFont* font = &fonts::Font2, float size = 1.0f);
bool pointInRect(int x, int y, int rx, int ry, int rw, int rh);
int itemAtPoint(int x, int y);
void setupWiFiConnection();
void checkWiFiConnection();
void setupMqttCommunication();
void handleMqttMessages();
void publishNotifyTopic();
void publishArticleSelected(int selectedArticle);
bool loadArticlesFromJson(const String& json);
void safeCopy(char* dest, size_t destSize, const char* source);

///////////////////////GUI ELEMENT///////////////////////
void handleTouch();
void handleTap(int x, int y);
void drawDashboard();//metodo che richiama tutti i draw
void drawHeader();
void drawInfoRow();
void drawListFull();
void drawRowsIntersecting(int clipY, int clipH);
void drawScrollbar();
void drawListRow(int index, int y);
void drawArticleName(int index, int rowY);
void updateArticleNameScroll();
int getArticleNameTextWidth(const char* text);
int getArticleNameScrollOffset(int index, int availableWidth);

void setScrollOffsetFast(int requestedOffset);

void drawButtons();

void drawHeaderLongPressProgress();
void drawMissionLongPressProgress();

void openEspInfoScreen();
void closeEspInfoScreen();
void handleEspInfoTouch(bool pressed, int x, int y);
void drawEspInfoScreen();
void drawEspInfoLine(int y, const String& label, const String& value, uint16_t valueColor = COL_TEXT);

void openMissionEditor();
void closeMissionEditor();
void drawMissionEditor();
void drawMissionEditorValue();
void drawMissionEditorKey(int col, int row, const String& label, uint16_t fillColor);
void appendMissionDigit(char digit);
void handleMissionEditorTouch(bool pressed, int x, int y);
void handleMissionEditorTap(int x, int y);

void openBufferUnloadWindow();
void closeBufferUnloadWindow();
void drawBufferUnloadWindow();
void drawBufferUnloadLoadingBar();
void updateBufferUnloadAnimation();
void handleBufferUnloadTouch(bool pressed, int x, int y);

void drawDropLogo(int cx, int cy);
void drawChipLogo(int cx, int cy);
void drawChipCore(int cx, int cy);
void drawStatusChip(int x, int y, int w, const char* label, bool ok);
void drawRefreshIcon(int cx, int cy, uint16_t color);
void drawCheckIcon(int cx, int cy, uint16_t color);
void drawAmrCobotIcon(int x, int y, int size, uint16_t color, uint16_t bgColor, uint16_t borderColor);

/////////////////////////////////////////////////////////////////////

// ============================================================
// OTA 
// ============================================================
WebServer server(80);

unsigned long ota_progress_millis = 0;

bool otaUpdate = false;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  otaUpdate = true;
  mqttClient.stop();
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  otaUpdate = false;
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
  //Serial.begin(115200);
  delay(400);

  lcd.init();
  lcd.setColorDepth(16);

  // Portrait. Se lo preferisci girato dall'altro lato, usa 1
  // e modifica offset_rotation nel file LGFX_4D_43CT.h da 7 a 5.
  lcd.setRotation(3);
  lcd.setBrightness(255);

  // Evita che una stringa troppo lunga venga mandata automaticamente
  // a capo dentro una clip area, cosa che sulla lista può sembrare
  // testo sovrapposto o "capovolto".
  lcd.setTextWrap(false, false);
  lcd.setTextDatum(textdatum_t::top_left);
  lcd.setFont(&fonts::Font2);
  lcd.setTextSize(1.0f);

  for (int i = 0; i < MAX_ROWS; i++) {
    articleNameLastOffset[i] = -1;
  }

  // Palette colori
  COL_BG          = lcd.color565(7, 13, 22);
  COL_SURFACE     = lcd.color565(13, 23, 35);
  COL_CARD        = lcd.color565(22, 34, 49);
  COL_CARD_ALT    = lcd.color565(19, 31, 45);
  COL_BORDER      = lcd.color565(49, 66, 84);
  COL_TEXT        = lcd.color565(245, 247, 250);
  COL_MUTED       = lcd.color565(190, 199, 210);
  COL_BLUE        = lcd.color565(55, 139, 235);
  COL_BLUE_SOFT   = lcd.color565(24, 62, 98);
  COL_GREEN       = lcd.color565(76, 190, 86);
  COL_ORANGE      = lcd.color565(255, 164, 45);
  COL_RED         = lcd.color565(230, 75, 75);
  COL_DARK_BUTTON = lcd.color565(42, 56, 72);

  setupWiFiConnection();
  setupMqttCommunication();
  drawDashboard();

  server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
  server.handleClient();
  ElegantOTA.loop();
  checkWiFiConnection();
  handleMqttMessages();
  handleTouch();
  updateBufferUnloadAnimation();
  updateArticleNameScroll();
  delay(3);
}





//METHODS 
int clampInt(int value, int minValue, int maxValue)
{
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

int maxScrollOffset()
{
  int contentH = rowCount * ROW_STEP - ROW_GAP;
  int maxOffset = contentH - LIST_INNER_H;
  return maxOffset > 0 ? maxOffset : 0;
}

void drawText(
  int x,
  int y,
  const String& text,
  uint16_t color,
  const lgfx::IFont* font,
  float size,
  textdatum_t datum
)
{
  lcd.setFont(font);
  lcd.setTextSize(size);
  lcd.setTextDatum(datum);
  lcd.setTextColor(color);
  lcd.drawString(text, x, y);
  lcd.setTextSize(1.0f);//reset font size
}

void drawTextCentered(
  int x,
  int y,
  int w,
  int h,
  const String& text,
  uint16_t color,
  const lgfx::IFont* font,
  float size
)
{
  drawText(
    x + w / 2,
    y + h / 2,
    text,
    color,
    font,
    size,
    textdatum_t::middle_center
  );
}

// ============================================================
// PICCOLI ELEMENTI GRAFICI
// ============================================================

void drawDropLogo(int cx, int cy)
{
  lcd.drawCircle(cx, cy, 24, COL_TEXT);
  lcd.drawCircle(cx, cy, 23, COL_BORDER);

  lcd.fillTriangle(
    cx,
    cy - 13,
    cx - 8,
    cy + 2,
    cx + 8,
    cy + 2,
    COL_BLUE
  );

  lcd.fillCircle(cx, cy + 5, 8, COL_BLUE);
}

void drawChipCore(int cx, int cy)
{
  // Corpo centrale del chip.
  const int chipW = 24;
  const int chipH = 22;
  const int chipX = cx - chipW / 2;
  const int chipY = cy - chipH / 2;

  // Fondo leggermente più scuro del cerchio, così il chip resta leggibile.
  lcd.fillRoundRect(chipX, chipY, chipW, chipH, 3, COL_DARK_BUTTON);
  lcd.drawRoundRect(chipX, chipY, chipW, chipH, 3, COL_TEXT);

  // Pin a sinistra e a destra.
  for (int i = 0; i < 4; i++)
  {
    int py = chipY + 4 + i * 5;

    lcd.drawFastHLine(chipX - 5, py, 5, COL_TEXT);
    lcd.drawFastHLine(chipX + chipW, py, 5, COL_TEXT);
  }

  // Pin sopra e sotto.
  for (int i = 0; i < 4; i++)
  {
    int px = chipX + 4 + i * 5;

    lcd.drawFastVLine(px, chipY - 5, 5, COL_TEXT);
    lcd.drawFastVLine(px, chipY + chipH, 5, COL_TEXT);
  }

  // Piccoli dettagli interni, senza scritte.
  lcd.fillCircle(cx - 5, cy - 4, 2, COL_TEXT);
  lcd.fillCircle(cx + 5, cy + 4, 2, COL_TEXT);

  lcd.drawLine(cx - 3, cy - 4, cx + 3, cy - 4, COL_TEXT);
  lcd.drawLine(cx + 3, cy - 4, cx + 3, cy + 2, COL_TEXT);
  lcd.drawLine(cx + 3, cy + 2, cx + 5, cy + 2, COL_TEXT);

  lcd.drawLine(cx - 5, cy + 4, cx - 1, cy + 4, COL_TEXT);
  lcd.drawLine(cx - 1, cy + 4, cx - 1, cy, COL_TEXT);
}

void drawChipLogo(int cx, int cy)
{
  // Cerchio blu di sfondo.
  lcd.fillCircle(cx, cy, CHIP_LOGO_RADIUS, COL_BLUE_SOFT);
  lcd.drawCircle(cx, cy, 24, COL_TEXT);
  lcd.drawCircle(cx, cy, 23, COL_BORDER);

  drawChipCore(cx, cy);
}

void drawStatusChip(int x, int y, int w, const char* label, bool ok)
{
  lcd.fillRoundRect(x, y, w, 32, 8, COL_CARD);
  lcd.drawRoundRect(x, y, w, 32, 8, COL_BORDER);

  lcd.fillCircle(x + 14, y + 16, 5, ok ? COL_GREEN : COL_RED);
  drawText(x + 26, y + 8, label, COL_TEXT, &fonts::Font2);
}

void drawRefreshIcon(int cx, int cy, uint16_t color)
{
  lcd.drawCircle(cx, cy, 11, color);
  lcd.fillTriangle(cx + 4, cy - 11, cx + 12, cy - 8, cx + 6, cy - 2, color);
  lcd.fillCircle(cx - 11, cy, 3, COL_DARK_BUTTON);
}

void drawCheckIcon(int cx, int cy, uint16_t color)
{
  lcd.drawCircle(cx, cy, 12, color);
  lcd.drawLine(cx - 6, cy, cx - 1, cy + 5, color);
  lcd.drawLine(cx - 1, cy + 5, cx + 7, cy - 6, color);
}

// ============================================================
// HEADER
// ============================================================

void drawHeader()
{
  lcd.fillRoundRect(HEADER_X, HEADER_Y, HEADER_W, HEADER_H, 12, COL_SURFACE);
  lcd.drawRoundRect(HEADER_X, HEADER_Y, HEADER_W, HEADER_H, 12, COL_BORDER);

  drawChipLogo(CHIP_LOGO_CX, CHIP_LOGO_CY);

  drawText(78, CHIP_LOGO_CY - (CHIP_LOGO_RADIUS/2), "Buffer Lavaggi", COL_TEXT, &fonts::Font4);

  drawStatusChip(302, 25, 74, "WiFi", wifiConnected);
  drawStatusChip(384, 25, 72, "MQTT", mqttConnected);

  drawText(306, 64, wifiConnected ? "ON" : "OFF", wifiConnected ? COL_GREEN : COL_RED, &fonts::Font2);
  drawText(390, 64, mqttConnected ? "ON" : "OFF", mqttConnected ? COL_GREEN : COL_RED, &fonts::Font2);
}

void drawInfoRow()
{
  lcd.fillRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 10, COL_SURFACE);
  lcd.drawRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 10, COL_BORDER);

  drawText(28, 122, String("MAC  ") + macAddress, COL_MUTED, &fonts::Font2);

  lcd.fillRoundRect(MISSION_X, MISSION_Y, MISSION_W, MISSION_H, 8, COL_BLUE_SOFT);
  lcd.drawRoundRect(MISSION_X, MISSION_Y, MISSION_W, MISSION_H, 8, COL_BLUE);

  drawTextCentered(
    MISSION_X,
    MISSION_Y,
    MISSION_W,
    MISSION_H,
    String("Missione ") + bufferMission,
    COL_TEXT,
    &fonts::Font2
  );
}

// ============================================================
// LISTA
// ============================================================

////////////////////////////////////
//DISEGNO LISTA ARTICOLI NEL BUFFER
////////////////////////////////////
int getArticleNameTextWidth(const char* text)
{
  if (text == nullptr || text[0] == '\0') {
    return 0;
  }

  lcd.setFont(&fonts::Font4);
  lcd.setTextSize(1.5f);

  int width = lcd.textWidth(text);

  lcd.setTextSize(1.0f);

  return width;
}


int getArticleNameScrollOffset(int index, int availableWidth)
{
  if (index < 0 || index >= rowCount) {
    return 0;
  }

  int textWidth = getArticleNameTextWidth(articles[index].nome);

  if (textWidth <= availableWidth) {
    return 0;
  }

  int maxOffset = textWidth - availableWidth;

  // Tempo necessario per arrivare fino alla fine del testo,
  // mantenendo una velocità costante indipendente dalla lunghezza.
  unsigned long travelMs =
    ((unsigned long)maxOffset * 1000UL) /
    ARTICLE_NAME_SCROLL_SPEED_PX_S;

  if (travelMs < 1) {
    travelMs = 1;
  }

  unsigned long cycleMs =
    ARTICLE_NAME_START_PAUSE_MS +
    travelMs +
    ARTICLE_NAME_END_PAUSE_MS;

  unsigned long elapsed = millis() - articleNameScrollEpoch;
  unsigned long phase = elapsed % cycleMs;

  // Pausa iniziale: il nome resta leggibile dall'inizio.
  if (phase < ARTICLE_NAME_START_PAUSE_MS) {
    return 0;
  }

  phase -= ARTICLE_NAME_START_PAUSE_MS;

  // Scorrimento da destra verso sinistra.
  if (phase < travelMs)
  {
    int offset =
      (int)((phase * (unsigned long)ARTICLE_NAME_SCROLL_SPEED_PX_S) / 1000UL);

    if (offset > maxOffset) {
      offset = maxOffset;
    }

    return offset;
  }

  // Pausa finale: permette di leggere l'ultima parte del nome.
  return maxOffset;
}


void drawArticleName(int index, int rowY)
{
  if (index < 0 || index >= rowCount) {
    return;
  }

  const Article& article = articles[index];

  uint16_t baseColor = (index == selectedItem)
    ? COL_BLUE_SOFT
    : ((index % 2 == 0) ? COL_CARD : COL_CARD_ALT);

  const int titleX = LIST_INNER_X + ROW_TITLE_OFFSET_X;
  const int titleY = rowY + ROW_TITLE_OFFSET_Y;

  // Limite destro interno della card.
  const int titleRight = LIST_INNER_X + ROW_W - 10;
  const int titleW = titleRight - titleX;

  // Altezza sufficiente per Font4 a 1.5x.
  const int titleH = 48;

  if (titleW <= 0) {
    return;
  }

  // Se il titolo è completamente fuori dal viewport non disegniamo nulla.
  if (titleY + titleH <= LIST_INNER_Y ||
      titleY >= LIST_INNER_Y + LIST_INNER_H)
  {
    return;
  }

  // ------------------------------------------------------------
  // CREA / RIUSA LO SPRITE
  // ------------------------------------------------------------
  if (articleNameSpriteW != titleW || articleNameSpriteH != titleH)
  {
    articleNameSprite.deleteSprite();
    articleNameSprite.setColorDepth(16);

    if (articleNameSprite.createSprite(titleW, titleH) == nullptr)
    {
      articleNameSpriteW = 0;
      articleNameSpriteH = 0;
      return;
    }

    articleNameSpriteW = titleW;
    articleNameSpriteH = titleH;
  }

  int scrollX = getArticleNameScrollOffset(index, titleW);

  // ------------------------------------------------------------
  // COMPONE TUTTO OFF-SCREEN
  // ------------------------------------------------------------
  // Qui non tocchiamo ancora il framebuffer principale:
  // quindi non esiste più il frame intermedio "sfondo vuoto".
  articleNameSprite.fillSprite(baseColor);

  articleNameSprite.setFont(&fonts::Font4);
  articleNameSprite.setTextSize(1.5f);
  articleNameSprite.setTextDatum(textdatum_t::top_left);
  articleNameSprite.setTextColor(COL_TEXT);
  articleNameSprite.setTextWrap(false, false);

  articleNameSprite.drawString(
    article.nome,
    -scrollX,
    0
  );

  // Reset locale dello sprite.
  articleNameSprite.setTextSize(1.0f);

  // ------------------------------------------------------------
  // PUSH UNICO SUL DISPLAY
  // ------------------------------------------------------------
  // Clip solo sulla zona visibile reale della lista.
  int visibleY = max(titleY, LIST_INNER_Y);
  int visibleBottom = min(
    titleY + titleH,
    LIST_INNER_Y + LIST_INNER_H
  );

  int visibleH = visibleBottom - visibleY;

  if (visibleH <= 0) {
    return;
  }

  lcd.setClipRect(
    titleX,
    visibleY,
    titleW,
    visibleH
  );

  // Un'unica operazione verso il display:
  // sfondo + testo arrivano insieme e il flicker sparisce.
  articleNameSprite.pushSprite(titleX, titleY);

  lcd.clearClipRect();
}


void updateArticleNameScroll()
{
  if (missionEditorOpen || espInfoOpen || bufferUnloadOpen) {
    return;
  }

  // Durante un gesto touch/scroll verticale non ridisegniamo il marquee.
  if (touchDown) {
    return;
  }

  unsigned long now = millis();

  if (now - lastArticleNameScrollFrame < ARTICLE_NAME_SCROLL_FRAME_MS) {
    return;
  }

  lastArticleNameScrollFrame = now;

  const int titleX = LIST_INNER_X + ROW_TITLE_OFFSET_X;
  const int titleRight = LIST_INNER_X + ROW_W - 10;
  const int titleW = titleRight - titleX;

  for (int i = 0; i < rowCount; i++)
  {
    int rowY = LIST_INNER_Y + i * ROW_STEP - scrollOffset;
    int rowBottom = rowY + ROW_H;

    if (rowBottom <= LIST_INNER_Y) {
      continue;
    }

    if (rowY >= LIST_INNER_Y + LIST_INNER_H) {
      break;
    }

    int textWidth = getArticleNameTextWidth(articles[i].nome);

    if (textWidth <= titleW) {
      continue;
    }

    int newOffset = getArticleNameScrollOffset(i, titleW);

    // Se il testo non si è spostato neanche di 1 pixel,
    // non facciamo nessun push inutile.
    if (articleNameLastOffset[i] == newOffset) {
      continue;
    }

    articleNameLastOffset[i] = newOffset;
    drawArticleName(i, rowY);
  }
}


void drawListRow(int index, int y)
{
  if (index < 0 || index >= rowCount) return;

  const Article& article = articles[index];
  bool selected = index == selectedItem;

  uint16_t baseColor = selected
    ? COL_BLUE_SOFT
    : ((index % 2 == 0) ? COL_CARD : COL_CARD_ALT);

  uint16_t outlineColor = selected ? COL_BLUE : COL_BORDER;

  lcd.fillRoundRect(LIST_INNER_X, y, ROW_W, ROW_H, 9, baseColor);
  lcd.drawRoundRect(LIST_INNER_X, y, ROW_W, ROW_H, 9, outlineColor);

  // Icona AMR + Cobot
  uint16_t badgeColor = selected ? COL_BLUE : COL_DARK_BUTTON;
  (void)badgeColor;

  lcd.fillRoundRect(
    LIST_INNER_X + ROW_NUMBER_OFFSET_X,
    y + ROW_NUMBER_OFFSET_Y,
    ROW_NUMBER_W,
    ROW_NUMBER_H,
    8,
    COL_BLUE_SOFT
  );

  drawAmrCobotIcon(
    LIST_INNER_X + ROW_NUMBER_OFFSET_X,
    y + ROW_NUMBER_OFFSET_Y + ((ROW_NUMBER_H - 80) / 2),
    80,
    COL_TEXT,
    COL_BLUE_SOFT,
    COL_BLUE_SOFT
  );

  char numberText[4];
  snprintf(numberText, sizeof(numberText), "%02d", index + 1);

  drawTextCentered(
    LIST_INNER_X + 40,
    y + ROW_NUMBER_OFFSET_Y - 10,
    ROW_NUMBER_W,
    ROW_NUMBER_H,
    String(article.linea),
    COL_TEXT,
    &fonts::Font4
  );

  // Riga secondaria.
  drawText(
    LIST_INNER_X + ROW_TEXT_OFFSET_X,
    y + ROW_TEXT_OFFSET_Y,
    article.lotto,
    COL_MUTED,
    &fonts::Font2,
    1.5f
  );

  // Nome articolo per ultimo, perché usa un clip dedicato.
  drawArticleName(index, y);
}

void drawScrollbar()
{
  // Pulisce solo la zona scrollbar
  lcd.fillRect(SCROLLBAR_X - 3, SCROLLBAR_Y - 2, 11, SCROLLBAR_H + 4, COL_SURFACE);

  lcd.fillRoundRect(
    SCROLLBAR_X,
    SCROLLBAR_Y,
    SCROLLBAR_W,
    SCROLLBAR_H,
    2,
    COL_DARK_BUTTON
  );

  int maxScroll = maxScrollOffset();

  if (maxScroll <= 0)
  {
    lcd.fillRoundRect(
      SCROLLBAR_X,
      SCROLLBAR_Y,
      SCROLLBAR_W,
      SCROLLBAR_H,
      2,
      COL_MUTED
    );
    return;
  }

  int contentH = rowCount * ROW_STEP - ROW_GAP;

  int thumbH = (LIST_INNER_H * SCROLLBAR_H) / contentH;
  if (thumbH < 38) thumbH = 38;

  int thumbTravel = SCROLLBAR_H - thumbH;
  int thumbY = SCROLLBAR_Y + (scrollOffset * thumbTravel) / maxScroll;

  lcd.fillRoundRect(
    SCROLLBAR_X,
    thumbY,
    SCROLLBAR_W,
    thumbH,
    2,
    COL_MUTED
  );
}

void drawRowsIntersecting(int clipY, int clipH)
{
  int clipBottom = clipY + clipH;

  for (int i = 0; i < rowCount; i++)
  {
    int rowY = LIST_INNER_Y + i * ROW_STEP - scrollOffset;
    int rowBottom = rowY + ROW_H;

    if (rowBottom < clipY) continue;
    if (rowY > clipBottom) break;

    // drawArticleName() usa un clip proprio e poi lo rimuove.
    // Per questo il clip della lista viene reimpostato prima di ogni riga.
    lcd.setClipRect(
      LIST_INNER_X,
      clipY,
      LIST_INNER_W,
      clipH
    );

    drawListRow(i, rowY);
  }

  lcd.clearClipRect();
}

void drawListFull()
{
  for (int i = 0; i < MAX_ROWS; i++) {
    articleNameLastOffset[i] = -1;
  }

  lcd.fillRoundRect(LIST_X, LIST_Y, LIST_W, UI_LIST_H, 12, COL_SURFACE);
  lcd.drawRoundRect(LIST_X, LIST_Y, LIST_W, UI_LIST_H, 12, COL_BORDER);

  lcd.fillRect(LIST_INNER_X, LIST_INNER_Y, LIST_INNER_W, LIST_INNER_H, COL_SURFACE);

  drawRowsIntersecting(LIST_INNER_Y, LIST_INNER_H);
  drawScrollbar();
}

// Scroll ottimizzato: sposta i pixel già presenti con copyRect()
// e ridisegna solo la striscia appena comparsa.
void setScrollOffsetFast(int requestedOffset)
{
  int newOffset = clampInt(requestedOffset, 0, maxScrollOffset());
  int delta = newOffset - scrollOffset;

  if (delta == 0) return;

  // Dopo un movimento verticale i marquee ripartono dall'inizio:
  // è più naturale leggere il nome appena compare una nuova riga.
  articleNameScrollEpoch = millis();
  for (int i = 0; i < MAX_ROWS; i++) {
    articleNameLastOffset[i] = -1;
  }

  if (abs(delta) >= LIST_INNER_H)
  {
    scrollOffset = newOffset;
    drawListFull();
    return;
  }

  // Usa la funzione di scroll nativa di LovyanGFX.
  // In questo modo il calcolo delle coordinate con setRotation(3)
  // resta interamente dentro alla libreria.
  lcd.setScrollRect(
    LIST_INNER_X,
    LIST_INNER_Y,
    LIST_INNER_W,
    LIST_INNER_H,
    COL_SURFACE
  );

  scrollOffset = newOffset;

  if (delta > 0)
  {
    // Contenuto verso l'alto.
    lcd.scroll(0, -delta);

    int exposedY = LIST_INNER_Y + LIST_INNER_H - delta;
    drawRowsIntersecting(exposedY, delta);
  }
  else
  {
    // Contenuto verso il basso.
    int amount = -delta;
    lcd.scroll(0, amount);

    drawRowsIntersecting(LIST_INNER_Y, amount);
  }

  lcd.clearScrollRect();

  drawScrollbar();
  lcd.drawRoundRect(LIST_X, LIST_Y, LIST_W, UI_LIST_H, 12, COL_BORDER);
}

// ============================================================
// BOTTONI / FOOTER
// ============================================================

void drawButtons()
{
  lcd.fillRoundRect(
    BUTTON_LEFT_X,
    BUTTON_Y,
    BUTTON_LEFT_W,
    BUTTON_H,
    10,
    COL_DARK_BUTTON
  );
  lcd.drawRoundRect(
    BUTTON_LEFT_X,
    BUTTON_Y,
    BUTTON_LEFT_W,
    BUTTON_H,
    10,
    COL_BORDER
  );

  drawRefreshIcon(BUTTON_LEFT_X + 31, BUTTON_Y + 31, COL_TEXT);
  drawText(BUTTON_LEFT_X + 55, BUTTON_Y + 21, "Aggiorna", COL_TEXT, &fonts::Font2, 1.5f);

  lcd.fillRoundRect(
    BUTTON_RIGHT_X,
    BUTTON_Y,
    BUTTON_RIGHT_W,
    BUTTON_H,
    10,
    COL_GREEN
  );

  drawCheckIcon(BUTTON_RIGHT_X + 31, BUTTON_Y + 31, COL_TEXT);
  drawText(BUTTON_RIGHT_X + 56, BUTTON_Y + 21, "Conferma", COL_TEXT, &fonts::Font2, 1.5f);
}

// ============================================================
// DASHBOARD COMPLETA
// ============================================================

void drawDashboard()
{
  lcd.fillScreen(COL_BG);

  drawHeader();
  drawInfoRow();
  drawListFull();
  drawButtons();
}

// ============================================================
// HIT TEST
// ============================================================

bool pointInRect(int x, int y, int rx, int ry, int rw, int rh)
{
  return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}


void drawHeaderLongPressProgress()
{
  unsigned long elapsed = millis() - headerLongPressStart;
  if (elapsed > HEADER_LONG_PRESS_MS) elapsed = HEADER_LONG_PRESS_MS;

  int progress = (int)((elapsed * 100UL) / HEADER_LONG_PRESS_MS);

  // Aggiorna solo quando cambia almeno dell'1%.
  if (progress == headerPressLastProgress) return;
  headerPressLastProgress = progress;

  const int r = CHIP_LOGO_RADIUS;

  // Sfondo originale blu.
  lcd.fillCircle(
    CHIP_LOGO_CX,
    CHIP_LOGO_CY,
    r,
    COL_BLUE_SOFT
  );

  // Il giallo/arancio cresce dal CENTRO verso l'esterno.
  // 0%   -> raggio 0
  // 50%  -> metà raggio
  // 100% -> cerchio completamente giallo
  int fillRadius = (r * progress) / 100;

  if (fillRadius > 0)
  {
    lcd.fillCircle(
      CHIP_LOGO_CX,
      CHIP_LOGO_CY,
      fillRadius,
      COL_ORANGE
    );
  }

  // Ridisegna bordo e icona chip sopra al riempimento.
  lcd.drawCircle(CHIP_LOGO_CX, CHIP_LOGO_CY, 24, COL_TEXT);
  lcd.drawCircle(CHIP_LOGO_CX, CHIP_LOGO_CY, 23, COL_BORDER);
  drawChipCore(CHIP_LOGO_CX, CHIP_LOGO_CY);
}

void drawEspInfoLine(
  int y,
  const String& label,
  const String& value,
  uint16_t valueColor
)
{
  drawText(
    ESP_INFO_X + 24,
    y,
    label,
    COL_MUTED,
    &fonts::Font2,
    1.0f
  );

  drawText(
    ESP_INFO_X + 138,
    y,
    value,
    valueColor,
    &fonts::Font2,
    1.0f
  );
}

void drawEspInfoScreen()
{
  lcd.fillScreen(COL_BG);

  lcd.fillRoundRect(
    ESP_INFO_X,
    ESP_INFO_Y,
    ESP_INFO_W,
    ESP_INFO_H,
    16,
    COL_SURFACE
  );

  lcd.drawRoundRect(
    ESP_INFO_X,
    ESP_INFO_Y,
    ESP_INFO_W,
    ESP_INFO_H,
    16,
    COL_BORDER
  );

  drawTextCentered(
    ESP_INFO_X,
    ESP_INFO_Y + 18,
    ESP_INFO_W,
    38,
    "Informazioni ESP32",
    COL_TEXT,
    &fonts::Font4,
    1.0f
  );

  // Separatore.
  lcd.drawFastHLine(
    ESP_INFO_X + 20,
    ESP_INFO_Y + 70,
    ESP_INFO_W - 40,
    COL_BORDER
  );

  int y = ESP_INFO_Y + 92;
  const int step = 38;

  drawEspInfoLine(y, "MAC", macAddress.length() > 0 ? macAddress : WiFi.macAddress());
  y += step;

  String localIp = WiFi.status() == WL_CONNECTED
    ? WiFi.localIP().toString()
    : String("Non connesso");

  drawEspInfoLine(
    y,
    "IP",
    localIp,
    WiFi.status() == WL_CONNECTED ? COL_GREEN : COL_RED
  );
  y += step;

  drawEspInfoLine(y, "Broker", String(broker));
  y += step;

  drawEspInfoLine(y, "Porta", String(port));
  y += step + 8;

  drawText(
    ESP_INFO_X + 24,
    y,
    "Topic MQTT",
    COL_TEXT,
    &fonts::Font2,
    1.2f
  );

  y += 34;

  drawEspInfoLine(y, "Notify", String(notifyTopic));
  y += step;

  drawEspInfoLine(y, "Select", String(selectTopic));
  y += step;

  drawEspInfoLine(y, "Lista", String(readListTopic));
  y += step + 8;

  drawEspInfoLine(
    y,
    "WiFi",
    wifiConnected ? String("Connesso") : String("Disconnesso"),
    wifiConnected ? COL_GREEN : COL_RED
  );
  y += step;

  drawEspInfoLine(
    y,
    "MQTT",
    mqttConnected ? String("Connesso") : String("Disconnesso"),
    mqttConnected ? COL_GREEN : COL_RED
  );

  // Bottone di chiusura.
  lcd.fillRoundRect(
    ESP_INFO_CLOSE_X,
    ESP_INFO_CLOSE_Y,
    ESP_INFO_CLOSE_W,
    ESP_INFO_CLOSE_H,
    10,
    COL_DARK_BUTTON
  );

  lcd.drawRoundRect(
    ESP_INFO_CLOSE_X,
    ESP_INFO_CLOSE_Y,
    ESP_INFO_CLOSE_W,
    ESP_INFO_CLOSE_H,
    10,
    COL_BORDER
  );

  drawTextCentered(
    ESP_INFO_CLOSE_X,
    ESP_INFO_CLOSE_Y,
    ESP_INFO_CLOSE_W,
    ESP_INFO_CLOSE_H,
    "Chiudi",
    COL_TEXT,
    &fonts::Font2,
    1.4f
  );
}

void openEspInfoScreen()
{
  espInfoOpen = true;
  espInfoTouchDown = false;

  // Il dito è ancora appoggiato dopo i 5 secondi.
  espInfoIgnoreUntilRelease = true;

  headerLongPressCandidate = false;
  headerPressLastProgress = -1;

  missionLongPressCandidate = false;
  missionLongPressLastProgress = -1;

  touchDown = false;
  touchMoved = false;

  drawEspInfoScreen();
}

void closeEspInfoScreen()
{
  espInfoOpen = false;
  espInfoTouchDown = false;
  espInfoIgnoreUntilRelease = false;

  drawDashboard();
}

void handleEspInfoTouch(bool pressed, int x, int y)
{
  if (espInfoIgnoreUntilRelease)
  {
    if (!pressed) {
      espInfoIgnoreUntilRelease = false;
    }
    return;
  }

  if (pressed)
  {
    if (!espInfoTouchDown)
    {
      espInfoTouchDown = true;
      espInfoTouchStartX = x;
      espInfoTouchStartY = y;
    }

    espInfoLastTouchX = x;
    espInfoLastTouchY = y;
    return;
  }

  if (espInfoTouchDown)
  {
    espInfoTouchDown = false;

    int dx = espInfoLastTouchX - espInfoTouchStartX;
    int dy = espInfoLastTouchY - espInfoTouchStartY;

    if (abs(dx) <= TAP_MOVE_TOLERANCE &&
        abs(dy) <= TAP_MOVE_TOLERANCE &&
        pointInRect(
          espInfoLastTouchX,
          espInfoLastTouchY,
          ESP_INFO_CLOSE_X,
          ESP_INFO_CLOSE_Y,
          ESP_INFO_CLOSE_W,
          ESP_INFO_CLOSE_H
        ))
    {
      closeEspInfoScreen();
    }
  }
}

void drawMissionLongPressProgress()
{
  unsigned long elapsed = millis() - missionLongPressStart;
  if (elapsed > MISSION_LONG_PRESS_MS) elapsed = MISSION_LONG_PRESS_MS;

  int progress = (int)((elapsed * 100UL) / MISSION_LONG_PRESS_MS);

  // Aggiorna solo quando cambia almeno dell'1%.
  if (progress == missionLongPressLastProgress) return;
  missionLongPressLastProgress = progress;

  int barX = MISSION_X + 7;
  int barY = MISSION_Y + MISSION_H - 5;
  int barW = MISSION_W - 14;
  int fillW = (barW * progress) / 100;

  lcd.fillRect(barX, barY, barW, 3, COL_BLUE_SOFT);

  if (fillW > 0) {
    lcd.fillRect(barX, barY, fillW, 3, COL_ORANGE);
  }
}

void drawMissionEditorValue()
{
  lcd.fillRoundRect(
    EDITOR_DISPLAY_X,
    EDITOR_DISPLAY_Y,
    EDITOR_DISPLAY_W,
    EDITOR_DISPLAY_H,
    10,
    COL_CARD
  );

  lcd.drawRoundRect(
    EDITOR_DISPLAY_X,
    EDITOR_DISPLAY_Y,
    EDITOR_DISPLAY_W,
    EDITOR_DISPLAY_H,
    10,
    COL_BLUE
  );

  String valueToDraw = missionEditorValue.length() > 0
    ? missionEditorValue
    : "_";

  drawTextCentered(
    EDITOR_DISPLAY_X,
    EDITOR_DISPLAY_Y,
    EDITOR_DISPLAY_W,
    EDITOR_DISPLAY_H,
    valueToDraw,
    COL_TEXT,
    &fonts::Font4,
    1.4f
  );

  // Pulisce / aggiorna il messaggio di stato sotto il campo.
  lcd.fillRect(EDITOR_DISPLAY_X, 304, EDITOR_DISPLAY_W, 14, COL_SURFACE);

  if (missionEditorMessage.length() > 0) {
    drawTextCentered(
      EDITOR_DISPLAY_X,
      303,
      EDITOR_DISPLAY_W,
      16,
      missionEditorMessage,
      COL_RED,
      &fonts::Font2,
      1.0f
    );
  }
}

void drawMissionEditorKey(int col, int row, const String& label, uint16_t fillColor)
{
  int x = KEYPAD_X + col * (KEY_W + KEY_GAP_X);
  int y = KEYPAD_Y + row * (KEY_H + KEY_GAP_Y);

  lcd.fillRoundRect(x, y, KEY_W, KEY_H, 10, fillColor);
  lcd.drawRoundRect(x, y, KEY_W, KEY_H, 10, COL_BORDER);

  drawTextCentered(
    x,
    y,
    KEY_W,
    KEY_H,
    label,
    COL_TEXT,
    &fonts::Font4,
    1.1f
  );
}

void drawMissionEditor()
{
  // Pagina modal: nasconde la dashboard finché l'editor è aperto.
  lcd.fillScreen(COL_BG);

  lcd.fillRoundRect(
    EDITOR_X,
    EDITOR_Y,
    EDITOR_W,
    EDITOR_H,
    16,
    COL_SURFACE
  );

  lcd.drawRoundRect(
    EDITOR_X,
    EDITOR_Y,
    EDITOR_W,
    EDITOR_H,
    16,
    COL_BORDER
  );

  drawTextCentered(
    EDITOR_X,
    EDITOR_Y + 18,
    EDITOR_W,
    34,
    "Modifica missione",
    COL_TEXT,
    &fonts::Font4,
    1.0f
  );

  drawMissionEditorValue();

  // Tastierino numerico.
  drawMissionEditorKey(0, 0, "1", COL_DARK_BUTTON);
  drawMissionEditorKey(1, 0, "2", COL_DARK_BUTTON);
  drawMissionEditorKey(2, 0, "3", COL_DARK_BUTTON);

  drawMissionEditorKey(0, 1, "4", COL_DARK_BUTTON);
  drawMissionEditorKey(1, 1, "5", COL_DARK_BUTTON);
  drawMissionEditorKey(2, 1, "6", COL_DARK_BUTTON);

  drawMissionEditorKey(0, 2, "7", COL_DARK_BUTTON);
  drawMissionEditorKey(1, 2, "8", COL_DARK_BUTTON);
  drawMissionEditorKey(2, 2, "9", COL_DARK_BUTTON);

  drawMissionEditorKey(0, 3, "C", COL_CARD_ALT);
  drawMissionEditorKey(1, 3, "0", COL_DARK_BUTTON);
  drawMissionEditorKey(2, 3, "<", COL_CARD_ALT);

  int cancelX = KEYPAD_X;
  int saveX = cancelX + EDITOR_ACTION_W + EDITOR_ACTION_GAP;

  lcd.fillRoundRect(
    cancelX,
    EDITOR_ACTION_Y,
    EDITOR_ACTION_W,
    EDITOR_ACTION_H,
    10,
    COL_DARK_BUTTON
  );
  lcd.drawRoundRect(
    cancelX,
    EDITOR_ACTION_Y,
    EDITOR_ACTION_W,
    EDITOR_ACTION_H,
    10,
    COL_BORDER
  );

  drawTextCentered(
    cancelX,
    EDITOR_ACTION_Y,
    EDITOR_ACTION_W,
    EDITOR_ACTION_H,
    "Annulla",
    COL_TEXT,
    &fonts::Font2,
    1.4f
  );

  lcd.fillRoundRect(
    saveX,
    EDITOR_ACTION_Y,
    EDITOR_ACTION_W,
    EDITOR_ACTION_H,
    10,
    COL_GREEN
  );

  drawTextCentered(
    saveX,
    EDITOR_ACTION_Y,
    EDITOR_ACTION_W,
    EDITOR_ACTION_H,
    "Salva",
    COL_TEXT,
    &fonts::Font2,
    1.4f
  );
}

void openMissionEditor()
{
  missionEditorOpen = true;
  missionEditorValue = String(bufferMission);
  missionEditorMessage = "";

  missionEditorTouchDown = false;

  // Il dito è ancora appoggiato dopo il long press:
  // non vogliamo che venga interpretato come pressione sul tastierino.
  missionEditorIgnoreUntilRelease = true;

  missionLongPressCandidate = false;
  missionLongPressLastProgress = -1;

  touchDown = false;
  touchMoved = false;

  drawMissionEditor();
}

void closeMissionEditor()
{
  missionEditorOpen = false;
  missionEditorTouchDown = false;
  missionEditorIgnoreUntilRelease = false;
  missionEditorMessage = "";

  // Ricostruisce la dashboard completa.
  drawDashboard();
}

void appendMissionDigit(char digit)
{
  if (missionEditorValue.length() >= MISSION_MAX_DIGITS) {
    missionEditorMessage = "Massimo 9 cifre";
    drawMissionEditorValue();
    return;
  }

  // Evita zeri iniziali inutili.
  if (missionEditorValue == "0") {
    missionEditorValue = "";
  }

  missionEditorValue += digit;
  missionEditorMessage = "";
  drawMissionEditorValue();
}

void handleMissionEditorTap(int x, int y)
{
  // Tastierino 3x4.
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 3; col++) {
      int keyX = KEYPAD_X + col * (KEY_W + KEY_GAP_X);
      int keyY = KEYPAD_Y + row * (KEY_H + KEY_GAP_Y);

      if (!pointInRect(x, y, keyX, keyY, KEY_W, KEY_H)) {
        continue;
      }

      static const char* labels[4][3] = {
        {"1", "2", "3"},
        {"4", "5", "6"},
        {"7", "8", "9"},
        {"C", "0", "<"}
      };

      const char* label = labels[row][col];

      if (label[0] >= '0' && label[0] <= '9' && label[1] == '\0') {
        appendMissionDigit(label[0]);
        return;
      }

      if (label[0] == 'C') {
        missionEditorValue = "";
        missionEditorMessage = "";
        drawMissionEditorValue();
        return;
      }

      if (label[0] == '<') {
        if (missionEditorValue.length() > 0) {
          missionEditorValue.remove(missionEditorValue.length() - 1);
        }
        missionEditorMessage = "";
        drawMissionEditorValue();
        return;
      }
    }
  }

  int cancelX = KEYPAD_X;
  int saveX = cancelX + EDITOR_ACTION_W + EDITOR_ACTION_GAP;

  if (pointInRect(
        x,
        y,
        cancelX,
        EDITOR_ACTION_Y,
        EDITOR_ACTION_W,
        EDITOR_ACTION_H
      ))
  {
    closeMissionEditor();
    return;
  }

  if (pointInRect(
        x,
        y,
        saveX,
        EDITOR_ACTION_Y,
        EDITOR_ACTION_W,
        EDITOR_ACTION_H
      ))
  {
    if (missionEditorValue.length() == 0) {
      missionEditorMessage = "Inserisci almeno una cifra";
      drawMissionEditorValue();
      return;
    }

    // 9 cifre positive stanno comodamente dentro un int32.
    bufferMission = missionEditorValue.toInt();
    closeMissionEditor();
    return;
  }
}

void handleMissionEditorTouch(bool pressed, int x, int y)
{
  if (missionEditorIgnoreUntilRelease) {
    if (!pressed) {
      missionEditorIgnoreUntilRelease = false;
    }
    return;
  }

  if (pressed) {
    if (!missionEditorTouchDown) {
      missionEditorTouchDown = true;
      missionEditorTouchStartX = x;
      missionEditorTouchStartY = y;
    }

    missionEditorLastTouchX = x;
    missionEditorLastTouchY = y;
    return;
  }

  if (missionEditorTouchDown) {
    missionEditorTouchDown = false;

    int dx = missionEditorLastTouchX - missionEditorTouchStartX;
    int dy = missionEditorLastTouchY - missionEditorTouchStartY;

    if (abs(dx) <= TAP_MOVE_TOLERANCE &&
        abs(dy) <= TAP_MOVE_TOLERANCE)
    {
      handleMissionEditorTap(
        missionEditorLastTouchX,
        missionEditorLastTouchY
      );
    }
  }
}


// ============================================================
// FINESTRA SCARICAMENTO BUFFER AMR
// ============================================================

void drawBufferUnloadLoadingBar()
{
  if (!bufferUnloadOpen) {
    return;
  }

  if (!bufferUnloadBarSpriteReady)
  {
    bufferUnloadBarSprite.setColorDepth(16);

    if (bufferUnloadBarSprite.createSprite(
          BUFFER_UNLOAD_BAR_W,
          BUFFER_UNLOAD_BAR_H
        ) == nullptr)
    {
      bufferUnloadBarSpriteReady = false;
      return;
    }

    bufferUnloadBarSpriteReady = true;
  }

  // Tutta la barra viene composta off-screen.
  bufferUnloadBarSprite.fillSprite(COL_SURFACE);

  bufferUnloadBarSprite.fillRoundRect(
    0,
    0,
    BUFFER_UNLOAD_BAR_W,
    BUFFER_UNLOAD_BAR_H,
    BUFFER_UNLOAD_BAR_H / 2,
    COL_DARK_BUTTON
  );

  bufferUnloadBarSprite.drawRoundRect(
    0,
    0,
    BUFFER_UNLOAD_BAR_W,
    BUFFER_UNLOAD_BAR_H,
    BUFFER_UNLOAD_BAR_H / 2,
    COL_BORDER
  );

  const int padding = 4;
  const int innerX = padding;
  const int innerY = padding;
  const int innerW = BUFFER_UNLOAD_BAR_W - padding * 2;
  const int innerH = BUFFER_UNLOAD_BAR_H - padding * 2;

  // Segmento mobile = circa un terzo della barra.
  const int segmentW = innerW * 32 / 100;
  const int travelW = innerW - segmentW;

  unsigned long elapsed = millis() - bufferUnloadAnimationStart;
  unsigned long phase = elapsed % BUFFER_UNLOAD_ANIM_PERIOD_MS;
  unsigned long halfPeriod = BUFFER_UNLOAD_ANIM_PERIOD_MS / 2;

  int segmentX = 0;

  if (phase < halfPeriod)
  {
    // Sinistra -> destra.
    segmentX = (int)(
      (phase * (unsigned long)travelW) / halfPeriod
    );
  }
  else
  {
    // Destra -> sinistra.
    unsigned long backPhase = phase - halfPeriod;

    segmentX = travelW - (int)(
      (backPhase * (unsigned long)travelW) / halfPeriod
    );
  }

  bufferUnloadBarSprite.fillRoundRect(
    innerX + segmentX,
    innerY,
    segmentW,
    innerH,
    innerH / 2,
    COL_BLUE
  );

  // Un solo push sul display per evitare flicker.
  bufferUnloadBarSprite.pushSprite(
    BUFFER_UNLOAD_BAR_X,
    BUFFER_UNLOAD_BAR_Y
  );
}


void drawBufferUnloadWindow()
{
  // ============================================================
  // PAGINA DI SCARICAMENTO
  // ============================================================
  // Qui NON lasciamo sotto Missione, lista e bottoni:
  // puliamo tutto lo schermo e manteniamo soltanto l'header.
  lcd.fillScreen(COL_BG);
  drawHeader();

  // Grande area dedicata allo scaricamento, sotto l'header.
  lcd.fillRoundRect(
    BUFFER_UNLOAD_X,
    BUFFER_UNLOAD_Y,
    BUFFER_UNLOAD_W,
    BUFFER_UNLOAD_H,
    18,
    COL_SURFACE
  );

  lcd.drawRoundRect(
    BUFFER_UNLOAD_X,
    BUFFER_UNLOAD_Y,
    BUFFER_UNLOAD_W,
    BUFFER_UNLOAD_H,
    18,
    COL_BORDER
  );

  drawTextCentered(
    BUFFER_UNLOAD_X,
    BUFFER_UNLOAD_Y + 24,
    BUFFER_UNLOAD_W,
    42,
    "Scaricamento buffer",
    COL_TEXT,
    &fonts::Font4,
    1.0f
  );

  drawTextCentered(
    BUFFER_UNLOAD_X + 28,
    BUFFER_UNLOAD_Y + 72,
    BUFFER_UNLOAD_W - 56,
    26,
    "AMR in scaricamento...",
    COL_MUTED,
    &fonts::Font2,
    1.25f
  );

  // Riutilizza la tua icona AMR + Cobot attuale.
  drawAmrCobotIcon(
    BUFFER_UNLOAD_LOGO_X,
    BUFFER_UNLOAD_LOGO_Y,
    BUFFER_UNLOAD_LOGO_SIZE,
    COL_TEXT,
    COL_BLUE_SOFT,
    COL_BLUE_SOFT
  );

  drawTextCentered(
    BUFFER_UNLOAD_X + 30,
    BUFFER_UNLOAD_BAR_Y - 47,
    BUFFER_UNLOAD_W - 60,
    28,
    "Scaricamento in corso",
    COL_TEXT,
    &fonts::Font2,
    1.2f
  );

  drawBufferUnloadLoadingBar();

  // Pulsante Completato.
  lcd.fillRoundRect(
    BUFFER_UNLOAD_BUTTON_X,
    BUFFER_UNLOAD_BUTTON_Y,
    BUFFER_UNLOAD_BUTTON_W,
    BUFFER_UNLOAD_BUTTON_H,
    12,
    COL_GREEN
  );

  lcd.drawRoundRect(
    BUFFER_UNLOAD_BUTTON_X,
    BUFFER_UNLOAD_BUTTON_Y,
    BUFFER_UNLOAD_BUTTON_W,
    BUFFER_UNLOAD_BUTTON_H,
    12,
    COL_BORDER
  );

  drawCheckIcon(
    BUFFER_UNLOAD_BUTTON_X + 38,
    BUFFER_UNLOAD_BUTTON_Y + BUFFER_UNLOAD_BUTTON_H / 2,
    COL_TEXT
  );

  drawTextCentered(
    BUFFER_UNLOAD_BUTTON_X + 52,
    BUFFER_UNLOAD_BUTTON_Y,
    BUFFER_UNLOAD_BUTTON_W - 64,
    BUFFER_UNLOAD_BUTTON_H,
    "Completato",
    COL_TEXT,
    &fonts::Font4,
    1.0f
  );
}


void openBufferUnloadWindow()
{
  bufferUnloadOpen = true;
  bufferUnloadTouchDown = false;

  bufferUnloadAnimationStart = millis();
  bufferUnloadLastFrame = 0;

  // Ferma eventuali gesture/long press rimasti attivi.
  touchDown = false;
  touchMoved = false;

  missionLongPressCandidate = false;
  missionLongPressLastProgress = -1;

  headerLongPressCandidate = false;
  headerPressLastProgress = -1;

  drawBufferUnloadWindow();
}


void closeBufferUnloadWindow()
{
  bufferUnloadOpen = false;
  bufferUnloadTouchDown = false;

  // Quando torni alla dashboard il marquee riparte dall'inizio.
  articleNameScrollEpoch = millis();

  for (int i = 0; i < MAX_ROWS; i++) {
    articleNameLastOffset[i] = -1;
  }

  drawDashboard();
}


void updateBufferUnloadAnimation()
{
  if (!bufferUnloadOpen) {
    return;
  }

  unsigned long now = millis();

  if (now - bufferUnloadLastFrame < BUFFER_UNLOAD_ANIM_FRAME_MS) {
    return;
  }

  bufferUnloadLastFrame = now;
  drawBufferUnloadLoadingBar();
}


void handleBufferUnloadTouch(bool pressed, int x, int y)
{
  if (pressed)
  {
    if (!bufferUnloadTouchDown)
    {
      bufferUnloadTouchDown = true;
      bufferUnloadTouchStartX = x;
      bufferUnloadTouchStartY = y;
    }

    bufferUnloadLastTouchX = x;
    bufferUnloadLastTouchY = y;
    return;
  }

  if (bufferUnloadTouchDown)
  {
    bufferUnloadTouchDown = false;

    int dx = bufferUnloadLastTouchX - bufferUnloadTouchStartX;
    int dy = bufferUnloadLastTouchY - bufferUnloadTouchStartY;

    if (abs(dx) <= TAP_MOVE_TOLERANCE &&
        abs(dy) <= TAP_MOVE_TOLERANCE &&
        pointInRect(
          bufferUnloadLastTouchX,
          bufferUnloadLastTouchY,
          BUFFER_UNLOAD_BUTTON_X,
          BUFFER_UNLOAD_BUTTON_Y,
          BUFFER_UNLOAD_BUTTON_W,
          BUFFER_UNLOAD_BUTTON_H
        ))
    {
      closeBufferUnloadWindow();
    }
  }
}


int itemAtPoint(int x, int y)
{
  if (!pointInRect(x, y, LIST_INNER_X, LIST_INNER_Y, LIST_INNER_W, LIST_INNER_H))
  {
    return -1;
  }

  int contentY = (y - LIST_INNER_Y) + scrollOffset;
  int index = contentY / ROW_STEP;
  int insideRowY = contentY % ROW_STEP;

  if (index < 0 || index >= rowCount) return -1;
  if (insideRowY >= ROW_H) return -1;

  return index;
}

// ============================================================
// TOUCH
// ============================================================

void handleTap(int x, int y)
{
  // Lista
  int index = itemAtPoint(x, y);

  if (index >= 0)
  {
    int oldSelected = selectedItem;
    selectedItem = index;

    if (oldSelected != selectedItem)
    {
      drawListFull();
    }

    return;
  }

  // Aggiorna
  if (pointInRect(x, y, BUTTON_LEFT_X, BUTTON_Y, BUTTON_LEFT_W, BUTTON_H))
  {
    publishNotifyTopic();
    return;
  }

  // Conferma
  if (pointInRect(x, y, BUTTON_RIGHT_X, BUTTON_Y, BUTTON_RIGHT_W, BUTTON_H))
  {
    if (selectedItem >= 0 && selectedItem < rowCount)
    {
      // Mantiene SEMPRE il comportamento funzionale originale:
      // prima invia la richiesta MQTT relativa all'articolo selezionato.
      publishArticleSelected(selectedItem);

      // Solo dopo il publish passa alla pagina di scaricamento AMR.
      openBufferUnloadWindow();
    }

    return;
  }
}

void handleTouch()
{
  uint16_t x = 0;
  uint16_t y = 0;
  bool pressed = lcd.getTouch(&x, &y);

  // Quando una finestra modal è aperta, il touch viene gestito solo da quella.
  if (bufferUnloadOpen) {
    handleBufferUnloadTouch(pressed, x, y);
    return;
  }

  if (espInfoOpen) {
    handleEspInfoTouch(pressed, x, y);
    return;
  }

  if (missionEditorOpen) {
    handleMissionEditorTouch(pressed, x, y);
    return;
  }

  if (pressed)
  {
    if (!touchDown)
    {
      touchDown = true;
      touchMoved = false;

      touchStartX = x;
      touchStartY = y;
      lastTouchX = x;
      lastTouchY = y;

      // Il long press viene armato solo se il tocco parte
      // all'interno del riquadro Missione.
      missionLongPressCandidate = pointInRect(
        x,
        y,
        MISSION_X,
        MISSION_Y,
        MISSION_W,
        MISSION_H
      );

      if (missionLongPressCandidate) {
        missionLongPressStart = millis();
        missionLongPressLastProgress = -1;
      }

      // Long press informazioni: valido da qualunque punto dell'header.
      headerLongPressCandidate = pointInRect(
        x,
        y,
        HEADER_X,
        HEADER_Y,
        HEADER_W,
        HEADER_H
      );

      if (headerLongPressCandidate) {
        headerLongPressStart = millis();
        headerPressLastProgress = -1;
      }

      return;
    }

    lastTouchX = x;

    int totalDx = (int)x - touchStartX;
    int totalDy = (int)y - touchStartY;

    if (abs(totalDx) > TAP_MOVE_TOLERANCE ||
        abs(totalDy) > TAP_MOVE_TOLERANCE)
    {
      touchMoved = true;
    }

    // ==========================================================
    // LONG PRESS MISSIONE
    // ==========================================================
    if (missionLongPressCandidate)
    {
      bool movedTooMuch =
        abs(totalDx) > MISSION_LONG_PRESS_MOVE_TOLERANCE ||
        abs(totalDy) > MISSION_LONG_PRESS_MOVE_TOLERANCE;

      bool stillNearMission = pointInRect(
        x,
        y,
        MISSION_X - 10,
        MISSION_Y - 10,
        MISSION_W + 20,
        MISSION_H + 20
      );

      if (movedTooMuch || !stillNearMission)
      {
        missionLongPressCandidate = false;
        missionLongPressLastProgress = -1;
        drawInfoRow();
      }
      else
      {
        drawMissionLongPressProgress();

        if (millis() - missionLongPressStart >= MISSION_LONG_PRESS_MS)
        {
          openMissionEditor();
          return;
        }
      }
    }

    // ==========================================================
    // LONG PRESS HEADER - 5 secondi
    // ==========================================================
    if (headerLongPressCandidate)
    {
      bool movedTooMuch =
        abs(totalDx) > HEADER_LONG_PRESS_MOVE_TOLERANCE ||
        abs(totalDy) > HEADER_LONG_PRESS_MOVE_TOLERANCE;

      bool stillNearHeader = pointInRect(
        x,
        y,
        HEADER_X - 10,
        HEADER_Y - 10,
        HEADER_W + 20,
        HEADER_H + 20
      );

      if (movedTooMuch || !stillNearHeader)
      {
        headerLongPressCandidate = false;
        headerPressLastProgress = -1;
        drawHeader();
      }
      else
      {
        drawHeaderLongPressProgress();

        if (millis() - headerLongPressStart >= HEADER_LONG_PRESS_MS)
        {
          openEspInfoScreen();
          return;
        }
      }
    }

    // Scroll solo se il gesto è partito nella lista.
    if (
      pointInRect(
        touchStartX,
        touchStartY,
        LIST_INNER_X,
        LIST_INNER_Y,
        LIST_INNER_W,
        LIST_INNER_H
      ) &&
      touchMoved &&
      millis() - lastScrollFrame >= SCROLL_FRAME_MS
    )
    {
      lastScrollFrame = millis();

      int fingerDy = (int)y - lastTouchY;

      if (abs(fingerDy) >= 2)
      {
        // Dito in alto -> contenuto sale -> offset aumenta.
        setScrollOffsetFast(scrollOffset - fingerDy);
        lastTouchY = y;
      }
    }

    return;
  }

  // Rilascio
  if (touchDown)
  {
    touchDown = false;

    // Se il long press Missione non ha raggiunto la soglia,
    // ripristina graficamente il riquadro Missione.
    if (missionLongPressCandidate)
    {
      missionLongPressCandidate = false;
      missionLongPressLastProgress = -1;
      drawInfoRow();
    }

    if (headerLongPressCandidate)
    {
      headerLongPressCandidate = false;
      headerPressLastProgress = -1;
      drawHeader();
    }

    if (!touchMoved)
    {
      handleTap(lastTouchX, lastTouchY);
    }

    touchMoved = false;
  }
}

//Connessione wifi
void setupWiFiConnection(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  macAddress = WiFi.macAddress();
  Serial.print(macAddress);
  wifiConnected = true;
  if (!missionEditorOpen && !espInfoOpen) drawHeader();
}

//Connessione wifi
void checkWiFiConnection(){
  if (WiFi.status() != WL_CONNECTED) {
    //wifi non connesso
    if(wifiConnected){
      wifiConnected = false;
      if (!missionEditorOpen && !espInfoOpen) drawHeader();
    }
    setupWiFiConnection();
    return;
  }
  //wificonnesso
}

//Connessione mqtt
void setupMqttCommunication(){
  if (!mqttClient.connect(broker, port)) {
  }else{
    mqttConnected = true;
    mqttClient.subscribe(readListTopic);
    publishNotifyTopic();
  }
}

void publishNotifyTopic(){
  mqttClient.beginMessage(notifyTopic, false, 1);
  mqttClient.print("connected");
  mqttClient.endMessage();
}

void handleMqttMessages()
{
  int messageSize = mqttClient.parseMessage();
  if (messageSize == 0) {
    return;
  }

  String topic = mqttClient.messageTopic();

  String payload;
  payload.reserve(messageSize + 1);

  while (mqttClient.available()) {
    payload += (char)mqttClient.read();
  }

  if (topic == readListTopic) {
    bool ok = loadArticlesFromJson(payload);

    if (ok) {
      listUpdated = true;
      if (!missionEditorOpen && !espInfoOpen && !bufferUnloadOpen) drawListFull();
    } else {
    }
  }
}

bool loadArticlesFromJson(const String& json)
{
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return false;
  }

  JsonArray articoli = doc["articoli"].as<JsonArray>();

  if (articoli.isNull()) {
    return false;
  }

  int index = 0;

  for (JsonObject articolo : articoli) {
    if (index >= MAX_ROWS) {
      break;
    }
    articles[index].id = articolo["id"] | -1;
    articles[index].articoloId = articolo["componenteId"] | -1;
    const char* linea = articolo["linea"] | "Z";
    articles[index].linea = linea[0];
    const char* lotto = articolo["lotto"] | "";
    if (strlen(lotto) > 0)
      snprintf(articles[index].lotto, sizeof(articles[index].lotto), "Lotto: %s", lotto);
    else
      snprintf(articles[index].lotto, sizeof(articles[index].lotto), "Lotto assente");

    const char* nome = articolo["nome"] | "";
    const char* startTimestamp = articolo["startTimestamp"] | "";

    safeCopy(articles[index].nome, sizeof(articles[index].nome), nome);
    safeCopy(articles[index].startTimestamp, sizeof(articles[index].startTimestamp), startTimestamp);

    index++;
  }
  rowCount = index;

  // Ogni nuova lista fa ripartire i nomi lunghi dall'inizio.
  articleNameScrollEpoch = millis();
  lastArticleNameScrollFrame = 0;
  for (int i = 0; i < MAX_ROWS; i++) {
    articleNameLastOffset[i] = -1;
  }

  if (selectedItem >= rowCount) {
    selectedItem = -1;
  }

  scrollOffset = clampInt(scrollOffset, 0, maxScrollOffset());

  return true;
}

void safeCopy(char* dest, size_t destSize, const char* source)
{
  if (destSize == 0) {
    return;
  }

  strncpy(dest, source, destSize - 1);
  dest[destSize - 1] = '\0';
}

void publishArticleSelected(int selectedArticle){
  if (selectedArticle < 0 || selectedArticle >= rowCount) {
    return;
  }

  String payload = String(String(articles[selectedArticle].id) + ":" + (articles[selectedArticle].linea) + ":" + String(articles[selectedArticle].nome) + ":" + String(articles[selectedArticle].articoloId) + ":" + String(bufferMission));
  
  mqttClient.beginMessage(selectTopic, false, 1);
  mqttClient.print(payload);
  mqttClient.endMessage();
}

void drawAmrCobotIcon(
  int x,
  int y,
  int size,
  uint16_t color,
  uint16_t bgColor,
  uint16_t borderColor
)
{
  if (size < 48) size = 48;

  // Sprite riutilizzabile: evita artefatti durante lo scroll della lista.
  if (amrCobotSpriteSize != size)
  {
    amrCobotSprite.deleteSprite();
    amrCobotSprite.setColorDepth(16);

    if (amrCobotSprite.createSprite(size, size) == nullptr)
    {
      amrCobotSpriteSize = 0;
      return;
    }

    amrCobotSpriteSize = size;
  }

  LGFX_Sprite& g = amrCobotSprite;
  g.fillSprite(bgColor);

  // ------------------------------------------------------------
  // CONTENITORE ESTERNO
  // ------------------------------------------------------------
  const int corner = size / 7;
  g.drawRoundRect(0, 0, size, size, corner, borderColor);

  // ============================================================
  // AMR
  // ============================================================
  const int amrW = size * 72 / 100;
  const int amrH = size * 15 / 100;
  const int amrX = (size - amrW) / 2;
  const int amrY = size * 77 / 100;

  g.fillRoundRect(
    amrX,
    amrY,
    amrW,
    amrH,
    max(3, size / 14),
    color
  );

  g.drawFastHLine(
    amrX + size * 8 / 100,
    amrY - 2,
    amrW - size * 16 / 100,
    color
  );

  // Ruote
  const int wheelR = max(3, size / 17);
  const int wheelY = min(size - wheelR - 1, amrY + amrH);

  g.fillCircle(
    amrX + amrW / 4,
    wheelY,
    wheelR,
    color
  );

  g.fillCircle(
    amrX + (amrW * 3) / 4,
    wheelY,
    wheelR,
    color
  );

  // ============================================================
  // PEDANA COBOT
  // ============================================================
  // Più piccola e direttamente appoggiata sull'AMR.
  const int pedestalW = size * 60 / 100;
  const int pedestalH = max(4, size * 7 / 100);
  const int pedestalX = (size - pedestalW) / 2;
  const int pedestalY = amrY - pedestalH + 1;

  g.fillRoundRect(
    pedestalX,
    pedestalY,
    pedestalW,
    pedestalH,
    max(2, size / 25),
    color
  );

  // ============================================================
  // COBOT - VERSIONE PIÙ PICCOLA
  // ============================================================
  const int armThickness = max(3, size / 16);

  // Giunto inferiore
  const int j1X = size * 50 / 100;
  const int j1Y = pedestalY - size * 3 / 100;

  // Giunto alto sinistro
  const int j2X = size * 38 / 100;
  const int j2Y = size * 36 / 100;

  // Giunto alto destro
  const int j3X = size * 63 / 100;
  const int j3Y = size * 36 / 100;

  const int j1R = max(4, size / 14);
  const int j2R = max(3, size / 16);
  const int j3R = max(3, size / 16);

  // Colonna corta dalla pedana al giunto inferiore
  g.drawWideLine(
    j1X,
    pedestalY + 1,
    j1X,
    j1Y,
    armThickness,
    color
  );

  // Braccio diagonale
  g.drawWideLine(
    j1X,
    j1Y,
    j2X,
    j2Y,
    armThickness,
    color
  );

  // Braccio superiore
  g.drawWideLine(
    j2X,
    j2Y,
    j3X,
    j3Y,
    armThickness,
    color
  );

  // Giunti cavi
  g.fillCircle(j1X, j1Y, j1R, bgColor);
  g.drawCircle(j1X, j1Y, j1R, color);

  g.fillCircle(j2X, j2Y, j2R, bgColor);
  g.drawCircle(j2X, j2Y, j2R, color);

  g.fillCircle(j3X, j3Y, j3R, bgColor);
  g.drawCircle(j3X, j3Y, j3R, color);

  // ============================================================
  // POLSO + PINZA
  // ============================================================

  // Attacco corto e sottile, leggermente inclinato verso destra.
  const int wristStartX = j3X + j3R / 2;
  const int wristStartY = j3Y + j3R / 2;

  const int wristX = size * 68 / 100;
  const int wristY = size * 44 / 100;
  const int gripThickness = 1.5;

  g.drawWideLine(
    wristStartX,
    wristStartY,
    wristX,
    wristY,
    gripThickness,
    color
  );

  // Collo piccolo della pinza, appena accennato.
  const int neckBottomX = size * 69 / 100;
  const int neckBottomY = size * 48 / 100;

  g.drawLine(wristX, wristY, neckBottomX, neckBottomY, color);
  g.drawLine(wristX + 1, wristY, neckBottomX + 1, neckBottomY, color);

  // ------------------------------------------------------------
  // GRIPPER
  // ------------------------------------------------------------
  // Leggermente più grande della versione minimale.
  const int topLeftX   = size * 65 / 100;
  const int topRightX  = size * 74 / 100;
  const int topY       = size * 49 / 100;

  const int sideLeftX  = size * 62 / 100;
  const int sideRightX = size * 77 / 100;
  const int sideY      = size * 55 / 100;

  const int clawLeftX  = size * 64 / 100;
  const int clawRightX = size * 75 / 100;
  const int clawY      = size * 62 / 100;


  // Traverso alto
  g.drawWideLine(
    topLeftX,
    topY,
    topRightX,
    topY,
    gripThickness,
    color
  );

  // Spalle inclinate
  g.drawWideLine(
    topLeftX,
    topY,
    sideLeftX,
    sideY,
    gripThickness,
    color
  );

  g.drawWideLine(
    topRightX,
    topY,
    sideRightX,
    sideY,
    gripThickness,
    color
  );

  // Dita aperte
  g.drawWideLine(
    sideLeftX,
    sideY,
    clawLeftX,
    clawY, 
    gripThickness,
    color
  );

  g.drawWideLine(
    sideRightX,
    sideY,
    clawRightX,
    clawY,
    gripThickness,
    color
  );

  // Terminali piccoli ma visibili.
  

  // Un solo push sul display: mantiene pulito lo scroll.
  g.pushSprite(x, y);
}