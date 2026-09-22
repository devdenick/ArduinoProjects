#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>

// ============================================================
// 4D Systems gen4-ESP32-43CT (NON Q)
// Display RGB 800x480, usato in portrait 480x800.
//
// Questa configurazione riprende quella che stai già usando
// sul tuo progetto attuale.
// ============================================================

class LGFX_4D_43CT : public lgfx::LGFX_Device
{
public:
  lgfx::Bus_RGB      _bus_instance;
  lgfx::Panel_RGB    _panel_instance;
  lgfx::Light_PWM    _light_instance;
  lgfx::Touch_FT5x06 _touch_instance;

  LGFX_4D_43CT(void)
  {
    // --------------------------------------------------------
    // PANEL
    // --------------------------------------------------------
    {
      auto cfg = _panel_instance.config();

      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width   = 800;
      cfg.panel_height  = 480;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;

      cfg.pin_cs   = -1;
      cfg.pin_rst  = -1;
      cfg.pin_busy = -1;

      cfg.readable   = true;
      cfg.invert     = false;
      cfg.rgb_order  = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
    }

    // --------------------------------------------------------
    // FRAMEBUFFER IN PSRAM
    // --------------------------------------------------------
    {
      auto cfg = _panel_instance.config_detail();
      cfg.use_psram = 1;
      _panel_instance.config_detail(cfg);
    }

    // --------------------------------------------------------
    // RGB BUS
    // --------------------------------------------------------
    {
      auto cfg = _bus_instance.config();

      cfg.panel = &_panel_instance;

      // B0..B4
      cfg.pin_d0 = GPIO_NUM_8;
      cfg.pin_d1 = GPIO_NUM_3;
      cfg.pin_d2 = GPIO_NUM_46;
      cfg.pin_d3 = GPIO_NUM_9;
      cfg.pin_d4 = GPIO_NUM_1;

      // G0..G5
      cfg.pin_d5  = GPIO_NUM_5;
      cfg.pin_d6  = GPIO_NUM_6;
      cfg.pin_d7  = GPIO_NUM_7;
      cfg.pin_d8  = GPIO_NUM_15;
      cfg.pin_d9  = GPIO_NUM_16;
      cfg.pin_d10 = GPIO_NUM_4;

      // R0..R4
      cfg.pin_d11 = GPIO_NUM_45;
      cfg.pin_d12 = GPIO_NUM_48;
      cfg.pin_d13 = GPIO_NUM_47;
      cfg.pin_d14 = GPIO_NUM_21;
      cfg.pin_d15 = GPIO_NUM_14;

      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_henable = GPIO_NUM_40; // DE
      cfg.pin_pclk    = GPIO_NUM_42;

      // Ridotto da 16 MHz a 12 MHz per aumentare il margine di banda
      // del framebuffer RGB in PSRAM quando WiFi/MQTT sono attivi.
      // Sul pannello RGB un underflow del DMA può apparire come porzioni
      // dell'immagine duplicate/spostate per uno o più refresh.
      cfg.freq_write = 16000000;

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 8;
      cfg.hsync_pulse_width = 4;
      cfg.hsync_back_porch  = 8;

      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 4;
      cfg.vsync_back_porch  = 8;

      cfg.pclk_active_neg = true;
      cfg.pclk_idle_high  = true;
      cfg.de_idle_high    = false;

      _bus_instance.config(cfg);
    }

    _panel_instance.setBus(&_bus_instance);

    // --------------------------------------------------------
    // BACKLIGHT
    // --------------------------------------------------------
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = GPIO_NUM_2;
      cfg.invert = false;
      cfg.freq = 25000;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    // --------------------------------------------------------
    // TOUCH FT5446 / FT5x06
    // --------------------------------------------------------
    {
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;
      cfg.x_max = 479;
      cfg.y_min = 0;
      cfg.y_max = 799;

      cfg.pin_int = -1;
      cfg.pin_rst = -1;
      cfg.bus_shared = false;

      // Questa demo usa lcd.setRotation(3).
      // Offset touch verificato per la stessa rotazione logica.
      cfg.offset_rotation = 7;

      cfg.i2c_port = I2C_NUM_0;
      cfg.pin_sda = GPIO_NUM_17;
      cfg.pin_scl = GPIO_NUM_18;
      cfg.freq = 400000;
      cfg.i2c_addr = 0x38;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};