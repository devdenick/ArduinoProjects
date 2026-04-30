#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "BufferGUI.h"

namespace BufferGUI
{

  // Palette colori
  static constexpr uint16_t UI_COLOR_BG            = TFT_BLACK;
  static constexpr uint16_t UI_COLOR_HEADER        = 0x2104; // grigio/blu scuro visibile
  static constexpr uint16_t UI_COLOR_PANEL         = 0x1082; // pannello scuro
  static constexpr uint16_t UI_COLOR_CARD          = 0x2945; // card pari
  static constexpr uint16_t UI_COLOR_CARD_ALT      = 0x3186; // card dispari
  static constexpr uint16_t UI_COLOR_CARD_SELECTED = 0x7BEF; //card selezionata
  static constexpr uint16_t UI_COLOR_BORDER        = 0x6B4D; // bordo grigio visibile
  static constexpr uint16_t UI_COLOR_TEXT          = TFT_WHITE;
  static constexpr uint16_t UI_COLOR_TEXT_MUTED    = 0xBDF7;
  static constexpr uint16_t UI_COLOR_PRIMARY       = TFT_ORANGE; 
  static constexpr uint16_t UI_COLOR_SUCCESS       = TFT_GREEN;
  static constexpr uint16_t UI_COLOR_DANGER        = TFT_RED;

  struct GuiBitmap16
  {
    const uint16_t* data;
    int16_t width;
    int16_t height;
  };

  static constexpr int16_t WIFI_ICON_W = 16;
  static constexpr int16_t WIFI_ICON_H = 16;

  #define G UI_COLOR_SUCCESS
  #define R TFT_RED
  #define W TFT_WHITE

  // =========================
  // WIFI CONNECTED
  // Sfondo verde, icona bianca
  // =========================
  static const uint16_t WIFI_ICON_16X16_DATA[WIFI_ICON_W * WIFI_ICON_H] PROGMEM =
  {
    G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,
    G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,
    G,G,G,G,W,W,W,W,W,W,W,W,G,G,G,G,
    G,G,G,W,W,G,G,G,G,G,G,W,W,G,G,G,
    G,G,W,W,G,G,G,G,G,G,G,G,W,W,G,G,
    G,W,W,G,G,G,G,G,G,G,G,G,G,W,W,G,
    G,G,G,G,G,W,W,W,W,W,W,G,G,G,G,G,
    G,G,G,G,W,W,G,G,G,G,W,W,G,G,G,G,
    G,G,G,G,G,G,W,W,W,W,G,G,G,G,G,G,
    G,G,G,G,G,W,W,G,G,W,W,G,G,G,G,G,
    G,G,G,G,G,G,G,W,W,G,G,G,G,G,G,G,
    G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,
    G,G,G,G,G,G,G,W,W,G,G,G,G,G,G,G,
    G,G,G,G,G,G,G,W,W,G,G,G,G,G,G,G,
    G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,
    G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G
  };

  // =========================
  // WIFI DISCONNECTED
  // Sfondo rosso, icona bianca
  // =========================
  static const uint16_t WIFI_ICON_16X16_DATA_DISCONNECTED[WIFI_ICON_W * WIFI_ICON_H] PROGMEM =
  {
    R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,
    R,R,R,R,W,W,W,W,W,W,W,W,R,R,R,R,
    R,R,R,W,W,R,R,R,R,R,R,W,W,R,R,R,
    R,R,W,W,R,R,R,R,R,R,R,R,W,W,R,R,
    R,W,W,R,R,R,R,R,R,R,R,R,R,W,W,R,
    R,R,R,R,R,W,W,W,W,W,W,R,R,R,R,R,
    R,R,R,R,W,W,R,R,R,R,W,W,R,R,R,R,
    R,R,R,R,R,R,W,W,W,W,R,R,R,R,R,R,
    R,R,R,R,R,W,W,R,R,W,W,R,R,R,R,R,
    R,R,R,R,R,R,R,W,W,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,W,W,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,W,W,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,
    R,R,R,R,R,R,R,R,R,R,R,R,R,R,R,R
  };

