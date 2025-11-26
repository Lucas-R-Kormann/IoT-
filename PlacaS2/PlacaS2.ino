#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "env.h"

WiFiClientSecure client;
PubSubClient mqtt(client);

// Definição dos pinos - sensor 1
const byte TRIGGER_PIN1 = 2;
const byte ECHO_PIN1 = 4;

// Definição dos pinos - sensor 2  
const byte TRIGGER_PIN2 = 5;
const byte ECHO_PIN2 = 18;

const int ledPin = 19;

// Tópicos MQTT
const char* TOPIC_PRESENCA1 = "sensor/ultrassonico1";
const char* TOPIC_PRESENCA2 = "sensor/ultrassonico2";
const char* TOPIC_ILUMINACAO = "iluminacao";

void setup() {
  Serial.begin(115200);
  client.setInsecure();
  
  // Configuração sensor 1
  pinMode(TRIGGER_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  
  // Configuração sensor 2
  pinMode(TRIGGER_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);
  
  // Configuração LED
  pinMode(ledPin, OUTPUT);

  Serial.println("Conectando ao WiFi");
  WiFi.begin(SSID, PASS);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");

  Serial.println("Conectando ao Broker...");
  
  mqtt.setServer(BrokerURL, BrokerPort);
  mqtt.setCallback(callback);
  
  String BoardID = "S2";
  BoardID += String(random(0xffff), HEX);
  
  if(mqtt.connect(BoardID.c_str(), BrokerUser, BrokerPass)) {
    Serial.println("\nConectado ao Broker!");
    mqtt.subscribe(TOPIC_ILUMINACAO);
  } else {
    Serial.println("\nFalha ao conectar ao Broker!");
    Serial.print("Estado: ");
    Serial.println(mqtt.state());
  }
}

// Função para ler distância
long lerDistancia(byte triggerPin, byte echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  
  long duracao = pulseIn(echoPin, HIGH);
  long distancia = duracao * 0.034924 / 2;
  
  return distancia;
}

void loop() {
  // Sensor 1
  long distancia1 = lerDistancia(TRIGGER_PIN1, ECHO_PIN1);
  
  Serial.print("Sensor 1 - Distância: ");
  Serial.print(distancia1);
  Serial.println(" cm");

  if (distancia1 < 10) {
    Serial.println("Trem detectado no sensor 1!");
    mqtt.publish(TOPIC_PRESENCA1, "Trem detectado");
    delay(2000);
  }

  // Sensor 2
  long distancia2 = lerDistancia(TRIGGER_PIN2, ECHO_PIN2);
  
  Serial.print("Sensor 2 - Distância: ");
  Serial.print(distancia2);
  Serial.println(" cm");

  if (distancia2 < 10) {
    Serial.println("Trem detectado no sensor 2!");
    mqtt.publish(TOPIC_PRESENCA2, "Trem detectado");
    delay(2000);
  }

  mqtt.loop();
  delay(500);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for(int i = 0; i < length; i++) {
    msg += (char) payload[i];
  }
  
  Serial.println("Mensagem recebida: " + msg + " no tópico: " + String(topic));
  
  if(strcmp(topic, TOPIC_ILUMINACAO) == 0) {
    if(msg.equals("Acender")) {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED aceso");
    } else if(msg.equals("Apagar")) {
      digitalWrite(ledPin, LOW);
      Serial.println("LED apagado");
    }
  }
}