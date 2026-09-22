#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <elapsedMillis.h>
#include "BufferGUI.h"
#include <ElegantOTA.h>
#include <WebServer.h>

BufferGUI::LGFX lcd;


const unsigned long MIN_TOUCH_SELECT_MS = 400;
const unsigned long WAITING_PANEL_REFRESH_MS = 300;
const unsigned long WAITING_UPDATE_REQUEST_MS = 3000;

elapsedMillis touchTimer;
elapsedMillis waitingPanelTimer;
elapsedMillis waitingUpdateRequestTimer;
bool touchHold = false;
int prevY = 0;
int touchStartY = 0;
int pendingScrollDelta = 0;
unsigned long lastSwipeFrameMs = 0;

// Il pannello fisico lavora a circa 39 Hz con PCLK 16 MHz e i timing correnti.
// 25 ms evita di ridisegnare più velocemente del refresh reale.
const unsigned long SWIPE_FRAME_INTERVAL_MS = 25;
const int DRAG_START_THRESHOLD_PX = 7;

BufferGUI::TableRow tableRows[BufferGUI::MAX_ROWS];
int rowsCount = 0;

//wifi
WiFiClient wifiClient;
const char* ssid = "ZFIOT";
const char* password = "GwGSXud3jbgfjWuxdXiaKc6S";//OSTI00048 psw CErrueGQzWESPAaAL6jetewg, Funzionante GwGSXud3jbgfjWuxdXiaKc6S
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

const unsigned long MQTT_RECONNECT_INTERVAL_MS = 3000;
unsigned long lastMqttReconnectAttemptMs = 0;

void setupWiFiConnection();
void setupMqttCommunication();
void mqttVerifyConnection();
void publishNotifyConnected();
void handleMqttMessages();
bool loadTableRowsFromJson(const String& json, int startTableX, int startTableY, int tableRowHeight);
void safeCopy(char* dest, size_t destSize, const char* source);

int selectedCard = -1;
bool swiped = false;

// Stato grafico del long-press. Durante lo swipe non viene ridisegnata
// l'intera tabella: si sposta direttamente il framebuffer.
int lastRenderedSelectedCard = -1;
bool lastRenderedLongEnough = false;

int cobotMission = 400;

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

