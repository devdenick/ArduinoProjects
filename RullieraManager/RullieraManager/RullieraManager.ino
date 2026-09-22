#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>
#include <atomic>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <tdslite.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "LGFX_4D_43CT.h"

LGFX_4D_43CT lcd;

// ==================== SQL SERVER ====================
// Installare "tdslite" dal Gestore librerie Arduino.
// Login SQL Server (utente/password), IP e porta TCP dell'istanza.
// WiFiClient non implementa TLS per SQL Server.
const char SQL_SERVER[]   = "ostv00005.zf-world.com";
const uint16_t SQL_PORT   = 1433;
const char SQL_DATABASE[] = "Innovation_DB";
const char SQL_USER[]     = "trwuser";
const char SQL_PASSWORD[] = "trwuser";

// READ al tocco; SAVE soltanto alla conferma. Nessuna query sui tasti +/-.
struct SqlRequest {
    int index;
    char id;
    bool save;
    long long expected;
    long long target;
};
struct SqlResult {
    int index;
    bool save;
    bool error;
    uint32_t rowCount;
    char quantity[101];
    char message[192];
};
QueueHandle_t sqlRequests = nullptr;
QueueHandle_t sqlResults = nullptr;
TaskHandle_t sqlTaskHandle = nullptr;
bool sqlBusy = false; // Stato gestito esclusivamente dal loop.
int sqlBusyIndex = -1;

void sqlInfoCallback(void*, const tdsl::tds_info_token&) noexcept;
void sqlRowCallback(void*, const tdsl::tds_colmetadata_token&, const tdsl::tdsl_row&);
void databaseTask(void*);
void startDatabaseTask();
void updateDatabaseLoop();
bool requestSql(int index, bool save);
bool parseQuantity(const char* text, long long& value);
int indiceRullieraSelezionata();
int pulsanteQuantitaInPunto(int x, int y);
void modificaQuantitaRulliera(int index, int delta);
void drawConfirmButton();
void disegnaRettangoliRulliere();

// COMPILARE le credenziali sul proprio PC.
const char* ssid = "ZFIOT";
const char* password = "XGWgxXcyLiyaMY4n9YDg9CKC";
const char broker[] = "10.18.129.41";
const int port = 1883; // Porta MQTT: impostare quella del proprio broker.
const char notifyTopic[] = "flowrack/buffer/connected";
const char selectTopic[] = "flowrack/buffer/selected"; // Solo riferimento nella pagina info.
const char readListTopic[] = "flowrack/buffer/update";
const char* PERCORSO_SFONDO = "/sfondorulliera.jpg";
constexpr int IMMAGINE_LARGHEZZA = 1086, IMMAGINE_ALTEZZA = 1448;
constexpr uint8_t ROTAZIONE_DISPLAY = 3;
constexpr uint8_t LUMINOSITA = 128;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
String macAddress;
bool wifiConnected = false, mqttConnected = false;
std::atomic<bool> networkWifi(false), networkMqtt(false);
bool filesystemReady = false;

constexpr int HEADER_X=12, HEADER_Y=12, HEADER_W=456, HEADER_H=84;
constexpr int CHIP_LOGO_CX=44, CHIP_LOGO_CY=54, CHIP_LOGO_RADIUS=22;
constexpr unsigned long HEADER_LONG_PRESS_MS=2000;
constexpr int HEADER_LONG_PRESS_MOVE_TOLERANCE=20, TAP_MOVE_TOLERANCE=12;
constexpr int ESP_INFO_X=24, ESP_INFO_Y=108, ESP_INFO_W=432, ESP_INFO_H=584;
constexpr int ESP_INFO_CLOSE_X=126, ESP_INFO_CLOSE_Y=620;
constexpr int ESP_INFO_CLOSE_W=228, ESP_INFO_CLOSE_H=52;
uint16_t COL_BG, COL_SURFACE, COL_CARD, COL_BORDER, COL_TEXT, COL_MUTED;
uint16_t COL_BLUE_SOFT, COL_GREEN, COL_ORANGE, COL_RED, COL_DARK_BUTTON;
constexpr int COUNTER_TOP = 620;
constexpr int COUNTER_BUTTON_Y = 674;
constexpr int COUNTER_BUTTON_W = 80;
constexpr int COUNTER_BUTTON_H = 54;
constexpr int CONFIRM_Y = 744;
constexpr int CONFIRM_H = 44;
constexpr int CONFIRM_W = 186;
constexpr int CANCEL_X = 50;
constexpr int CONFIRM_X = 244;
constexpr int COUNTER_MINUS_X = 50;
constexpr int COUNTER_PLUS_X = 350;
constexpr int COUNTER_VALUE_X = 140;
constexpr int COUNTER_VALUE_W = 200;
bool counterTouchActive = false;
bool counterTouchCancelled = false;
int counterTouchDelta = 0;
int counterTouchIndex = -1;

