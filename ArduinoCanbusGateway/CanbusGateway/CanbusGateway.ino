#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>
#include <Arduino_PortentaMachineControl.h>
#include "web_interface.h"
#include "USBConfigLoader.h"
#include "can_handler.h"

#define FIRST_REGISTER 0

unsigned long lastCanSendMs = 0;
const unsigned long CAN_SEND_INTERVAL_MS = 20;

// Server Modbus
IPAddress ip(192, 168, 0, 50);
EthernetServer ethServer(502);
ModbusTCPServer modbusTCPServer;

JSONSignal mappingTable[MAX_MAPS];
uint16_t mapCount = 0;

USBConfigLoader configLoader;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //while (!Serial) {}

  Serial.println("Avvio loader config...");

  if (!configLoader.begin()) {
    Serial.println("Errore mount USB");
    return;
  }

  if (!configLoader.loadConfig("config.json")) {
    Serial.println("Errore caricamento config.json");
    return;
  }

  Serial.println("Config caricata correttamente");

  const JSONSignal* loadedMappings = configLoader.getMappings();
  mapCount = configLoader.getMapCount();
  memcpy(mappingTable, loadedMappings, mapCount * sizeof(JSONSignal));

  //STAMPA CONFIG CARICATA
  Serial.println("CONFIGURAZIONE JSON CARICATA!");
  for (uint16_t i = 0; i < mapCount; i++) {
    Serial.print(i);
    Serial.print(" | canID=0x");
    Serial.print(mappingTable[i].canID, HEX);
    Serial.print(" | startBit=");
    Serial.print(mappingTable[i].startBit);
    Serial.print(" | length=");
    Serial.print(mappingTable[i].length);
    Serial.print(" | regIndex=");
    Serial.print(mappingTable[i].regIndex);
    Serial.print(" | source=");
    Serial.println(mappingTable[i].source);
  }

  // Ethernet
  while (Ethernet.begin(NULL, ip) == 0) {
    delay(100);
  }
  Serial.println("Ethernet inizializzato.");
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  // CAN
  if (!MachineControl_CANComm.begin(CanBitRate::BR_500k)) {
    Serial.println("❌ Errore CAN!");
    while (1);
  }
  Serial.println("✅ CAN avviato @500kbps");

  // Modbus TCP
  ethServer.begin();
  if (!modbusTCPServer.begin()) {
    Serial.println("❌ Modbus TCP Server non avviato!");
    while (1);
  }
  modbusTCPServer.configureHoldingRegisters(FIRST_REGISTER, MAX_MAPS); // 2 registri per float
  Serial.println("✅ Modbus TCP Server pronto sulla porta 502");

  // Web server
  //web_init();
}

void loop() {
  //web_handle(mappingTable, MAX_MAPS, mapCount);
  EthernetClient client = ethServer.available();

  if(mapCount > 0){//è stato scaricato un mapping su arduino
    if (client) {
      modbusTCPServer.accept(client);
      Serial.println("🟢 Client TCP Modbus connesso.");
      while (client.connected()) {
        modbusTCPServer.poll();

        // Lettura CAN
        CanDataRaw data;
        if (readCanFrame(data)) {
          for(uint8_t i = 0; i < mapCount; i++){
            if(mappingTable[i].canID != data.canId) continue;
            if(mappingTable[i].source == 0){//messaggio ricevuto da CANBus
              uint16_t raw_data = extractToUint16(data.raw, mappingTable[i].startBit, mappingTable[i].length);
              //AGGIORNAMENTO REGISTRO MODBUS
              Serial.print("Scrivo registro : ");
              Serial.println(mappingTable[i].regIndex);
              modbusTCPServer.holdingRegisterWrite(mappingTable[i].regIndex, raw_data);
            }
          }
        }

        //Lettura Modbus
        uint32_t previousCanId = 0;
        uint8_t dataSend[8] = {0}; 
        bool modbusSourceFound = false;
        //scorrere tutti i mapping
        //verifico se dato è per scrittura verso canbus
        //se previousCanId = 0 vuol dire che sto gestendo il primo dato da scrivere verso canbus
        //se il canId del dato che devo inviare è diverso dal precedente allora procedo con la scrittura verso canbus
        //ad ogni giro estraggo valore dal registro corrispondente del dato e costruisco la parte corrispondente nel payload canbus
        uint16_t isPlcUpdate = readHoldingRegisterU16(FIRST_REGISTER);
        if(isPlcUpdate == 1){
          Serial.println("PLC UPDATE INCOMING");
          for(uint8_t i = 0; i < mapCount; i++){
            if(mappingTable[i].source == 1){
                Serial.print("Registro da leggere : ");
                Serial.println(mappingTable[i].regIndex);
                if(!modbusSourceFound){
                  previousCanId = mappingTable[i].canID;
                  modbusSourceFound = true;
                }

                if(mappingTable[i].canID != previousCanId){
                  CanMsg msg(CanStandardId(previousCanId), 8, dataSend);
                  CAN.write(msg);
                  previousCanId = mappingTable[i].canID;
                  memset(dataSend, 0, sizeof(dataSend));
                }

                uint16_t value = readHoldingRegisterU16(mappingTable[i].regIndex);
                insertBits(dataSend, value, mappingTable[i].startBit, mappingTable[i].length);
            }
          }
          //scrivo ultimo messaggio che non viene preso in considerazione dal for
          if(modbusSourceFound){
            CanMsg msg(CanStandardId(previousCanId), 8, dataSend);
            CAN.write(msg);
          }
          //resetto il register di notifica sul plc
          modbusTCPServer.holdingRegisterWrite(FIRST_REGISTER, (uint16_t)0);
        }
        
      }
      client.stop();
      Serial.println("🔴 Client TCP disconnesso.");
    }
    
  }

  
}


uint16_t readHoldingRegisterU16(int regAddress)
{
  long value = modbusTCPServer.holdingRegisterRead(regAddress);

  if (value < 0)
  {
    return 0; // errore
  }

  return (uint16_t)value;
}
