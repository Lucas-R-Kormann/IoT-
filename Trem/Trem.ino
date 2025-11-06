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
  WiFi.begin(SSID,PASS); 
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

void loop() {
  if (!mqtt.connected()) {
    // Reconectar se necessário
    String BoardID = "Trem";
    BoardID += String(random(0xffff),HEX);
    mqtt.connect(BoardID.c_str(), BROKER_USER, BROKER_PASS);
    mqtt.subscribe(TOPIC_VELOCIDADE);
  }
  
  mqtt.loop();
  delay(100);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == TOPIC_VELOCIDADE) {
    int velocidade = msg.toInt();
    
    if (velocidade >= 0 && velocidade <= 255) {
      analogWrite(pinoMotor, velocidade);
      Serial.print("Velocidade: ");
      Serial.println(velocidade);
    }
  }
}