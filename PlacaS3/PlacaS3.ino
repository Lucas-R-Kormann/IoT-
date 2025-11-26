#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <env.h>


WiFiClient client;
PubSubClient mqtt(client);

const byte Trigger = *;
const byte echo = *;

const byte pinLed = *;

//os servos vem aqui
Servo Servo1_S3;
Servo Servo2_S3;

const byte Servo1Pin = *;
const byte Servo2Pin = *;

void setup() {
  Serial.begin(115200);
  Serial.println("Conectando ao WiFi");
  WiFi.begin(SSID,PASS);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");

  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BrokerURL.c_str(), BrokerPort);
  String BoardID = "S3";
  BoardID += String(random(0xffff), HEX);
  mqtt.connect(BrokerURL.c_str(), BrokerUser.c_str(), BrokerPass.c_str());
  while(!mqtt.connected()) {
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nConectado ao Broker!");
}

void loop(){

  // distancia 
  long distancia = lerDistancia();

  if (distancia < 20){
    mqtt.publish(Topic_S3_Presenca, "S3 - Presença : em rota colisão !!!");
  }else{
    mqtt.publish(Topic_S3_Presenca, "S3 - Presença : Caminho Livre");
  }

delay(200);
mqtt.loop(500);
}
void callback(char* topic, byte* payload, unsigned int lenght){
String msg = "";
for(int i = 0; i < length; i++){
  msg += (char) payload[i];
}
if(topic == "Iluminação" && msg == "Acender"){
  digitalWrite(2,high);
}else if(topic == "Iluminação" && msg == "Apagar"){
  digitalWrite(2,low)
}
}