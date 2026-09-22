#include <WiFi.h>

const char* SSID = "iPhone di Andrea (2)";
const char* PASSWORD = "12345678";

bool eraConnesso = false;
unsigned long ultimoTentativo = 0;
const unsigned long INTERVALLO = 20000;

void stampaInfo() {
  Serial.println("\n===== WIFI CONNESSO =====");
  Serial.print("SSID: "); Serial.println(WiFi.SSID());
  Serial.print("MAC ESP32 (STA): "); Serial.println(WiFi.macAddress());
  Serial.print("IP locale: "); Serial.println(WiFi.localIP());
  Serial.print("Subnet mask: "); Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
  Serial.print("DNS 1: "); Serial.println(WiFi.dnsIP(0));
  Serial.print("DNS 2: "); Serial.println(WiFi.dnsIP(1));
  Serial.print("MAC access point: "); Serial.println(WiFi.BSSIDstr());
  Serial.print("Canale: "); Serial.println(WiFi.channel());
  Serial.print("Segnale RSSI: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.println("========================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Serial.print("MAC ESP32 (STA): "); Serial.println(WiFi.macAddress());
  Serial.print("Connessione alla rete: "); Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);
  ultimoTentativo = millis();
}

void loop() {
  bool connesso = WiFi.status() == WL_CONNECTED;
  if (connesso && !eraConnesso) {
    stampaInfo();
  } else if (!connesso && eraConnesso) {
    Serial.println("Wi-Fi disconnesso. Riconnessione in corso...");
    ultimoTentativo = millis();
  }
  eraConnesso = connesso;

  if (!connesso && millis() - ultimoTentativo >= INTERVALLO) {
    ultimoTentativo = millis();
    Serial.print("Non connesso. Stato Wi-Fi: ");
    Serial.println((int)WiFi.status());
    Serial.println("Nuovo tentativo...");
    WiFi.reconnect();
  }
  delay(100);
}