bool touchDown=false, touchMoved=false, headerLongPressCandidate=false;
int touchStartX=0, touchStartY=0;
unsigned long headerLongPressStart=0;
int headerPressLastProgress=-1;
bool espInfoOpen=false, espInfoTouchDown=false, espInfoIgnoreUntilRelease=false;
int espInfoTouchStartX=0, espInfoTouchStartY=0, espInfoLastTouchX=0, espInfoLastTouchY=0;
int selectRullieraYArea = 100;
bool rullieraSelected = false;
char rullieraSelectedId = ' ';

struct RettangoloRulliera
{
    char id;
    const char* articleName;
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    bool selected;

    bool contiene(int touchX, int touchY) const
    {
        return touchX >= x &&
               touchX < x + width &&
               touchY >= y - selectRullieraYArea &&
               touchY < y + height;
    }
};

RettangoloRulliera rettangoliRulliera[] =
{
    // ID    X    Y    W   H
    // Piano superiore
    { 'I', "FLANGE_MIDDLE_PLATE_DL382_CASTED",  205, 282,  59, 27, false },
    { 'L', "DRIVEN GEAR_HP",                    267, 282,  54, 27, false  },
    { 'M', "DRIVER_GEAR_HP",                    324, 282,  54, 27, false  },
    { 'N', "HOUSING_HP",                        381, 282,  57, 27, false  },

    // Piano centrale
    { 'E', "COVER",                          42, 405,  98, 26, false  },
    { 'F', "DRIVEN_GEAR",                   144, 405,  94, 26, false  },
    { 'G', "DRIVER_GEAR",                   242, 405,  94, 26, false  },
    { 'H', "LP_FLANGE_HOUSING_INTEGRETED",    340, 405,  98, 26, false  },

    // Piano inferiore
    { 'A',"COVER",                       42, 524,  98, 26, false  },
    { 'B', "DRIVEN_GEAR",                 144, 524,  94, 26, false  },
    { 'C', "DRIVER_GEAR",                 242, 524,  94, 26, false  },
    { 'D', "LP_FLANGE_HOUSING_INTEGRETED",  340, 524,  98, 26, false  }
};

constexpr size_t NUM_RULLIERE =
    sizeof(rettangoliRulliera) / sizeof(rettangoliRulliera[0]);


// Cache reale: mai inizializzata con una quantità fittizia.
struct QuantityState {
    bool valid;
    bool failed;
    bool refresh;
    long long original;
    long long draft;
};
QuantityState quantityStates[NUM_RULLIERE] = {};

int trovaRulliera(int touchX, int touchY)
{
    const int current=indiceRullieraSelezionata();
    // Non scartare una bozza cambiando rulliera: prima Conferma o Annulla.
    if (current>=0 && quantityStates[current].valid &&
        quantityStates[current].draft!=quantityStates[current].original) {
        if (!espInfoOpen) drawConfirmButton();
        Serial.println("[UI] Confermare o annullare le modifiche prima di cambiare rulliera.");
        return current;
    }
    if (sqlBusy && sqlBusyIndex==current) return current;
    int found = -1;
    for (size_t i=0; i<NUM_RULLIERE; ++i) {
        if (rettangoliRulliera[i].contiene(touchX,touchY)) {
            found = static_cast<int>(i);
            break;
        }
    }
    const char id = found >= 0 ? rettangoliRulliera[found].id : ' ';
    for (size_t i=0; i<NUM_RULLIERE; ++i)
        rettangoliRulliera[i].selected = static_cast<int>(i)==found;
    rullieraSelected = found >= 0;
    rullieraSelectedId = id;
    if (found >= 0) {
        // Anche un nuovo tocco sulla stessa rulliera richiede una lettura.
        quantityStates[found].valid = false;
        quantityStates[found].failed = false;
        quantityStates[found].refresh = true;
    }
    disegnaRettangoliRulliere();
    drawConfirmButton();
    return found;
}

void drawText(int x,int y,const String& text,uint16_t color,
 const lgfx::IFont* font=&fonts::Font2,float size=1.0f,
 textdatum_t datum=textdatum_t::top_left);
void drawTextCentered(int x,int y,int w,int h,const String& text,uint16_t color,
 const lgfx::IFont* font=&fonts::Font2,float size=1.0f);
void drawEspInfoLine(int y,const String& label,const String& value,uint16_t valueColor=COL_TEXT);
void drawDashboard();
void drawEspInfoScreen();
void closeEspInfoScreen();
void drawChipCore(int cx,int cy);

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