void setup()
{
  Serial.begin(9600);
  delay(500);

  lcd.init();
  lcd.setRotation(3);     // prova 0, 1, 2, 3 se lo schermo e' girato
  lcd.setBrightness(255); // 0-255
  
  lcd.fillScreen(TFT_BLACK);

  BufferGUI::begin(lcd);
  // WiFi disconnesso inizialmente
  BufferGUI::drawHeader(lcd, false, false, macAddress, cobotMission);

  // ------------------------------------------------------
  // MEMORIA GRAFICA
  // ------------------------------------------------------
  // Il framebuffer RGB del pannello è già in PSRAM.
  // Non creiamo più uno sprite 440x700: in portrait il push di uno
  // sprite così grande richiede una rotazione software pixel-per-pixel.
  // Rimangono soltanto i piccoli sprite 16x16 delle icone, in RAM interna.

  Serial.print("PSRAM totale: ");
  Serial.print(ESP.getPsramSize() / 1024);
  Serial.println(" KB");

  Serial.print("PSRAM libera: ");
  Serial.print(ESP.getFreePsram() / 1024);
  Serial.println(" KB");

  BufferGUI::clearTableArea(lcd);

  Serial.println("LovyanGFX avviato.");

  // CONNESSIONE WIFI
  setupWiFiConnection();
  //INIZIALIZZAZIONE COMUNICAZIONE MQTT
  setupMqttCommunication();
  
  BufferGUI::drawHeader(lcd, wifiConnected, mqttConnected, macAddress, cobotMission);
  
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

void loop()
{
  server.handleClient();
  ElegantOTA.loop();

  if (otaUpdate)
    return;

  mqttVerifyConnection();
  handleMqttMessages();

  int x, y;
  const bool isTouched = lcd.getTouch(&x, &y);

  if (listUpdated)
  {
    if (isTouched)
    {
      // ------------------------------------------------------
      // INIZIO TOUCH
      // ------------------------------------------------------
      if (!touchHold)
      {
        touchHold = true;
        swiped = false;
        prevY = y;
        touchStartY = y;
        pendingScrollDelta = 0;
        lastSwipeFrameMs = millis();
        touchTimer = 0;

        selectedCard = BufferGUI::tableRowHitbox(tableRows, x, y, rowsCount);
        lastRenderedSelectedCard = -1;
        lastRenderedLongEnough = false;

        return;
      }

      // ------------------------------------------------------
      // TOUCH TENUTO / DRAG
      // ------------------------------------------------------
      const int fingerDelta = y - prevY;
      prevY = y;

      if (!swiped && abs(y - touchStartY) >= DRAG_START_THRESHOLD_PX)
      {
        // Appena il gesto diventa uno swipe togliamo un eventuale highlight
        // da long press e passiamo alla modalità drag.
        if (lastRenderedSelectedCard >= 0)
        {
          BufferGUI::redrawTableRow(
            lcd,
            tableRows,
            rowsCount,
            lastRenderedSelectedCard,
            false,
            false
          );
        }

        swiped = true;
        selectedCard = -1;
        lastRenderedSelectedCard = -1;
        lastRenderedLongEnough = false;
      }

      if (swiped)
      {
        pendingScrollDelta += fingerDelta;

        // Aggiorniamo al massimo ~40 frame/s, coerente con il refresh fisico
        // del pannello. Più chiamate causerebbero solo più traffico PSRAM.
        if (millis() - lastSwipeFrameMs >= SWIPE_FRAME_INTERVAL_MS)
        {
          if (pendingScrollDelta != 0)
          {
            BufferGUI::scrollTableByPixels(
              lcd,
              tableRows,
              rowsCount,
              pendingScrollDelta
            );
            pendingScrollDelta = 0;
          }

          lastSwipeFrameMs = millis();
        }

        return;
      }

      // ------------------------------------------------------
      // LONG PRESS: evidenzia solo dopo la soglia, non al primo touch.
      // In questo modo un normale swipe non provoca un redraw enorme.
      // ------------------------------------------------------
      const bool longEnough = touchTimer >= MIN_TOUCH_SELECT_MS;

      if (longEnough && selectedCard >= 0 && !lastRenderedLongEnough)
      {
        BufferGUI::redrawTableRow(
          lcd,
          tableRows,
          rowsCount,
          selectedCard,
          true,
          true
        );

        lastRenderedSelectedCard = selectedCard;
        lastRenderedLongEnough = true;
      }
    }
    else
    {
      // ------------------------------------------------------
      // RILASCIO TOUCH
      // ------------------------------------------------------
      if (touchHold)
      {
        if (swiped)
        {
          // Applica anche l'ultimo pezzetto di movimento accumulato.
          if (pendingScrollDelta != 0)
          {
            BufferGUI::scrollTableByPixels(
              lcd,
              tableRows,
              rowsCount,
              pendingScrollDelta
            );
          }
        }
        else
        {
          const bool longEnough = touchTimer >= MIN_TOUCH_SELECT_MS;

          if (longEnough && selectedCard >= 0)
          {
            publishCardSelected(selectedCard);
            listUpdated = false;
            BufferGUI::clearTableArea(lcd);
          }
          else if (lastRenderedSelectedCard >= 0)
          {
            // Rimuove solo l'highlight della singola riga.
            BufferGUI::redrawTableRow(
              lcd,
              tableRows,
              rowsCount,
              lastRenderedSelectedCard,
              false,
              false
            );
          }
        }
      }

      swiped = false;
      touchHold = false;
      selectedCard = -1;
      lastRenderedSelectedCard = -1;
      lastRenderedLongEnough = false;
      pendingScrollDelta = 0;
    }
  }
  else
  {
    if (waitingPanelTimer >= WAITING_PANEL_REFRESH_MS)
    {
      waitingPanelTimer = 0;
      BufferGUI::drawWaitingPanel(lcd, "WAITING BROKER");
    }

    if (waitingUpdateRequestTimer >= WAITING_UPDATE_REQUEST_MS)
    {
      waitingUpdateRequestTimer = 0;

      if (mqttClient.connected())
        publishNotifyConnected();
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
  macAddress = WiFi.macAddress();
  Serial.println(macAddress);
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

void mqttVerifyConnection()
{
  // Stato già OK.
  if (mqttClient.connected())
  {
    if (!mqttConnected)
    {
      mqttConnected = true;
      BufferGUI::drawHeader(lcd, wifiConnected, mqttConnected, macAddress, cobotMission);
    }
    return;
  }

  // Segnala la disconnessione una sola volta, non ad ogni loop.
  if (mqttConnected)
  {
    mqttConnected = false;
    listUpdated = false;
    BufferGUI::drawHeader(lcd, wifiConnected, mqttConnected, macAddress, cobotMission);
    BufferGUI::clearTableArea(lcd);
  }

  // Non martellare broker e display centinaia di volte al secondo.
  if (millis() - lastMqttReconnectAttemptMs < MQTT_RECONNECT_INTERVAL_MS)
    return;

  lastMqttReconnectAttemptMs = millis();
  Serial.println("BROKER NON CONNESSO - TENTATIVO RICONNESSIONE");

  if (!mqttClient.connect(broker, port))
  {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    return;
  }

  Serial.println("BROKER RICONNESSO");
  mqttClient.subscribe(readListTopic);
  mqttConnected = true;
  BufferGUI::drawHeader(lcd, wifiConnected, mqttConnected, macAddress, cobotMission);
  publishNotifyConnected();
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

  String payload = String(tableRows[selectedCard].id) + ":" + String(cobotMission) + ":" +tableRows[selectedCard].linea;
  
  mqttClient.beginMessage(selectTopic, false, 1);
  mqttClient.print(payload);
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
      BufferGUI::clearWaitingPanel(lcd);
      BufferGUI::drawTable(lcd, tableRows, rowsCount, -1, false);
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
    const char* linea = articolo["linea"] | "Z";
    tableRows[index].linea = linea[0];
    const char* lotto = articolo["lotto"] | "";
    if (strlen(lotto) > 0)
      snprintf(tableRows[index].lotto, sizeof(tableRows[index].lotto), "Lotto: %s", lotto);
    else
      snprintf(tableRows[index].lotto, sizeof(tableRows[index].lotto), "Lotto assente");

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
    Serial.print(", linea: ");
    Serial.print(tableRows[index].linea);
    Serial.print(", lotto: ");
    Serial.print(lotto);
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

