#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "BufferGUI.h"

namespace BufferGUI
{

  //ESP32 240x320 2.8 screen
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

  // Palette colori
  static constexpr uint16_t UI_COLOR_BG            = TFT_BLACK;
  static constexpr uint16_t UI_COLOR_HEADER        = 0x2104; // grigio/blu scuro visibile
  static constexpr uint16_t UI_COLOR_PANEL         = 0x1082; // pannello scuro
  static constexpr uint16_t UI_COLOR_CARD          = 0x2945; // card pari
  static constexpr uint16_t UI_COLOR_CARD_ALT      = 0x3186; // card dispari
  static constexpr uint16_t UI_COLOR_CARD_SELECTED = 0x7BEF; //card selezionata
  static constexpr uint16_t UI_COLOR_CARD_SELECTED_LONG_ENOUGH = TFT_DARKGREEN; //card selezionata
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
    char linea;
    char lotto[32];
  } TableRow;


}