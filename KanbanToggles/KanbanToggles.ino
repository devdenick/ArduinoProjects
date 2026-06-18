#include <ArduinoMqttClient.h>
#include "OptaBlue.h"
#include <WiFi.h>

using namespace Opta;

/*
----------------------------------DICHIARAZIONI--------------------------------------
*/

//Wifi vars
//MAC A0-CD-F3-B1-EC-1E PASS tVpicCsLsdJHbJpm4d7nv5bx	FLOWRACK_1 
//MAC FC-84-A7-30-B2-58 PASS rdKR3wVHKGhyyN54Qv6tYgr5	FLOWRACK_2
WiFiClient wifiClient;
char SSID[] = "ZFIOT";
char WIFI_PASS[] = "rdKR3wVHKGhyyN54Qv6tYgr5";
byte mac[] = {0xA0, 0xCD, 0xF3, 0xB1, 0xEC, 0x1E};

//mqtt vars
const char broker[] = "10.18.129.41";
int        port     = 1883;
MqttClient mqttClient(wifiClient);
char toggle_topic[] = "flowrack/toggle";
String debounce_topic;

//Util vars
int front_sensorstate = LOW;
int front_precsensorstate = LOW;
int back_sensorstate = LOW;
int back_precsensorstate = LOW;
bool front_state = false; //variabile di toggle
bool back_state = false; //variabile di toggle
int pins[] = {A0,A1,A2,A3,A4,A5,A6,A7,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};//valori pin opta dallo 0 al 23
unsigned long lastStates[] = {LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 1000; // tempo di debouncing in ms
unsigned long lastDebounceTimes[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//Prototipi
void setupWiFiConnection();
void setupMqttCommunication();
void rilevamento_anteriore();
void rilevamento_posteriore();
void initIO(int* pins);
void letturaDI(int PinName);
void letturaDI_Exp(int PinName, Expansion exp);
void mqttVerifyConnection();
int mqttSubscription();
void updateDebouncedelay();
void letturaIngressi();

/*
----------------------------------DICHIARAZIONI--------------------------------------
*/

void setup() {
  Serial.begin(9600);
  //INIZIALIZZAZIONE MODULO ESPANSIONE OPTA
  OptaController.begin(); 
  // CONNESSIONE WIFI
  setupWiFiConnection();
  //INIZIALIZZAZIONE COMUNICAZIONE MQTT
  setupMqttCommunication();
  //INIZIALIZZAZIONE PIN OPTA
  initIO(pins);
}

void loop() {

  //CHECK COMUNICAZIONE MQTT  
  mqttVerifyConnection();
  //CHECK TOPIC DEBOUNCE DELAY
  updateDebouncedelay();
  //LETTURA INGRESSI OPTA E ESPANSIONE OPTA
  letturaIngressi();
   
}

/*
-------------------------------FUNZIONI--------------------------------------
*/

void letturaIngressi(){
  //AGGIORNAMENTO INGRESSI/USCITE OPTA
  OptaController.update();
  //AGGIORNAMENTO INGRESSI/USCITE ESPANSIONE OPTA
  DigitalMechExpansion exp = OptaController.getExpansion(0); //mi riferisco all'espansione
  exp.updateDigitalInputs(); //aggiorno i pin  

  //LETTURA INGRESSI OPTA
  for(int i=0; i < 8; i++){
    letturaDI(i);
  }
  //LETTURA INGRESSI ESPANSIONE OPTA
  for(int i=8; i < 24; i++){
    letturaDI_Exp(i, exp);
  }
}

void setupWiFiConnection(){
  pinMode(LED_USER, OUTPUT);

  // INIZIALIZZAZIONE CONNESSIONE WIFI CON SSID E PASSWORD
  Serial.print("- Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_USER, HIGH);
    delay(1000);
    Serial.print(".");
    digitalWrite(LED_USER, LOW);
    delay(1000);
  }

  Serial.println();
  digitalWrite(LED_USER, HIGH);
  Serial.println("- Wi-Fi connected!");
  Serial.print("- IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMqttCommunication(){
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  debounce_topic = "flowrack/"+WiFi.localIP().toString()+"/debounce";

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

  }else{
    Serial.println("You're connected to the MQTT broker!");
    Serial.println("Iscrizione al topic "+debounce_topic);
    mqttClient.subscribe(debounce_topic);
  }

  Serial.println();

}



void letturaDI(int pinIndex){
  int lettura = digitalRead(pins[pinIndex]);
  if(lettura != lastStates[pinIndex]){
    lastStates[pinIndex] = lettura;
    mqttClient.beginMessage(toggle_topic);
    String letturaString;
    if(lettura == HIGH) letturaString = "HIGH";
    else letturaString = "LOW";
    mqttClient.print(WiFi.localIP().toString() + ":" + String(pinIndex)+":"+letturaString);
    mqttClient.endMessage();
  }
  
}

void letturaDI_Exp(int pinIndex, DigitalMechExpansion exp){
  int lettura = exp.digitalRead(pins[pinIndex]);
  if(lettura != lastStates[pinIndex]){
    lastStates[pinIndex] = lettura;
    mqttClient.beginMessage(toggle_topic);
    String letturaString;
    if(lettura == HIGH) letturaString = "HIGH";
    else letturaString = "LOW";
    mqttClient.print(WiFi.localIP().toString() + ":" + String(pinIndex)+":"+letturaString);
    mqttClient.endMessage();
  }
}

void DItoggle(int pinIndex, unsigned long nowMillis){
  if((nowMillis - lastDebounceTimes[pinIndex]) > debounceDelay){
    Serial.println("Toggle pin index: "+String(pinIndex));
    lastDebounceTimes[pinIndex] = nowMillis;
    mqttClient.beginMessage(toggle_topic);
    mqttClient.print(WiFi.localIP().toString() + ":" + String(pinIndex));
    mqttClient.endMessage();
  }
}

void mqttVerifyConnection(){
  if(!mqttClient.connected()){
    Serial.println("BROKER NON CONNESSO - TENTATIVO RICONNESSIONE");
    if (!mqttClient.connect(broker, port)) {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.connectError());
    }
    else{
      Serial.println("BROKER RICONNESSO");    
      Serial.println("Iscrizione al topic "+debounce_topic);
      mqttClient.subscribe(debounce_topic);
    }
  }
}


void initIO(int* pins){
  for(int i=0; i<=7; i++){
    pinMode(pins[i], INPUT);
  }
}

int mqttSubscription(){
  int messageSize = mqttClient.parseMessage();
  if(messageSize){
    String message = "";
    while(mqttClient.available()){
      message += (char)mqttClient.read();
    }
    return message.toInt();
  }
  return -1;
}

void updateDebouncedelay(){
  int newDebounce = mqttSubscription();
  if(newDebounce != -1){
    debounceDelay = newDebounce;
    Serial.println("Nuovo valore debounce delay: " + String(debounceDelay));
  }
}