void drawHeader()
{
  lcd.fillRoundRect(HEADER_X, HEADER_Y, HEADER_W, HEADER_H, 12, COL_SURFACE);
  lcd.drawRoundRect(HEADER_X, HEADER_Y, HEADER_W, HEADER_H, 12, COL_BORDER);

  drawChipLogo(CHIP_LOGO_CX, CHIP_LOGO_CY);

  drawText(78, CHIP_LOGO_CY - (CHIP_LOGO_RADIUS/2), "Rulliera Linea S", COL_TEXT, &fonts::Font4, 0.85f);

  drawStatusChip(302, 25, 74, "WiFi", wifiConnected);
  drawStatusChip(384, 25, 72, "MQTT", mqttConnected);

  drawText(306, 64, wifiConnected ? "ON" : "OFF", wifiConnected ? COL_GREEN : COL_RED, &fonts::Font2);
  drawText(390, 64, mqttConnected ? "ON" : "OFF", mqttConnected ? COL_GREEN : COL_RED, &fonts::Font2);
}

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

  lcd.setFont(&fonts::Font2);
  lcd.setTextSize(1.0f);
  int tw = lcd.textWidth(value);
  float scale = tw > 274 ? 274.0f / tw : 1.0f;
  drawText(
    ESP_INFO_X + 138,
    y,
    value,
    valueColor,
    &fonts::Font2,
    scale
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
  counterTouchActive = false;
  counterTouchDelta = 0;
  counterTouchIndex = -1;
  espInfoOpen = true;
  espInfoTouchDown = false;

  // Il dito è ancora appoggiato dopo i 2 secondi.
  espInfoIgnoreUntilRelease = true;

  headerLongPressCandidate = false;
  headerPressLastProgress = -1;


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
  disegnaRettangoliRulliere();
  drawConfirmButton();
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


void drawDashboard()
{
  lcd.fillScreen(COL_BG);
  // Area sotto l'header: la rulliera non viene coperta dalla testata.
  const int top=104, areaH=lcd.height()-top;
  float sx=(float)lcd.width()/IMMAGINE_LARGHEZZA;
  float sy=(float)areaH/IMMAGINE_ALTEZZA;
  float scale=sx<sy?sx:sy;
  int w=(int)(IMMAGINE_LARGHEZZA*scale), h=(int)(IMMAGINE_ALTEZZA*scale);
  int x=(lcd.width()-w)/2, y=top+(areaH-h)/2-70;
  bool ok=false;
  if(filesystemReady && LittleFS.exists(PERCORSO_SFONDO)) {
    ok=lcd.drawJpgFile(LittleFS,PERCORSO_SFONDO,x,y,
                       lcd.width()-x,lcd.height()-y,0,0,scale,scale);
  }
  if(!ok) {
    Serial.println("Sfondo non disponibile: verificare LittleFS e nome JPG");
    drawText(20,150,"Sfondo non disponibile",COL_RED);
  }
  drawHeader();
}

void handleTouch()
{
    uint16_t x=0,y=0;
    const bool pressed=lcd.getTouch(&x,&y);
    if (espInfoOpen) { handleEspInfoTouch(pressed,x,y); return; }
    if (pressed) {
        if (!touchDown) {
            touchDown=true; touchStartX=x; touchStartY=y;
            counterTouchActive=false; counterTouchCancelled=false;
            counterTouchDelta=0; counterTouchIndex=-1;
            headerLongPressCandidate=false; headerPressLastProgress=-1;
            if (y>=COUNTER_TOP) {
                counterTouchActive=true;
                counterTouchDelta=pulsanteQuantitaInPunto(x,y);
                counterTouchIndex=indiceRullieraSelezionata();
                return;
            }
            trovaRulliera(x,y);
            headerLongPressCandidate=pointInRect(x,y,HEADER_X,HEADER_Y,HEADER_W,HEADER_H);
            headerLongPressStart=millis();
        }
        if (counterTouchActive) {
            if (abs(static_cast<int>(x)-touchStartX)>TAP_MOVE_TOLERANCE ||
                abs(static_cast<int>(y)-touchStartY)>TAP_MOVE_TOLERANCE ||
                pulsanteQuantitaInPunto(x,y)!=counterTouchDelta)
                counterTouchCancelled=true;
            return;
        }
        if (headerLongPressCandidate) {
            if (abs(static_cast<int>(x)-touchStartX)>HEADER_LONG_PRESS_MOVE_TOLERANCE ||
                abs(static_cast<int>(y)-touchStartY)>HEADER_LONG_PRESS_MOVE_TOLERANCE ||
                !pointInRect(x,y,HEADER_X-10,HEADER_Y-10,HEADER_W+20,HEADER_H+20)) {
                headerLongPressCandidate=false; headerPressLastProgress=-1;
                drawChipLogo(CHIP_LOGO_CX,CHIP_LOGO_CY);
            } else {
                drawHeaderLongPressProgress();
                if (millis()-headerLongPressStart>=HEADER_LONG_PRESS_MS) openEspInfoScreen();
            }
        }
        return;
    }
    if (!touchDown) return;
    touchDown=false;
    if (counterTouchActive) {
        const bool apply=!counterTouchCancelled && counterTouchDelta!=0 &&
            counterTouchIndex>=0 && counterTouchIndex==indiceRullieraSelezionata();
        const int index=counterTouchIndex, delta=counterTouchDelta;
        counterTouchActive=false; counterTouchCancelled=false;
        counterTouchDelta=0; counterTouchIndex=-1;
        if (apply) modificaQuantitaRulliera(index,delta);
        return;
    }
    if (headerLongPressCandidate) drawChipLogo(CHIP_LOGO_CX,CHIP_LOGO_CY);
    headerLongPressCandidate=false; headerPressLastProgress=-1;
}

// Solo questo task accede a wifiClient/mqttClient. Nessun disegno dal task.
void networkTask(void*)
{
  if(ssid[0]=='\0') {
    Serial.println("Inserire ssid e password nello sketch per connettere il WiFi");
    vTaskDelete(nullptr); return;
  }
  String clientId="Rulliera-"+macAddress;
  clientId.replace(":", "");
  mqttClient.setId(clientId);
  mqttClient.setConnectionTimeout(3000);
  mqttClient.setKeepAliveInterval(20000);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid,password);
  uint32_t lastWiFi=millis(), lastMqtt=millis()-5000;
  bool hadWiFi=false;
  for(;;) {
    bool connected=WiFi.status()==WL_CONNECTED;
    networkWifi.store(connected);
    if(!connected) {
      networkMqtt.store(false);
      if(hadWiFi) mqttClient.stop();
      hadWiFi=false;
      lastMqtt=millis()-5000;
      if(millis()-lastWiFi>=15000) {
        lastWiFi=millis(); WiFi.reconnect();
      }
    } else {
      hadWiFi=true;
      if(broker[0]!='\0' && !mqttClient.connected()) {
        networkMqtt.store(false);
        if(millis()-lastMqtt>=5000) {
          lastMqtt=millis();
          if(mqttClient.connect(broker,port)) {
            mqttClient.subscribe(readListTopic);
            // Stesso topic, payload e QoS del progetto originale.
            mqttClient.beginMessage(notifyTopic,false,1);
            mqttClient.print("connected");
            mqttClient.endMessage();
            Serial.println("MQTT connesso");
          } else {
            Serial.printf("Connessione MQTT fallita: %d\n",mqttClient.connectError());
          }
        }
      }
      if(mqttClient.connected()) {
        mqttClient.poll();
        // La lista articoli non fa parte di questa integrazione.
        // Consuma i messaggi senza allocare un payload illimitato.
        int size=mqttClient.parseMessage();
        if(size>0) {
          Serial.printf("MQTT ricevuto: %d byte\n",size);
          while(mqttClient.available()) mqttClient.read();
        }
      }
      networkMqtt.store(mqttClient.connected());
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void updateConnectionDisplay()
{
  bool w=networkWifi.load(), m=networkMqtt.load();
  if(w==wifiConnected && m==mqttConnected) return;
  wifiConnected=w; mqttConnected=m;
  if(espInfoOpen) drawEspInfoScreen();
  else {
    drawHeader();
    if(headerLongPressCandidate) {
      headerPressLastProgress=-1;
      drawHeaderLongPressProgress();
    }
  }
}

void drawRectWithBorderSize(
    int x, int y,
    int w, int h,
    int spessore,
    uint16_t colore)
{
    for (int i = 0; i < spessore; i++)
    {
        if (w - 2 * i <= 0 || h - 2 * i <= 0)
            break;

        lcd.drawRect(
            x + i,
            y + i,
            w - 2 * i,
            h - 2 * i,
            colore
        );
    }
}

void drawRoundRectWithBorderSize(
    int x, int y,
    int w, int h,
    int round, int spessore,
    uint16_t colore)
{
    for (int i = 0; i < spessore; i++)
    {
        if (w - 2 * i <= 0 || h - 2 * i <= 0)
            break;

        lcd.drawRoundRect(
            x + i,
            y + i,
            w - 2 * i,
            h - 2 * i,
            round,
            colore
        );
    }
}

void disegnaRettangoliRulliere(){
  for (const auto& rettangolo : rettangoliRulliera)
    {
      if(rettangolo.selected)
        drawRectWithBorderSize(rettangolo.x, rettangolo.y, rettangolo.width, rettangolo.height, 3, TFT_GREEN);
      else
        drawRectWithBorderSize(rettangolo.x, rettangolo.y, rettangolo.width, rettangolo.height, 3, TFT_RED);
    }
}


// ==================== SQL: LETTURA E SALVATAGGIO SU CONFERMA ====================
void sqlInfoCallback(void* context, const tdsl::tds_info_token& token) noexcept
{
    if (token.is_info()) return;
    auto& result=*static_cast<SqlResult*>(context);
    result.error=true;
    int n=snprintf(result.message,sizeof(result.message),"SQL %lu: ",
                   static_cast<unsigned long>(token.number));
    size_t pos=n>0 ? static_cast<size_t>(n) : 0;
    if (pos>=sizeof(result.message)) pos=sizeof(result.message)-1;
    for (const auto ch: token.msgtext) {
        if (pos+1>=sizeof(result.message)) break;
        result.message[pos++]=ch<128 ? static_cast<char>(ch) : '?';
    }
    result.message[pos]='\0';
}

void sqlRowCallback(void* context, const tdsl::tds_colmetadata_token&,
                    const tdsl::tdsl_row& row)
{
    auto& result=*static_cast<SqlResult*>(context);
    ++result.rowCount;
    if (result.rowCount!=1 || row[0].is_null()) {
        result.error=true;
        snprintf(result.message,sizeof(result.message),"Risposta SQL non valida o NULL");
        return;
    }
    size_t pos=0;
    for (const char ch: row[0].as<tdsl::char_view>()) {
        if (pos+1>=sizeof(result.quantity)) {
            result.error=true;
            break;
        }
        result.quantity[pos++]=ch;
    }
    result.quantity[pos]='\0';
}

// Accetta quantità intere non negative, anche da DECIMAL (es. "12.000").
// Non arrotonda silenziosamente valori frazionari.
bool parseQuantity(const char* text, long long& value)
{
    if (!text || text[0]<'0' || text[0]>'9') return false;
    errno=0;
    char* end=nullptr;
    const long long parsed=strtoll(text,&end,10);
    if (errno==ERANGE || parsed<0 || end==text) return false;
    if (*end=='.') {
        ++end;
        if (*end=='\0') return false;
        while (*end=='0') ++end;
    }
    if (*end!='\0') return false;
    value=parsed;
    return true;
}

// Scrittura assoluta con confronto sul valore letto: se un altro sistema
// cambia QT_ACT durante l'editing, non viene sovrascritto in silenzio.
// Il valore confermato viene restituito solo DOPO COMMIT.
bool buildSql(const SqlRequest& request, char* query, size_t capacity)
{
    if (request.index<0 || request.index>=static_cast<int>(NUM_RULLIERE) ||
        request.id!=rettangoliRulliera[request.index].id ||
        request.id<'A' || request.id>'Z' || request.expected<0 || request.target<0)
        return false;
    int n=0;
    if (!request.save) {
        // Lettura semplice: niente lock di aggiornamento durante l'editing.
        n=snprintf(query,capacity,
            "SET NOCOUNT ON; SET LOCK_TIMEOUT 5000; "
            "DECLARE @n INT, @q VARCHAR(100); "
            "SELECT @n=COUNT(*), @q=MAX(CONVERT(VARCHAR(100),QT_ACT)) "
            "FROM RULLIERA WHERE UBICAZIONE_ID='0' AND ID='%c'; "
            "IF @n<>1 BEGIN ;THROW 50001,'Rulliera assente o duplicata',1; END; "
            "IF @q IS NULL BEGIN ;THROW 50002,'QT_ACT e NULL',1; END; "
            "SELECT @q AS QT_ACT;",request.id);
    } else {
        n=snprintf(query,capacity,
            "SET NOCOUNT ON; SET XACT_ABORT ON; SET LOCK_TIMEOUT 5000; "
            "BEGIN TRY BEGIN TRANSACTION; "
            "DECLARE @n INT, @q VARCHAR(100); "
            "SELECT @n=COUNT(*) FROM RULLIERA WITH (UPDLOCK,HOLDLOCK) "
            "WHERE UBICAZIONE_ID='0' AND ID='%c'; "
            "IF @n<>1 BEGIN ;THROW 50001,'Rulliera assente o duplicata',1; END; "
            "UPDATE RULLIERA SET QT_ACT=%lld "
            "WHERE UBICAZIONE_ID='0' AND ID='%c' AND QT_ACT=%lld; "
            "IF @@ROWCOUNT<>1 BEGIN ;THROW 50004,'Quantita cambiata: rileggere la rulliera',1; END; "
            "SELECT @q=CONVERT(VARCHAR(100),QT_ACT) FROM RULLIERA "
            "WHERE UBICAZIONE_ID='0' AND ID='%c'; "
            "IF @q IS NULL BEGIN ;THROW 50003,'QT_ACT non valido',1; END; "
            "COMMIT TRANSACTION; SELECT @q AS QT_ACT; "
            "END TRY BEGIN CATCH "
            "IF @@TRANCOUNT>0 ROLLBACK TRANSACTION; THROW; END CATCH;",
            request.id,request.target,request.id,request.expected,request.id);
    }
    return n>0 && static_cast<size_t>(n)<capacity;
}

void databaseTask(void*)
{
    static tdsl::uint8_t sqlBuffer[8192]={};
    for (;;) {
        SqlRequest request{};
        if (xQueueReceive(sqlRequests,&request,portMAX_DELAY)!=pdTRUE) continue;
        SqlResult result{};
        result.index=request.index; result.save=request.save;
        char query[1800]={};
        if (!buildSql(request,query,sizeof(query))) {
            result.error=true;
            snprintf(result.message,sizeof(result.message),"Richiesta SQL non valida");
        } else if (WiFi.status()!=WL_CONNECTED) {
            result.error=true;
            snprintf(result.message,sizeof(result.message),"WiFi non connesso");
        } else {
            WiFiClient sqlClient;
            tdsl::arduino_driver<WiFiClient&> db{sqlBuffer,sqlClient};
            decltype(db)::connection_parameters params{};
            params.server_name=SQL_SERVER; params.port=SQL_PORT;
            params.db_name=SQL_DATABASE; params.user_name=SQL_USER;
            params.password=SQL_PASSWORD;
            params.client_name="ESP32-Rulliera"; params.app_name="RullieraQuantity";
            params.packet_size={1400}; params.conn_retry_count=1;
            params.conn_retry_delay_ms=1000;
            db.set_info_callback(sqlInfoCallback,&result);
            const auto connection=db.connect(params);
            if (connection!=decltype(db)::e_driver_error_code::success) {
                result.error=true;
                if (!result.message[0]) snprintf(result.message,sizeof(result.message),
                    "Connessione fallita, codice tdslite: %d",static_cast<int>(connection));
            } else {
                const auto execution=db.execute_query(query,sqlRowCallback,&result);
                if (!execution || result.rowCount!=1 || !result.quantity[0]) {
                    result.error=true;
                    if (!result.message[0]) snprintf(result.message,sizeof(result.message),
                        "Risposta SQL incompleta: rileggere il valore");
                }
            }
            // Chiude TCP a fine lettura/salvataggio, anche in caso di errore.
            // Nessuna sessione SQL rimane aperta durante l'editing locale.
            sqlClient.stop();
        }
        // Mai ripetere automaticamente un UPDATE: in caso di perdita della
        // risposta potrebbe essere stato gia' applicato. Toccare di nuovo la
        // rulliera per rileggere il valore; nessun retry automatico.
        xQueueOverwrite(sqlResults,&result);
    }
}

void startDatabaseTask()
{
    if (!SQL_SERVER[0] || !SQL_DATABASE[0] || !SQL_USER[0]) {
        Serial.println("[DB] Compilare SQL_SERVER, SQL_DATABASE, SQL_USER e SQL_PASSWORD.");
        return;
    }
    sqlRequests=xQueueCreate(1,sizeof(SqlRequest));
    sqlResults=xQueueCreate(1,sizeof(SqlResult));
    if (!sqlRequests || !sqlResults) {
        if (sqlRequests) vQueueDelete(sqlRequests);
        if (sqlResults) vQueueDelete(sqlResults);
        sqlRequests=nullptr; sqlResults=nullptr;
        Serial.println("[DB] Memoria insufficiente per code SQL.");
        return;
    }
    if (xTaskCreatePinnedToCore(databaseTask,"rullieraSql",12288,nullptr,
                               1,&sqlTaskHandle,0)!=pdPASS) {
        vQueueDelete(sqlRequests); vQueueDelete(sqlResults);
        sqlRequests=nullptr; sqlResults=nullptr; sqlTaskHandle=nullptr;
        Serial.println("[DB] Impossibile avviare task SQL.");
    }
}

bool requestSql(int index, bool save)
{
    if (!sqlTaskHandle || sqlBusy || WiFi.status()!=WL_CONNECTED ||
        index<0 || index>=static_cast<int>(NUM_RULLIERE)) return false;
    auto& state=quantityStates[index];
    if (save && (!state.valid || state.original==state.draft)) return false;
    SqlRequest request{index,rettangoliRulliera[index].id,save,state.original,state.draft};
    if (xQueueSend(sqlRequests,&request,0)!=pdTRUE) return false;
    sqlBusy=true; sqlBusyIndex=index;
    state.refresh=false; state.failed=false;
    // Mantiene la bozza fino alla risposta; controlli disabilitati con sqlBusy.
    if (!espInfoOpen) drawConfirmButton();
    return true;
}

void updateDatabaseLoop()
{
    // Nessuna lettura periodica. Le richieste nascono solo dal touch.
    // La cache e il display appartengono esclusivamente al loop Arduino.
    static bool previousWifi=false;
    const bool wifi=WiFi.status()==WL_CONNECTED;
    if (wifi!=previousWifi) {
        previousWifi=wifi;
        // Conserva la bozza offline; il salvataggio confrontera' il valore
        // originale sul server prima di applicare la modifica.
        if (!espInfoOpen) drawConfirmButton();
    }
    SqlResult result{};
    if (sqlResults && xQueueReceive(sqlResults,&result,0)==pdTRUE) {
        sqlBusy=false; sqlBusyIndex=-1;
        if (result.index>=0 && result.index<static_cast<int>(NUM_RULLIERE)) {
            auto& state=quantityStates[result.index];
            state.refresh=false;
            long long value=0;
            if (!result.error && !parseQuantity(result.quantity,value)) {
                result.error=true;
                snprintf(result.message,sizeof(result.message),
                    "QT_ACT deve essere un intero non negativo rappresentabile");
            }
            state.valid=!result.error;
            state.failed=result.error;
            if (state.valid) {
                state.original=value; state.draft=value;
                Serial.printf("[DB] %s | Rulliera %c | UBICAZIONE_ID=0 | QT_ACT=%lld\n",
                    result.save ? "SALVATO" : "LETTO",rettangoliRulliera[result.index].id,value);
            } else {
                Serial.printf("[DB] Rulliera %c | ERRORE: %s\n",
                    rettangoliRulliera[result.index].id,result.message);
                if (result.save) Serial.println(
                    "[DB] Salvataggio non ripetuto. Toccare la rulliera per rileggere il valore reale.");
            }
        }
        if (!espInfoOpen) drawConfirmButton();
    }
    const int selected=indiceRullieraSelezionata();
    if (selected<0 || sqlBusy || !wifi || !sqlTaskHandle || counterTouchActive) return;
    if (quantityStates[selected].refresh) requestSql(selected,false);
}

void setup()
{
  Serial.begin(115200);
  lcd.init(); lcd.setColorDepth(16);
  lcd.setRotation(ROTAZIONE_DISPLAY);
  // Riprende la relazione rotazione/touch documentata nel progetto sorgente:
  // rotazione 3 -> offset 7; rotazione 1 -> offset 5.
  auto touchCfg=lcd._touch_instance.config();
  touchCfg.offset_rotation=(ROTAZIONE_DISPLAY==1)?5:7;
  lcd._touch_instance.config(touchCfg);
  lcd.setBrightness(LUMINOSITA);
  lcd.setTextWrap(false,false);
  COL_BG=lcd.color565(10,32,60);
  COL_SURFACE=lcd.color565(13,23,35);
  COL_CARD=lcd.color565(22,34,49);
  COL_BORDER=lcd.color565(49,66,84);
  COL_TEXT=lcd.color565(245,247,250);
  COL_MUTED=lcd.color565(190,199,210);
  COL_BLUE_SOFT=lcd.color565(24,62,98);
  COL_GREEN=lcd.color565(76,190,86);
  COL_ORANGE=lcd.color565(255,164,45);
  COL_RED=lcd.color565(230,75,75);
  COL_DARK_BUTTON=lcd.color565(42,56,72);
  WiFi.mode(WIFI_STA);
  macAddress=WiFi.macAddress();
  filesystemReady=LittleFS.begin(false);
  drawDashboard();
  disegnaRettangoliRulliere();
  drawConfirmButton();
  startDatabaseTask();
  drawConfirmButton();
  // Tiene il task di rete sul core 0, separandolo dal loop Arduino/UI
  // che sull'ESP32-S3 normalmente lavora sul core 1.
  // Questo riduce le interferenze temporali durante gli aggiornamenti grafici.
  if(xTaskCreatePinnedToCore(
       networkTask,
       "rullieraNet",
       8192,
       nullptr,
       1,
       nullptr,
       0
     ) != pdPASS)
  {
    Serial.println("Impossibile avviare task rete");
  }
}

int indiceRullieraSelezionata()
{
    if (!rullieraSelected) return -1;
    for (size_t i=0;i<NUM_RULLIERE;++i)
        if (rettangoliRulliera[i].id==rullieraSelectedId) return static_cast<int>(i);
    return -1;
}

// Azioni touch: -1 meno, +1 piu', 2 conferma, 3 annulla.
int pulsanteQuantitaInPunto(int x,int y)
{
    const int index=indiceRullieraSelezionata();
    if (index<0 || sqlBusy || !quantityStates[index].valid) return 0;
    const auto& state=quantityStates[index];
    if (pointInRect(x,y,COUNTER_MINUS_X,COUNTER_BUTTON_Y,COUNTER_BUTTON_W,COUNTER_BUTTON_H))
        return state.draft>0 ? -1 : 0;
    if (pointInRect(x,y,COUNTER_PLUS_X,COUNTER_BUTTON_Y,COUNTER_BUTTON_W,COUNTER_BUTTON_H))
        return state.draft<LLONG_MAX ? 1 : 0;
    if (state.draft==state.original) return 0;
    if (pointInRect(x,y,CANCEL_X,CONFIRM_Y,CONFIRM_W,CONFIRM_H)) return 3;
    if (pointInRect(x,y,CONFIRM_X,CONFIRM_Y,CONFIRM_W,CONFIRM_H) &&
        sqlTaskHandle && WiFi.status()==WL_CONNECTED) return 2;
    return 0;
}

void modificaQuantitaRulliera(int index,int action)
{
    if (index<0 || index>=static_cast<int>(NUM_RULLIERE) || sqlBusy) return;
    auto& state=quantityStates[index];
    if (!state.valid) return;
    if (action==2) { requestSql(index,true); return; }
    if (action==3) state.draft=state.original;
    else if (action==1 && state.draft<LLONG_MAX) ++state.draft;
    else if (action==-1 && state.draft>0) --state.draft;
    else return;
    // Risposta immediata: nessuna connessione o query in questo ramo.
    drawConfirmButton();
}

void drawConfirmButton()
{
    lcd.fillRect(0,COUNTER_TOP,lcd.width(),lcd.height()-COUNTER_TOP,COL_BG);
    const int index=indiceRullieraSelezionata();
    if (index<0) return;
    const auto& state=quantityStates[index];
    // ID e nome articolo della rulliera selezionata, es. "A: COVER".
    const auto& selectedRack = rettangoliRulliera[index];
    char letter[2] = {selectedRack.id, '\0'};
    String rackTitle = String(letter) + ": ";
    rackTitle += selectedRack.articleName ? selectedRack.articleName : "";

    // Adatta i nomi lunghi alla larghezza disponibile senza andare a capo.
    const int titleX = 12;
    const int titleW = lcd.width() - 2 * titleX;
    lcd.setFont(&fonts::Font4);
    lcd.setTextSize(1.0f);
    const int titleTextWidth = lcd.textWidth(rackTitle);
    float titleScale = 1.0f;
    if (titleTextWidth > titleW && titleW > 0) {
        titleScale = static_cast<float>(titleW) / titleTextWidth;
    }
    drawTextCentered(titleX,621,titleW,32,rackTitle,COL_TEXT,
                     &fonts::Font4,titleScale);
    const bool wifi=WiFi.status()==WL_CONNECTED;
    const bool enabled=!sqlBusy && state.valid;
    const bool dirty=state.valid && state.draft!=state.original;
    const char* hint=state.failed ? "Errore: tocca la rulliera per rileggere" :
        sqlBusy ? "Attendere il database..." :
        dirty ? "Modifiche da confermare" : "";
    drawTextCentered(0,655,lcd.width(),16,hint,dirty ? COL_ORANGE : COL_MUTED,&fonts::Font2,0.85f);
    lcd.fillRoundRect(COUNTER_MINUS_X,COUNTER_BUTTON_Y,COUNTER_BUTTON_W,
        COUNTER_BUTTON_H,12,enabled && state.draft>0 ? COL_RED : COL_DARK_BUTTON);
    lcd.fillRoundRect(COUNTER_PLUS_X,COUNTER_BUTTON_Y,COUNTER_BUTTON_W,
        COUNTER_BUTTON_H,12,enabled && state.draft<LLONG_MAX ? COL_GREEN : COL_DARK_BUTTON);
    const int cy=COUNTER_BUTTON_Y+COUNTER_BUTTON_H/2;
    const int mx=COUNTER_MINUS_X+COUNTER_BUTTON_W/2;
    const int px=COUNTER_PLUS_X+COUNTER_BUTTON_W/2;
    lcd.fillRect(mx-14,cy-3,28,6,TFT_WHITE);
    lcd.fillRect(px-14,cy-3,28,6,TFT_WHITE);
    lcd.fillRect(px-3,cy-14,6,28,TFT_WHITE);
    lcd.fillRoundRect(COUNTER_VALUE_X,COUNTER_BUTTON_Y,COUNTER_VALUE_W,
        COUNTER_BUTTON_H,12,COL_CARD);
    lcd.drawRoundRect(COUNTER_VALUE_X,COUNTER_BUTTON_Y,COUNTER_VALUE_W,
        COUNTER_BUTTON_H,12,dirty ? COL_ORANGE : COL_BORDER);
    char number[32]={};
    snprintf(number,sizeof(number),"%lld",state.draft);
    String value;
    if (!sqlTaskHandle) value="Config. DB";
    else if (sqlBusy && sqlBusyIndex==index) value="Attendere...";
    else if (state.valid) value=number;
    else if (!wifi) value="Offline";
    else if (state.failed) value="Errore DB";
    else value="Lettura...";
    lcd.setFont(&fonts::Font4); lcd.setTextSize(1.0f);
    const int width=lcd.textWidth(value);
    float scale=1.5f;
    if (width>0 && width*scale>COUNTER_VALUE_W-16)
        scale=static_cast<float>(COUNTER_VALUE_W-16)/width;
    drawTextCentered(COUNTER_VALUE_X,COUNTER_BUTTON_Y,COUNTER_VALUE_W,
        COUNTER_BUTTON_H,value,state.failed ? COL_RED : COL_TEXT,&fonts::Font4,scale);
    lcd.fillRoundRect(CANCEL_X,CONFIRM_Y,CONFIRM_W,CONFIRM_H,10,
        enabled && dirty ? COL_RED : COL_DARK_BUTTON);
    drawTextCentered(CANCEL_X,CONFIRM_Y,CONFIRM_W,CONFIRM_H,
        "Annulla",COL_TEXT,&fonts::Font2,1.2f);
    lcd.fillRoundRect(CONFIRM_X,CONFIRM_Y,CONFIRM_W,CONFIRM_H,10,
        enabled && dirty && wifi ? COL_GREEN : COL_DARK_BUTTON);
    drawTextCentered(CONFIRM_X,CONFIRM_Y,CONFIRM_W,CONFIRM_H,
        !wifi ? "Offline" : "Conferma",COL_TEXT,&fonts::Font2,1.2f);
}

void loop()
{
  updateConnectionDisplay();
  handleTouch();
  updateDatabaseLoop();
  delay(5);
  
}