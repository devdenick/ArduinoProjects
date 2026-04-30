#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <driver/spi_master.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <elapsedMillis.h>
#include "BufferGUI.h"

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_XPT2046 _touch_instance;

public:
  LGFX(void) {
    { // SPI Bus
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 14;
      cfg.pin_mosi = 13;
      cfg.pin_miso = 12;
      cfg.pin_dc   = 2;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
 
    { // TFT Panel
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 15;
      cfg.pin_rst          = -1;
      cfg.pin_busy         = -1;
      cfg.memory_width     = 240;
      cfg.memory_height    = 320;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;//5
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;//false;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }
 
    { // Backlight (optional)
      auto cfg = _light_instance.config();
      cfg.pin_bl = 21;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
 
    { // XPT2046 Touchscreen
      auto cfg = _touch_instance.config();
      cfg.x_min      = 652;//222;
      cfg.x_max      = 3620;//3367;
      cfg.y_min      = 350;//192;
      cfg.y_max      = 3750;//3732;
      cfg.pin_int    = -1;
      cfg.bus_shared = true;
      cfg.offset_rotation = 4; //6;
      cfg.spi_host   = SPI3_HOST;
      cfg.freq       = 1000000;
      cfg.pin_sclk   = 25; //14; DCLK
      cfg.pin_mosi   = 32; //13; DIN
      cfg.pin_miso   = 39; //12; DOUT
      cfg.pin_cs     = 33; //33; /CS
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
 
    setPanel(&_panel_instance);
  }
};

LGFX lcd;
LGFX_Sprite tableSprite(&lcd);
LGFX_Sprite waitingPanelSprite(&lcd);

bool lcdCleared = true;

const unsigned long MIN_TOUCH_SELECT_MS = 400;
const unsigned long WAITING_PANEL_REFRESH_MS = 300;
const unsigned long WAITING_UPDATE_REQUEST_MS = 3000;

elapsedMillis touchTimer;
elapsedMillis waitingPanelTimer;
elapsedMillis waitingUpdateRequestTimer;
bool touchHold = false;
int prevY;
BufferGUI::SwipeState prevSwipeState = BufferGUI::STILL;

BufferGUI::TableRow tableRows[BufferGUI::MAX_ROWS];
int rowsCount = 0;

//wifi
WiFiClient wifiClient;
const char* ssid = "ZFIOT";
const char* password = "CErrueGQzWESPAaAL6jetewg";//OSTI00048 psw CErrueGQzWESPAaAL6jetewg, Funzionante GwGSXud3jbgfjWuxdXiaKc6S
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

void setupWiFiConnection();
void setupMqttCommunication();
void mqttVerifyConnection();
void publishNotifyConnected();
void handleMqttMessages();
bool loadTableRowsFromJson(const String& json);
void safeCopy(char* dest, size_t destSize, const char* source);

int selectedCard = -1;
BufferGUI::SwipeState swipeState;
bool swiped = false;

void setup()
{
  Serial.begin(9600);
  delay(500);

  lcd.init();
  lcd.setRotation(0);     // prova 0, 1, 2, 3 se lo schermo e' girato
  lcd.setBrightness(255); // 0-255
  
  lcd.fillScreen(TFT_BLACK);

  BufferGUI::begin(lcd);
  // WiFi disconnesso inizialmente
  BufferGUI::drawHeader(lcd, false, false);

  waitingPanelSprite.setColorDepth(16);
  bool ok = waitingPanelSprite.createSprite(BufferGUI::WAITING_PANEL_WIDTH, BufferGUI::WAITING_PANEL_HEIGHT);

  //BufferGUI::initTable(tableRows);
  tableSprite.setColorDepth(16);
  ok = tableSprite.createSprite(BufferGUI::TABLE_AREA_W, BufferGUI::TABLE_AREA_H);
  BufferGUI::clearTableArea(lcd);

  Serial.println("LovyanGFX avviato.");

  // CONNESSIONE WIFI
  setupWiFiConnection();
  //INIZIALIZZAZIONE COMUNICAZIONE MQTT
  setupMqttCommunication();
  
  BufferGUI::drawHeader(lcd, wifiConnected, mqttConnected);
  
  //BufferGUI::drawTableSprite(tableRows, tableSprite, rowsCount, -1);
}

void loop()
{
  mqttVerifyConnection();
  handleMqttMessages();
  int x, y;
  bool isTouched = lcd.getTouch(&x, &y);

  if(listUpdated){
    if (isTouched)
    {
      if (!touchHold)
      {
        touchHold = true;
        prevY = y;
        touchTimer = 0;
        return;
      }

      selectedCard = BufferGUI::tableRowHitbox(tableRows, x, y, rowsCount);

      swipeState = BufferGUI::swipe(prevY, y);

      if (swipeState != BufferGUI::STILL)
      {
        swiped = true;
        BufferGUI::swipeTable(tableRows, swipeState, rowsCount);
        BufferGUI::drawTableSprite(tableRows, tableSprite, rowsCount, -1);
      }
      else
        BufferGUI::drawTableSprite(tableRows, tableSprite, rowsCount, selectedCard);
      

      //prevY = y;
    }
    else
    {
      if(touchHold){
        bool longEnough = touchTimer >= MIN_TOUCH_SELECT_MS;
        BufferGUI::drawTableSprite(tableRows, tableSprite, rowsCount, -1);//reset selected card
        if(!swiped && longEnough){
          publishCardSelected(selectedCard);
          listUpdated = false;
          BufferGUI::clearTableArea(lcd);
        }
        //BufferGUI::drawTableSprite(tableRows, tableSprite, rowsCount, -1);//reset selected card
      }
      swiped = false;
      touchHold = false;
    }
  }
  else {
    if (waitingPanelTimer >= WAITING_PANEL_REFRESH_MS) {
      waitingPanelTimer = 0;
      BufferGUI::drawWaitingPanel(waitingPanelSprite);
    }

    if (waitingUpdateRequestTimer >= WAITING_UPDATE_REQUEST_MS) {
      waitingUpdateRequestTimer = 0;

      if (mqttClient.connected()) {
        publishNotifyConnected();
      }
    }
  }
}

void setupWiFiConnection(){
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  //wificonnesso
  Serial.print("- IP address: ");
  Serial.println(WiFi.localIP());
  wifiConnected = true;
}

void setupMqttCommunication(){
  Serial.print("MQTT init connection");
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }else{
    mqttConnected = true;
    Serial.println("You're connected to the MQTT broker!");
    Serial.print("Iscrizione al topic ");
    Serial.println(readListTopic);
    mqttClient.subscribe(readListTopic);
    publishNotifyConnected();
  }
}

void mqttVerifyConnection(){
  if(!mqttClient.connected()){
    Serial.println("BROKER NON CONNESSO - TENTATIVO RICONNESSIONE");
    mqttConnected = false;
    BufferGUI::drawHeader(lcd, wifiConnected, mqttConnected);
    listUpdated = false;//broker disconnesso pericolo disallineamento dati devo richiederli
    if (!mqttClient.connect(broker, port)) {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.connectError());
    }
    else{
      Serial.println("BROKER RICONNESSO");    
      mqttClient.subscribe(readListTopic);
      mqttConnected = true;
      BufferGUI::drawHeader(lcd, wifiConnected, mqttConnected);
      publishNotifyConnected();
    }
  }
}

void publishNotifyConnected(){
  mqttClient.beginMessage(notifyTopic, false, 1);
  mqttClient.print("connected");
  mqttClient.endMessage();
}

void publishCardSelected(int selectedCard){
  if (selectedCard < 0 || selectedCard >= rowsCount) {
    return;
  }
  mqttClient.beginMessage(selectTopic, false, 1);
  mqttClient.print(tableRows[selectedCard].id);
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

  Serial.println("----- MQTT MESSAGE RECEIVED -----");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(payload);

  if (topic == readListTopic) {
    bool ok = loadTableRowsFromJson(payload, BufferGUI::START_TABLE_X, BufferGUI::START_TABLE_Y, BufferGUI::TABLE_ROW_HEIGHT);

    if (ok) {
      Serial.println("JSON letto correttamente. Tabella aggiornata.");

      listUpdated = true;
      BufferGUI::clearWaitingPanel(waitingPanelSprite);
      BufferGUI::drawTableSprite(tableRows, tableSprite, rowsCount, -1);
    } else {
      Serial.println("Errore lettura JSON.");
    }
  }
}

bool loadTableRowsFromJson(const String& json, int startTableX, int startTableY, int tableRowHeight)
{
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonArray articoli = doc["articoli"].as<JsonArray>();

  if (articoli.isNull()) {
    Serial.println("Campo 'articoli' non trovato nel JSON.");
    return false;
  }

  BufferGUI::clearTableRowsData(tableRows);

  int index = 0;

  for (JsonObject articolo : articoli) {
    if (index >= BufferGUI::MAX_ROWS) {
      Serial.println("Raggiunto numero massimo righe tabella.");
      break;
    }

    tableRows[index].x = startTableX;
    tableRows[index].y = startTableY + (index * tableRowHeight);
    Serial.println(tableRows[index].y);
    tableRows[index].id = articolo["id"] | -1;

    const char* nome = articolo["nome"] | "";
    const char* startTimestamp = articolo["startTimestamp"] | "";

    safeCopy(tableRows[index].nome, sizeof(tableRows[index].nome), nome);
    safeCopy(tableRows[index].startTimestamp, sizeof(tableRows[index].startTimestamp), startTimestamp);

    Serial.print("Riga ");
    Serial.print(index);
    Serial.print(" -> id: ");
    Serial.print(tableRows[index].id);
    Serial.print(", nome: ");
    Serial.print(tableRows[index].nome);
    Serial.print(", startTimestamp: ");
    Serial.println(tableRows[index].startTimestamp);

    index++;
  }

  rowsCount = index;

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

