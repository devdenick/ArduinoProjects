#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <driver/spi_master.h>
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
      cfg.freq_write = 80000000;
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
      cfg.rgb_order        = true;
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

bool lcdCleared = true;

bool touchHold = false;
int prevY;
BufferGUI::SwipeState prevSwipeState = BufferGUI::STILL;

BufferGUI::TableRow tableRows[BufferGUI::MAX_ROWS];

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
  BufferGUI::drawWifiTopRight(lcd, false);

  BufferGUI::initTable(tableRows);
  BufferGUI::clearTableArea(lcd);
  BufferGUI::drawTable(lcd, tableRows);

  Serial.println("LovyanGFX avviato.");
}

void loop()
{
  int x, y;
  bool isTouched = lcd.getTouch(&x, &y);

  if (isTouched)
  {
    if (!touchHold)
    {
      touchHold = true;
      prevY = y;
      return;
    }

    BufferGUI::SwipeState swipeState = BufferGUI::swipe(prevY, y);

    if (swipeState != BufferGUI::STILL)
    {
      BufferGUI::swipeTable(tableRows, swipeState);

      BufferGUI::clearTableArea(lcd);
      BufferGUI::drawTable(lcd, tableRows);
    }

    prevY = y;
  }
  else
  {
    touchHold = false;
  }
}