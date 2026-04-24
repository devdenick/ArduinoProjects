#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>

namespace BufferGUI
{
  /////////////////////////////////////////////////////
  ///////////////////UI THEME//////////////////////////
  /////////////////////////////////////////////////////

  // Palette RGB565 semplice e sicura: niente colori troppo scuri su nero puro.
  static constexpr uint16_t UI_COLOR_BG          = TFT_BLACK;
  static constexpr uint16_t UI_COLOR_HEADER      = 0x2104; // grigio/blu scuro visibile
  static constexpr uint16_t UI_COLOR_PANEL       = 0x1082; // pannello scuro
  static constexpr uint16_t UI_COLOR_CARD        = 0x2945; // card pari
  static constexpr uint16_t UI_COLOR_CARD_ALT    = 0x3186; // card dispari
  static constexpr uint16_t UI_COLOR_BORDER      = 0x6B4D; // bordo grigio visibile
  static constexpr uint16_t UI_COLOR_TEXT        = TFT_WHITE;
  static constexpr uint16_t UI_COLOR_TEXT_MUTED  = 0xBDF7;
  static constexpr uint16_t UI_COLOR_PRIMARY     = 0x04FF; // cyan
  static constexpr uint16_t UI_COLOR_SHADOW      = TFT_BLACK;
  static constexpr uint16_t UI_COLOR_SUCCESS     = TFT_GREEN;
  static constexpr uint16_t UI_COLOR_DANGER      = TFT_RED;

  /////////////////////////////////////////////////////
  ///////////////////WIFI SPRITE///////////////////////
  /////////////////////////////////////////////////////

  enum class GuiSpriteId : uint8_t
  {
    WifiConnected = 0,
    WifiDisconnected = 1,

    Count
  };

  struct GuiBitmap16
  {
    const uint16_t* data;
    int16_t width;
    int16_t height;
  };

  static constexpr int16_t WIFI_ICON_W = 16;
  static constexpr int16_t WIFI_ICON_H = 16;

  #define G TFT_GREEN
  #define R TFT_RED
  #define W TFT_WHITE

  // WIFI CONNECTED: sfondo verde, icona bianca.
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

  // WIFI DISCONNECTED: sfondo rosso, icona bianca.
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

  static const GuiBitmap16 BITMAPS[] =
  {
    { WIFI_ICON_16X16_DATA,              WIFI_ICON_W, WIFI_ICON_H },
    { WIFI_ICON_16X16_DATA_DISCONNECTED, WIFI_ICON_W, WIFI_ICON_H }
  };

  /////////////////////////////////////////////////////
  ///////////////////GESTURES//////////////////////////
  /////////////////////////////////////////////////////

  enum SwipeState
  {
    UP,
    DOWN,
    STILL
  };

  /////////////////////////////////////////////////////
  ///////////////////TABLE/////////////////////////////
  /////////////////////////////////////////////////////

  static const int MAX_ROWS = 50;

  // Stessa dimensione sprite che avevi prima: 200 x 260.
  // Ho solo spostato la tabella sotto la top bar per non coprirla.
  static const int TABLE_AREA_X = 20;
  static const int TABLE_AREA_Y = 50;
  static const int TABLE_AREA_W = 200;
  static const int TABLE_AREA_H = 260;

  static const int START_TABLE_X = TABLE_AREA_X;
  static const int START_TABLE_Y = TABLE_AREA_Y;

  static const int TABLE_ROW_WIDTH = TABLE_AREA_W;
  static const int TABLE_ROW_HEIGHT = 60;

  static const int SWIPE_MOVE = 15;

  typedef struct {
    int x;
    int y;
    char value[32];
  } TableRow;
}
