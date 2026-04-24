#include <Ethernet.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include "OptaBlue.h"

using namespace Opta;

EthernetClient ethClient; //client Web Ethernet
MqttClient mqttClient(ethClient);

const char broker[] = "192.168.200.10";
int        port     = 1883;
const char topic[]  = "arduino/simple";

//Dichiarazione delle funzioni
void rilevamento_anteriore();
void rilevamento_posteriore();
void initIO(int* pins);
void letturaDI(int PinName);
void letturaDI_Exp(int PinName, Expansion exp);
void mqttVerifyConnection();
int mqttSubscription();
void updateDebouncedelay();

//Dichiarazione delle variabili
int front_sensorstate = LOW;
int front_precsensorstate = LOW;
int back_sensorstate = LOW;
int back_precsensorstate = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 1000; // tempo di debouncing in ms
bool front_state = false; //variabile di toggle
bool back_state = false; //variabile di toggle

int pins[] = {A0,A1,A2,A3,A4,A5,A6,A7,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
unsigned long lastDebounceTimes[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const char toggle_topic[] = "flowrack/toggle"; 
String debounce_topic;

//Ethernet
IPAddress ip(192, 168, 200, 170); // Indirizzo IP statico del publisher MQTT
IPAddress gateway(192,168,200,10);
IPAddress subnet(255, 255, 255,0);
byte mac[] = {0xA0, 0xCD, 0xF3, 0xB1, 0xEC, 0x1E};

//WiFi
char ssid[] = "ZFIOT";    // your network SSID (name)
char pass[] = "tVpicCsLsdJHbJpm4d7nv5bx";    // your network password (use for WPA, or use as key for WEP)

void setup() {
  // Inizializzo la comunicazionne seriale a 9600 baud
  Serial.begin(9600);

  // CONNESSIONE WIFI
  
  pinMode(LED_USER, OUTPUT);

  // Wait for the serial port to connect,
  // This is necessary for boards that have native USB.
  while (!Serial);

  // Start the Wi-Fi connection using the provided SSID and password.
  Serial.print("- Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

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
  
  
  OptaController.begin(); //inizializzo l'espansione


  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  mqttClient.setId(WiFi.localIP().toString());

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  initIO(pins); // imposto modalità pin

  // subscribe to a topic
  debounce_topic = "flowrack/"+WiFi.localIP().toString()+"/debounce";
  
  mqttClient.subscribe(debounce_topic);

}

void loop() {
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  OptaController.update();
  mqttVerifyConnection();

  
  updateDebouncedelay();
  

  
  DigitalMechExpansion exp = OptaController.getExpansion(0); //mi riferisco all'espansione
  exp.updateDigitalInputs(); //aggiorno i pin  

  for(int i=0; i < 8; i++){
    letturaDI(i);
  }

  for(int i=8; i < 24; i++){
    letturaDI_Exp(i, exp);
  }

  /*
  int lettura = exp.digitalRead(0);
  Serial.println(lettura);
  */

}

void letturaDI(int pinIndex){
  int lettura = digitalRead(pins[pinIndex]); 
  unsigned long nowMillis = millis();
  if(lettura == HIGH){
    //Serial.println("Impulso " + String(pinName) + "rilevato");
    DItoggle(pinIndex, nowMillis);
  }
  
}

void letturaDI_Exp(int pinIndex, DigitalMechExpansion exp){
  int lettura = exp.digitalRead(pins[pinIndex]);
  unsigned long nowMillis = millis();
  if(lettura == HIGH){
    //Serial.println("Impulso " + String(pinName) + "rilevato");
    DItoggle(pinIndex, nowMillis);
  }
}

void DItoggle(int pinIndex, unsigned long nowMillis){
  if((nowMillis - lastDebounceTimes[pinIndex]) > debounceDelay){
    Serial.println("Toggle pin "+String(pins[pinIndex])+" ");
    lastDebounceTimes[pinIndex] = nowMillis;
    mqttClient.beginMessage(toggle_topic);
    mqttClient.print(Ethernet.localIP().toString() + ":" + String(pinIndex));
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
