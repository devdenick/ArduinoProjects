#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <driver/spi_master.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// =========================================================
// WIFI SETTINGS
// =========================================================
const char* WIFI_SSID = "iPhone di Andrea (2)";
const char* WIFI_PASSWORD = "12345678";

// =========================================================
// OPEN-METEO SETTINGS - FERRARA
// =========================================================
const char* WEATHER_URL =
  "https://api.open-meteo.com/v1/forecast"
  "?latitude=44.8381"
  "&longitude=11.6198"
  "&current=temperature_2m,precipitation,rain,showers,weather_code"
  "&hourly=temperature_2m,precipitation_probability,precipitation,rain,showers,weather_code"
  "&forecast_days=1"
  "&timezone=Europe%2FRome";

const unsigned long WEATHER_REFRESH_MS = 10UL * 60UL * 1000UL;
unsigned long lastWeatherUpdate = 0;

// =========================================================
// LOVYANGFX DISPLAY CONFIG - TUOI SETTINGS
// =========================================================
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
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = true;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }

    { // Backlight
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
      cfg.x_min      = 652;
      cfg.x_max      = 3620;
      cfg.y_min      = 350;
      cfg.y_max      = 3750;
      cfg.pin_int    = -1;
      cfg.bus_shared = true;
      cfg.offset_rotation = 4;
      cfg.spi_host   = SPI3_HOST;
      cfg.freq       = 1000000;
      cfg.pin_sclk   = 25;
      cfg.pin_mosi   = 32;
      cfg.pin_miso   = 39;
      cfg.pin_cs     = 33;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

LGFX lcd;

// =========================================================
// UI COLORS 565
// =========================================================
static const uint16_t C_BG        = 0x0861;
static const uint16_t C_CARD      = 0x18E3;
static const uint16_t C_CARD_2    = 0x2124;
static const uint16_t C_BORDER    = 0x3A49;
static const uint16_t C_TEXT      = 0xFFFF;
static const uint16_t C_MUTED     = 0xBDF7;
static const uint16_t C_BLUE      = 0x04BF;
static const uint16_t C_CYAN      = 0x05FF;
static const uint16_t C_GREEN     = 0x07E0;
static const uint16_t C_YELLOW    = 0xFFE0;
static const uint16_t C_ORANGE    = 0xFD20;
static const uint16_t C_RED       = 0xF800;
static const uint16_t C_BAR_BG    = 0x2945;

// =========================================================
// WEATHER DATA MODEL
// =========================================================
#define MAX_FORECAST_HOURS 8

struct ForecastHour {
  char time[6];       // HH:MM
  float temp;
  float precip;
  float rain;
  float showers;
  int probability;
  int weatherCode;
};

struct WeatherData {
  char currentTime[20];

  float currentTemp;
  float currentPrecip;
  float currentRain;
  float currentShowers;
  int currentWeatherCode;

  float todayTotalMm;
  int todayMaxProbability;
  float peakMm;
  char firstRainTime[6];
  char peakRainTime[6];

  ForecastHour hours[MAX_FORECAST_HOURS];
  int hourCount;
};

WeatherData weather;

// =========================================================
// UTILS
// =========================================================
int parseHourFromIso(const char* isoTime) {
  if (isoTime == nullptr) return -1;
  if (strlen(isoTime) < 13) return -1;

  char hh[3];
  hh[0] = isoTime[11];
  hh[1] = isoTime[12];
  hh[2] = '\0';

  return atoi(hh);
}

void copyHourMinute(char* dest, const char* isoTime) {
  if (isoTime == nullptr || strlen(isoTime) < 16) {
    strcpy(dest, "--:--");
    return;
  }

  dest[0] = isoTime[11];
  dest[1] = isoTime[12];
  dest[2] = ':';
  dest[3] = isoTime[14];
  dest[4] = isoTime[15];
  dest[5] = '\0';
}

bool isRainCode(int code) {
  return
    code == 51 || code == 53 || code == 55 ||
    code == 61 || code == 63 || code == 65 ||
    code == 66 || code == 67 ||
    code == 80 || code == 81 || code == 82 ||
    code == 95 || code == 96 || code == 99;
}

const char* weatherLabel(int code) {
  switch (code) {
    case 0: return "Sereno";
    case 1: return "Quasi sereno";
    case 2: return "Parz. nuvoloso";
    case 3: return "Nuvoloso";

    case 45: return "Nebbia";
    case 48: return "Nebbia brinata";

    case 51: return "Pioviggine debole";
    case 53: return "Pioviggine";
    case 55: return "Pioviggine forte";

    case 61: return "Pioggia debole";
    case 63: return "Pioggia";
    case 65: return "Pioggia forte";

    case 66: return "Pioggia gelata";
    case 67: return "Pioggia gelata forte";

    case 71: return "Neve debole";
    case 73: return "Neve";
    case 75: return "Neve forte";
    case 77: return "Nevischio";

    case 80: return "Rovesci deboli";
    case 81: return "Rovesci";
    case 82: return "Rovesci forti";

    case 95: return "Temporale";
    case 96: return "Temporale grandine";
    case 99: return "Temporale forte";

    default: return "Meteo n/d";
  }
}

String formatTemp(float value) {
  if (isnan(value)) return "--";
  return String(value, 1);
}

String formatMm(float value) {
  if (isnan(value)) return "--";
  return String(value, 1);
}

// =========================================================
// DRAW HELPERS
// =========================================================
void drawStatusScreen(const char* title, const char* message) {
  lcd.fillScreen(C_BG);

  lcd.setTextDatum(MC_DATUM);
  lcd.setTextColor(C_TEXT, C_BG);
  lcd.drawString(title, 120, 120, 4);

  lcd.setTextColor(C_MUTED, C_BG);
  lcd.drawString(message, 120, 155, 2);

  lcd.setTextDatum(TL_DATUM);
}

void drawDropIcon(int x, int y, uint16_t color) {
  lcd.fillTriangle(x, y - 7, x - 7, y + 5, x + 7, y + 5, color);
  lcd.fillCircle(x, y + 6, 7, color);
}

void drawThermoIcon(int x, int y, uint16_t color) {
  lcd.fillRoundRect(x - 3, y - 12, 6, 22, 3, color);
  lcd.fillCircle(x, y + 12, 8, color);
  lcd.drawRoundRect(x - 5, y - 15, 10, 28, 5, C_TEXT);
}

void drawCard(int x, int y, int w, int h, const char* title) {
  lcd.fillRoundRect(x, y, w, h, 12, C_CARD);
  lcd.drawRoundRect(x, y, w, h, 12, C_BORDER);

  lcd.setTextDatum(TL_DATUM);
  lcd.setTextColor(C_MUTED, C_CARD);
  lcd.drawString(title, x + 10, y + 8, 2);
}

void drawHeader(const WeatherData& data) {
  for (int y = 0; y < 52; y++) {
    uint8_t b = 40 + y;
    uint16_t c = lcd.color565(5, 24, b);
    lcd.drawFastHLine(0, y, 240, c);
  }

  lcd.setTextDatum(TL_DATUM);
  lcd.setTextColor(C_TEXT);
  lcd.drawString("Ferrara Meteo", 10, 8, 4);

  lcd.setTextColor(C_MUTED);
  String updateText = "Agg. ";
  updateText += data.currentTime;
  lcd.drawString(updateText, 12, 35, 2);

  if (WiFi.status() == WL_CONNECTED) {
    lcd.fillCircle(220, 20, 5, C_GREEN);
  } else {
    lcd.fillCircle(220, 20, 5, C_RED);
  }
}

void drawCurrentCards(const WeatherData& data) {
  // TEMPERATURA
  drawCard(10, 62, 105, 78, "TEMPERATURA");

  drawThermoIcon(29, 105, C_ORANGE);

  lcd.setTextDatum(TL_DATUM);
  lcd.setTextColor(C_TEXT, C_CARD);
  String tempText = formatTemp(data.currentTemp) + " C";
  lcd.drawString(tempText, 45, 92, 4);

  lcd.setTextColor(C_MUTED, C_CARD);
  lcd.drawString(weatherLabel(data.currentWeatherCode), 18, 122, 2);

  // PIOGGIA
  drawCard(125, 62, 105, 78, "PIOGGIA");

  bool realRain =
    data.currentPrecip > 0.0 ||
    data.currentRain > 0.0 ||
    data.currentShowers > 0.0;

  bool possibleRain = isRainCode(data.currentWeatherCode);

  uint16_t rainColor = realRain ? C_CYAN : possibleRain ? C_YELLOW : C_MUTED;
  drawDropIcon(145, 105, rainColor);

  lcd.setTextColor(C_TEXT, C_CARD);
  String rainText = formatMm(data.currentPrecip) + " mm";
  lcd.drawString(rainText, 163, 92, 4);

  lcd.setTextColor(rainColor, C_CARD);
  if (realRain) {
    lcd.drawString("Sta piovendo", 137, 122, 2);
  } else if (possibleRain) {
    lcd.drawString("Possibile", 145, 122, 2);
  } else {
    lcd.drawString("Asciutto", 150, 122, 2);
  }
}

void drawDailySummary(const WeatherData& data) {
  drawCard(10, 150, 220, 50, "OGGI");

  lcd.setTextColor(C_TEXT, C_CARD);
  String total = "Totale: " + formatMm(data.todayTotalMm) + " mm";
  lcd.drawString(total, 20, 174, 2);

  lcd.setTextColor(C_MUTED, C_CARD);
  String prob = "Max prob: " + String(data.todayMaxProbability) + "%";
  lcd.drawString(prob, 132, 174, 2);

  lcd.setTextColor(C_CYAN, C_CARD);
  String next = "Prima pioggia: ";
  next += data.firstRainTime;
  lcd.drawString(next, 20, 188, 2);
}

void drawForecastRows(const WeatherData& data) {
  lcd.setTextDatum(TL_DATUM);
  lcd.setTextColor(C_TEXT, C_BG);
  lcd.drawString("Prossime ore", 12, 211, 2);

  int startY = 234;
  int rowH = 18;

  for (int i = 0; i < data.hourCount && i < 5; i++) {
    ForecastHour h = data.hours[i];

    int y = startY + i * rowH;
    uint16_t rowColor = (i % 2 == 0) ? C_CARD : C_CARD_2;

    lcd.fillRoundRect(10, y, 220, 16, 5, rowColor);

    bool rainRisk = h.precip > 0.0 || h.rain > 0.0 || h.showers > 0.0 || h.probability >= 50 || isRainCode(h.weatherCode);
    uint16_t dotColor = rainRisk ? C_CYAN : C_MUTED;

    lcd.fillCircle(19, y + 8, 4, dotColor);

    lcd.setTextColor(C_TEXT, rowColor);
    lcd.drawString(h.time, 30, y + 1, 2);

    String temp = String(h.temp, 1) + "C";
    lcd.drawString(temp, 76, y + 1, 2);

    String mm = String(h.precip, 1) + "mm";
    lcd.drawString(mm, 125, y + 1, 2);

    // Probability bar
    int barX = 172;
    int barY = y + 5;
    int barW = 48;
    int fillW = map(constrain(h.probability, 0, 100), 0, 100, 0, barW);

    lcd.fillRoundRect(barX, barY, barW, 6, 3, C_BAR_BG);

    uint16_t probColor = C_GREEN;
    if (h.probability >= 70) probColor = C_CYAN;
    else if (h.probability >= 40) probColor = C_YELLOW;

    lcd.fillRoundRect(barX, barY, fillW, 6, 3, probColor);
  }

  lcd.setTextColor(C_MUTED, C_BG);
  lcd.drawString("mm = precipitazione prevista nell'ora", 12, 302, 2);
}

void drawWeatherScreen(const WeatherData& data) {
  lcd.fillScreen(C_BG);

  drawHeader(data);
  drawCurrentCards(data);
  drawDailySummary(data);
  drawForecastRows(data);
}

// =========================================================
// WIFI
// =========================================================
void connectWiFi() {
  drawStatusScreen("WiFi", "Connessione...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    attempts++;

    lcd.setTextDatum(MC_DATUM);
    lcd.setTextColor(C_MUTED, C_BG);
    String dots = "";
    for (int i = 0; i < attempts % 4; i++) dots += ".";
    lcd.fillRect(70, 175, 100, 20, C_BG);
    lcd.drawString(dots, 120, 185, 2);
  }

  if (WiFi.status() == WL_CONNECTED) {
    drawStatusScreen("WiFi OK", WiFi.localIP().toString().c_str());
    delay(800);
  } else {
    drawStatusScreen("WiFi ERR", "Controlla SSID/password");
    delay(1500);
  }

  lcd.setTextDatum(TL_DATUM);
}

// =========================================================
// OPEN METEO HTTP
// =========================================================
bool fetchWeatherPayload(String& payload, String& errorMessage) {
  if (WiFi.status() != WL_CONNECTED) {
    errorMessage = "WiFi non connesso";
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Per prototipo. In produzione meglio usare certificato CA.

  HTTPClient http;
  http.setTimeout(12000);

  if (!http.begin(client, WEATHER_URL)) {
    errorMessage = "HTTP begin fallito";
    return false;
  }

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    errorMessage = "HTTP code: " + String(httpCode);
    http.end();
    return false;
  }

  payload = http.getString();
  http.end();

  return true;
}

// =========================================================
// JSON PARSING
// =========================================================
bool parseWeatherJson(const String& payload, WeatherData& out, String& errorMessage) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(24576);
#endif

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    errorMessage = "JSON error: ";
    errorMessage += error.c_str();
    return false;
  }

  JsonObject current = doc["current"];

  const char* currentTime = current["time"] | "----";
  strncpy(out.currentTime, currentTime, sizeof(out.currentTime));
  out.currentTime[sizeof(out.currentTime) - 1] = '\0';

  out.currentTemp = current["temperature_2m"] | NAN;
  out.currentPrecip = current["precipitation"] | 0.0;
  out.currentRain = current["rain"] | 0.0;
  out.currentShowers = current["showers"] | 0.0;
  out.currentWeatherCode = current["weather_code"] | -1;

  out.todayTotalMm = 0.0;
  out.todayMaxProbability = 0;
  out.peakMm = 0.0;
  strcpy(out.firstRainTime, "--:--");
  strcpy(out.peakRainTime, "--:--");

  JsonObject hourly = doc["hourly"];

  JsonArray times = hourly["time"];
  JsonArray temps = hourly["temperature_2m"];
  JsonArray probs = hourly["precipitation_probability"];
  JsonArray precips = hourly["precipitation"];
  JsonArray rains = hourly["rain"];
  JsonArray showers = hourly["showers"];
  JsonArray codes = hourly["weather_code"];

  if (times.isNull() || temps.isNull() || probs.isNull() || precips.isNull()) {
    errorMessage = "JSON hourly incompleto";
    return false;
  }

  int currentHour = parseHourFromIso(out.currentTime);
  out.hourCount = 0;

  for (int i = 0; i < times.size(); i++) {
    const char* timeIso = times[i] | "";
    int hour = parseHourFromIso(timeIso);

    float precip = precips[i] | 0.0;
    float rain = rains[i] | 0.0;
    float shower = showers[i] | 0.0;
    int probability = probs[i] | 0;

    out.todayTotalMm += precip;

    if (probability > out.todayMaxProbability) {
      out.todayMaxProbability = probability;
    }

    if (precip > 0.0 && strcmp(out.firstRainTime, "--:--") == 0) {
      copyHourMinute(out.firstRainTime, timeIso);
    }

    if (precip > out.peakMm) {
      out.peakMm = precip;
      copyHourMinute(out.peakRainTime, timeIso);
    }

    if (hour >= currentHour && out.hourCount < MAX_FORECAST_HOURS) {
      ForecastHour& h = out.hours[out.hourCount];

      copyHourMinute(h.time, timeIso);
      h.temp = temps[i] | NAN;
      h.probability = probability;
      h.precip = precip;
      h.rain = rain;
      h.showers = shower;
      h.weatherCode = codes[i] | -1;

      out.hourCount++;
    }
  }

  return true;
}

bool updateWeather() {
  drawStatusScreen("Meteo", "Aggiornamento...");

  String payload;
  String errorMessage;

  if (!fetchWeatherPayload(payload, errorMessage)) {
    drawStatusScreen("Errore HTTP", errorMessage.c_str());
    return false;
  }

  if (!parseWeatherJson(payload, weather, errorMessage)) {
    drawStatusScreen("Errore JSON", errorMessage.c_str());
    return false;
  }

  drawWeatherScreen(weather);
  return true;
}

// =========================================================
// SETUP / LOOP
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(255);
  lcd.fillScreen(C_BG);

  lcd.setTextDatum(TL_DATUM);
  lcd.setTextColor(C_TEXT, C_BG);

  connectWiFi();

  if (updateWeather()) {
    lastWeatherUpdate = millis();
  } else {
    lastWeatherUpdate = millis();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (millis() - lastWeatherUpdate >= WEATHER_REFRESH_MS) {
    updateWeather();
    lastWeatherUpdate = millis();
  }

  delay(500);
}