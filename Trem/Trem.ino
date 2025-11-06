#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

WiFiClientSecure client;
PubSubClient mqtt(client);

const int pinoMotor = 5;

void setup() {
  Serial.begin(115200);
  pinMode(pinoMotor, OUTPUT);
  client.setInsecure();
  Serial.println("Conectando ao WiFi"); 
  WiFi.begin(WIFI_SSID,WIFI_PASS); 
  while(WiFi.status() != WL_CONNECTED){
    Serial.print("."); 
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");

  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL, BROKER_PORT);
  String BoardID = "Trem";
  BoardID += String(random(0xffff),HEX);
  mqtt.connect(BoardID.c_str() , BROKER_USER , BROKER_PASS);
  while(!mqtt.connected()){
    Serial.print(".");
    delay(200);
  }
  mqtt.subscribe(TOPIC_VELOCIDADE);
  mqtt.setCallback(callback); 
  Serial.println("\nConectado ao Broker!");
}

if (String(topic) == TOPIC_VELOCIDADE) {
  int velocidade = msg.toInt();
  
  if (velocidade >= 0 && velocidade <= 255) {
    analogWrite(pinoMotor, velocidade);
    Serial.print("Velocidade: ");
    Serial.println(velocidade);
  }
}



