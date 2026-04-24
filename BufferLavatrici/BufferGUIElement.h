#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "BufferGUI.h"

namespace BufferGUIElements
{
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
    }
  };
}