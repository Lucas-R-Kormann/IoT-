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

// topicos

  mqtt.subscribe(Topic_S2_Presenca1);
  mqtt.subscribe(Topic_S2_Presenca2);
  mqtt.subscribe(Topic_S1_Iluminacao);
  mqtt.subscribe(Topic_S3_Presenca);
// Inscreve o dispositivo em 4 tópicos MQTT diferentes
// O dispositivo vai receber mensagens publicadas nesses tópicos

  Serial.println("\nConectado ao broker!"); 
  mqtt.setCallback(callback);  //função de callback

  Servo1_S3.setPeriodHertz(50);  //Define a frequência do sinal PWM para 50Hz
  Servo1_S3.attach(Servo1Pin, 500, 2400); //Conecta o servo ao pino específico com pulsos entre 500-2400μs


  Servo2_S3.setPeriodHertz(50);
  Servo2_S3.attach(Servo2Pin, 500, 2400);


  pinMode(Trigger, OUTPUT); //Pino que envia o pulso ultrassônico
  pinMode(echo, INPUT); //Pino que recebe o eco do pulso para calcular distância

}

}

  long lerDistancia(){
  digitalWrite(Trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);
  long duracao = pulseIn(echo, HIGH);
  long distancia = duracao * 349.24 / 2 / 10000;
  return distancia;
    

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

 Serial.printf("Msg:%s / Topic%s\n", msg, topic);

// config servos
  if (strcmp(topic, Topic_S2_Presenca1) == 0 && msg == "S2 - Presença 1:  Em rota de Colisão !!!"){
    Servo1_S3.write(90);
    
  } else if (strcmp(topic, Topic_S3_Presenca) == 0 && msg == "S3 - Presença : em rota colisão !!!"){
    Servo1_S3.write(120);
    Servo2_S3.write(90);

  }else if (strcmp(topic, Topic_S2_Presenca2) == 0 && msg == "S2 - Presença 2: Em rota de Colisão !!!"){
    Servo2_S3.write(120);
  }

// configuração de luz
    if (strcmp(topic, Topic_S1_Iluminacao) == 0 && msg == "Acender")
  {
    digitalWrite(ledPin, HIGH);
    Serial.println("luz ");
  }
  else if (strcmp(topic, Topic_S1_Iluminacao) == 0 && msg == "Apagar")
  {
    digitalWrite(ledPin, LOW);
  }
}
