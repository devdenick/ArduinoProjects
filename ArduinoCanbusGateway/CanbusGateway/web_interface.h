#pragma once
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "gateway_types.h"

EthernetServer webServer(80);

void web_init() {
  webServer.begin();
  Serial.println("Web server started on port 80");
}

void web_handle(JSONSignal* signals, uint8_t maxMessages, uint8_t& messageCount) {
  EthernetClient client = webServer.available();
  if (!client) return;

  client.setTimeout(1500);

  String requestLine = "";
  int contentLength = 0;
  bool isJson = false;

  // Leggo request line + header
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    line.trim();

    if (requestLine.length() == 0) {
      requestLine = line;
    }

    if (line.startsWith("Content-Length:")) {
      contentLength = line.substring(15).toInt();
    }

    if (line.startsWith("Content-Type:") && line.indexOf("application/json") >= 0) {
      isJson = true;
    }

    if (line.length() == 0) {
      break;
    }
  }

  Serial.println("requestLine: "+requestLine);
  if (!requestLine.startsWith("POST ")) {
    client.println("HTTP/1.1 405 Method Not Allowed");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Only POST supported");
    delay(1);
    client.stop();
    return;
  }

  if (!requestLine.startsWith("POST /config ")) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("Route not found");
  delay(1);
  client.stop();
  return;
}

  if (!isJson || contentLength <= 0) {
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Invalid JSON request");
    delay(1);
    client.stop();
    return;
  }

  JsonDocument doc;

  DeserializationError err = deserializeJson(doc, client);
  if (err) {
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print("JSON parse error: ");
    client.println(err.c_str());

    delay(1);
    client.stop();
    return;
  }

  JsonArray messages = doc["Messages"].as<JsonArray>();

  Serial.println("=== JSON RICEVUTO ===");
  Serial.print("Numero messaggi: ");
  Serial.println(messages.size());
  if(messages.size() > 0) messageCount = 0;

  for (JsonObject msg : messages) {
    const char* messageName = msg["MessageName"] | "";
    int canId = msg["CanId"] | 0;
    const char* canIdHex = msg["CanIdHex"] | "";

    Serial.println("----------------------------");
    Serial.print("MessageName: ");
    Serial.println(messageName);
    Serial.print("CanId: ");
    Serial.println(canId);
    Serial.print("CanIdHex: ");
    Serial.println(canIdHex);

    JsonArray fields = msg["Fields"].as<JsonArray>();

    Serial.print("Numero campi: ");
    Serial.println(fields.size());

    for (JsonObject field : fields) {
      const char* fieldName = field["FieldName"] | "";
      uint8_t startBit = field["StartBit"] | 0;
      uint8_t length = field["Length"] | 0;
      const char* dataType = field["DataType"] | "";
      uint8_t io = field["Source"] | 0;
      String modbusRegister = field["ModbusRegister"] | "0";

      uint16_t modbusRegisterInt = (uint16_t)modbusRegister.toInt();

      JSONSignal signal = { canId, startBit, length, modbusRegisterInt, io };

      Serial.print("  FieldName: ");
      Serial.println(fieldName);
      Serial.print("  CanId: ");
      Serial.println(signal.canID);
      Serial.print("  StartBit: ");
      Serial.println(signal.startBit);
      Serial.print("  Length: ");
      Serial.println(signal.length);
      Serial.print("  Source: ");
      Serial.println(signal.source);
      Serial.print("  ModbusRegister: ");
      Serial.println(signal.regIndex);
      Serial.println();

      signals[messageCount++] = signal;
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println("{\"status\":\"ok\"}");

  delay(1);
  client.stop();
}