  #undef G
  #undef R
  #undef W

  static constexpr int16_t MQTT_ICON_W = 16;
  static constexpr int16_t MQTT_ICON_H = 16;

  #define MG UI_COLOR_SUCCESS
  #define MR TFT_RED
  #define MW TFT_WHITE

  // =========================
  // MQTT CONNECTED
  // Sfondo verde, lettera M bianca
  // =========================
  static const uint16_t MQTT_ICON_16X16_DATA[MQTT_ICON_W * MQTT_ICON_H] PROGMEM =
  {
    MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,
    MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,

    MG,MG,MG,MW,MW,MG,MG,MG,MG,MG,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MW,MG,MG,MG,MG,MW,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MW,MG,MG,MW,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MG,MW,MW,MG,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MG,MW,MW,MG,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MG,MG,MG,MG,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MG,MG,MG,MG,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MG,MG,MG,MG,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MG,MG,MG,MG,MG,MW,MW,MG,MG,MG,
    MG,MG,MG,MW,MW,MG,MG,MG,MG,MG,MG,MW,MW,MG,MG,MG,

    MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,
    MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,
    MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,
    MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG,MG
  };

  // =========================
  // MQTT DISCONNECTED
  // Sfondo rosso, lettera M bianca
  // =========================
  static const uint16_t MQTT_ICON_16X16_DATA_DISCONNECTED[MQTT_ICON_W * MQTT_ICON_H] PROGMEM =
  {
    MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,
    MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,

    MR,MR,MR,MW,MW,MR,MR,MR,MR,MR,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MW,MR,MR,MR,MR,MW,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MW,MR,MR,MW,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MR,MW,MW,MR,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MR,MW,MW,MR,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MR,MR,MR,MR,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MR,MR,MR,MR,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MR,MR,MR,MR,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MR,MR,MR,MR,MR,MW,MW,MR,MR,MR,
    MR,MR,MR,MW,MW,MR,MR,MR,MR,MR,MR,MW,MW,MR,MR,MR,

    MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,
    MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,
    MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,
    MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR,MR
  };

  #undef MG
  #undef MR
  #undef MW

  static const GuiBitmap16 BITMAPS[] =
  {
    {
      WIFI_ICON_16X16_DATA,
      WIFI_ICON_W,
      WIFI_ICON_H
    },
    {
      WIFI_ICON_16X16_DATA_DISCONNECTED,
      WIFI_ICON_W,
      WIFI_ICON_H
    },
    {
      MQTT_ICON_16X16_DATA,
      MQTT_ICON_W,
      MQTT_ICON_H
    },
    {
      MQTT_ICON_16X16_DATA_DISCONNECTED,
      MQTT_ICON_W,
      MQTT_ICON_H
    }
  };

  enum SwipeState
  {
    UP,
    DOWN,
    STILL
  };

  static const int HEADER_HEIGHT = 38;

  static const int MAX_ROWS = 50;

  static const int TABLE_AREA_X = 20;
  static const int TABLE_AREA_Y = 60;
  static const int TABLE_AREA_W = 200;
  static const int TABLE_AREA_H = 260;

  static const int START_TABLE_X = TABLE_AREA_X;
  static const int START_TABLE_Y = TABLE_AREA_Y;

  static const int TABLE_ROW_WIDTH = TABLE_AREA_W;
  static const int TABLE_ROW_HEIGHT = 60;

  static const int WAITING_PANEL_X = 5;
  static const int WAITING_PANEL_Y = 60;
  static const int WAITING_PANEL_WIDTH = 230;
  static const int WAITING_PANEL_HEIGHT = 100;
  static const int WAITING_PANEL_BOX_MARGIN = 10;
  static const int WAITING_PANEL_BOX_WIDTH = 210;
  static const int WAITING_PANEL_BOX_HEIGHT = 80;
  static int waitingCounter = 0;


  static const int SWIPE_SENS = 20;
  static const int SWIPE_MOVE = 15;

  typedef struct {
    int x;
    int y;
    char nome[32];
    char startTimestamp[32];
    int id;
  } TableRow;


}