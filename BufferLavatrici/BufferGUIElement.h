#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include "BufferGUI.h"

namespace BufferGUI
{

class LGFX : public lgfx::LGFX_Device
{
public:

  lgfx::Bus_RGB       _bus_instance;
  lgfx::Panel_RGB     _panel_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_FT5x06  _touch_instance;


  LGFX(void)
  {
    // ======================================================
    // PANEL
    // ======================================================

    {
      auto cfg = _panel_instance.config();

      cfg.memory_width  = 800;
      cfg.memory_height = 480;

      cfg.panel_width   = 800;
      cfg.panel_height  = 480;

      cfg.offset_x = 0;
      cfg.offset_y = 0;

      cfg.pin_cs   = -1;
      cfg.pin_rst  = -1;
      cfg.pin_busy = -1;

      // RGB panel
      cfg.readable   = true;
      cfg.invert     = false;
      cfg.rgb_order  = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
    }


    // ======================================================
    // FRAMEBUFFER IN PSRAM
    // ======================================================

    {
      auto cfg =
        _panel_instance.config_detail();

      // 2 = PSRAM only. Nella versione attuale di LovyanGFX il Bus_RGB
      // alloca comunque il framebuffer principale in PSRAM; lo lasciamo
      // esplicito per documentare l'intento.
      cfg.use_psram = 2;

      _panel_instance.config_detail(cfg);
    }


    // ======================================================
    // RGB BUS
    // ======================================================

    {
      auto cfg = _bus_instance.config();

      cfg.panel = &_panel_instance;

      // ----------------------------------------------------
      // BLUE
      // ----------------------------------------------------

      cfg.pin_d0 = GPIO_NUM_8;   // B0
      cfg.pin_d1 = GPIO_NUM_3;   // B1
      cfg.pin_d2 = GPIO_NUM_46;  // B2
      cfg.pin_d3 = GPIO_NUM_9;   // B3
      cfg.pin_d4 = GPIO_NUM_1;   // B4

      // ----------------------------------------------------
      // GREEN
      // ----------------------------------------------------

      cfg.pin_d5  = GPIO_NUM_5;   // G0
      cfg.pin_d6  = GPIO_NUM_6;   // G1
      cfg.pin_d7  = GPIO_NUM_7;   // G2
      cfg.pin_d8  = GPIO_NUM_15;  // G3
      cfg.pin_d9  = GPIO_NUM_16;  // G4
      cfg.pin_d10 = GPIO_NUM_4;   // G5

      // ----------------------------------------------------
      // RED
      // ----------------------------------------------------

      cfg.pin_d11 = GPIO_NUM_45;  // R0
      cfg.pin_d12 = GPIO_NUM_48;  // R1
      cfg.pin_d13 = GPIO_NUM_47;  // R2
      cfg.pin_d14 = GPIO_NUM_21;  // R3
      cfg.pin_d15 = GPIO_NUM_14;  // R4

      // ----------------------------------------------------
      // CONTROL SIGNALS
      // ----------------------------------------------------

      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_henable = GPIO_NUM_40; // DE
      cfg.pin_pclk    = GPIO_NUM_42;

      // ----------------------------------------------------
      // PIXEL CLOCK
      //
      // Configurazione ufficiale 4D Systems per 43CT
      // ----------------------------------------------------

      cfg.freq_write = 16000000;

      // ----------------------------------------------------
      // HORIZONTAL TIMING
      // ----------------------------------------------------

      cfg.hsync_polarity    = 0;

      cfg.hsync_front_porch = 8;
      cfg.hsync_pulse_width = 4;
      cfg.hsync_back_porch  = 8;

      // ----------------------------------------------------
      // VERTICAL TIMING
      // ----------------------------------------------------

      cfg.vsync_polarity    = 0;

      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 4;
      cfg.vsync_back_porch  = 8;

      // ----------------------------------------------------
      // PCLK
      //
      // 4D utilizza il dato sul falling edge.
      // ----------------------------------------------------

      cfg.pclk_active_neg = true;
      cfg.pclk_idle_high  = true;

      cfg.de_idle_high = false;

      _bus_instance.config(cfg);
    }

    _panel_instance.setBus(&_bus_instance);


    // ======================================================
    // BACKLIGHT
    // ======================================================

    {
      auto cfg =
        _light_instance.config();

      // Backlight ufficiale gen4 RGB
      cfg.pin_bl = GPIO_NUM_2;

      cfg.invert = false;

      // La libreria 4D usa 25 kHz
      cfg.freq = 25000;

      cfg.pwm_channel = 7;

      _light_instance.config(cfg);

      _panel_instance.setLight(
        &_light_instance
      );
    }


    // ======================================================
    // TOUCH
    //
    // FT5446 compatibile con protocollo FT5x06.
    //
    // SDA = GPIO17
    // SCL = GPIO18
    // ADDR = 0x38
    //
    // INT e RESET NON sono GPIO ESP32 diretti:
    // sono collegati al TCA9554.
    //
    // Per questo pin_int e pin_rst restano -1.
    // Lovyan lavorerà in polling I2C.
    // ======================================================

    {
      auto cfg =
        _touch_instance.config();

      /*
       * ATTENZIONE:
       *
       * Il controller touch 4D presenta gli assi
       * fisici scambiati rispetto al pannello RGB.
       *
       * RAW X -> 0 ... 479
       * RAW Y -> 0 ... 799
       */

      cfg.x_min = 0;
      cfg.x_max = 479;

      cfg.y_min = 0;
      cfg.y_max = 799;

      // Interrupt collegato all'expander,
      // quindi non possiamo indicare un GPIO ESP32.

      cfg.pin_int = -1;

      // Stessa cosa per RESET.

      cfg.pin_rst = -1;

      cfg.bus_shared = false;

      /*
       * Questo offset permette di riallineare gli assi
       * FT5446 a quelli del display.
       *
       * Con lcd.setRotation(3):
       *
       * X = 479 - RAW_X
       * Y = RAW_Y
       *
       * che replica la trasformazione usata
       * dalla libreria originale 4D in PORTRAIT.
       */

      cfg.offset_rotation = 7;

      // ----------------------------------------------------
      // I2C
      // ----------------------------------------------------

      cfg.i2c_port = I2C_NUM_0;

      cfg.pin_sda = GPIO_NUM_17;
      cfg.pin_scl = GPIO_NUM_18;

      cfg.freq = 400000;

      cfg.i2c_addr = 0x38;

      _touch_instance.config(cfg);

      _panel_instance.setTouch(
        &_touch_instance
      );
    }


    // ======================================================
    // REGISTRA PANEL
    // ======================================================

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

  static const int HEADER_HEIGHT = 38;

  static const int MAX_ROWS = 50;

  // gen4-ESP32-43CT in PORTRAIT = 480 x 800.
  // Leave 20 px side margins and some space at the bottom.
  static const int TABLE_AREA_X = 20; 
  static const int TABLE_AREA_Y = 60;
  static const int TABLE_AREA_W = 440;
  static const int TABLE_AREA_H = 700;

  // La parte destra viene riservata alla scrollbar.
  // Così durante lo scroll possiamo copiare solo il contenuto e ridisegnare
  // la scrollbar separatamente, senza trascinarne copie fantasma.
  static const int TABLE_SCROLLBAR_ZONE_W = 12;
  static const int TABLE_CONTENT_W = TABLE_AREA_W - TABLE_SCROLLBAR_ZONE_W;

  static const int START_TABLE_X = TABLE_AREA_X;
  static const int START_TABLE_Y = TABLE_AREA_Y;

  static const int TABLE_ROW_WIDTH = TABLE_CONTENT_W;
  static const int TABLE_ROW_HEIGHT = 200;

  static const int WAITING_PANEL_X = 5;
  static const int WAITING_PANEL_Y = 60;
  static const int WAITING_PANEL_WIDTH = 230;
  static const int WAITING_PANEL_HEIGHT = 100;
  static const int WAITING_PANEL_BOX_MARGIN = 10;
  static const int WAITING_PANEL_BOX_WIDTH = 210;
  static const int WAITING_PANEL_BOX_HEIGHT = 80;
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