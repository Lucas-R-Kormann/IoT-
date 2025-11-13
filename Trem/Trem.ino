#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h"

WiFiClientSecure client;
PubSubClient mqtt(client);

const int pinoMotor = 5;
const int ledFrente = 18;   // LED verde - frente
const int ledTras = 19;     // LED vermelho - ré

void setup() {
  Serial.begin(115200);
  pinMode(pinoMotor, OUTPUT);
  pinMode(ledFrente, OUTPUT);
  pinMode(ledTras, OUTPUT);

  // TESTE DOS LEDs
  Serial.println("Testando LED vermelho...");
  digitalWrite(ledTras, HIGH);
  delay(1000);
  digitalWrite(ledTras, LOW);
  delay(500);
  
  Serial.println("Testando LED verde...");
  digitalWrite(ledFrente, HIGH);
  delay(1000);
  digitalWrite(ledFrente, LOW);
  delay(500);
  
  Serial.println("✅ Teste de LEDs concluído");
  // FIM DO TESTE
  
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
  
  // Inicializar com tudo desligado
  analogWrite(pinoMotor, 0);
  digitalWrite(ledFrente, LOW);
  digitalWrite(ledTras, LOW);
}

void loop() {
  if (!mqtt.connected()) {
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
    
    // Controle dos LEDs baseado na direção
    if (velocidade > 0) {
      // FRENTE - LED verde
      analogWrite(pinoMotor, velocidade);
      digitalWrite(ledFrente, HIGH);
      digitalWrite(ledTras, LOW);
      Serial.print("FRENTE - Velocidade: ");
      Serial.println(velocidade);
      
    } else if (velocidade < 0) {
      // RÉ - LED vermelho (usa valor absoluto)
      analogWrite(pinoMotor, abs(velocidade));
      digitalWrite(ledFrente, LOW);
      digitalWrite(ledTras, HIGH);
      Serial.print("RÉ - Velocidade: ");
      Serial.println(abs(velocidade));
      
    } else {
      // PARADO
      analogWrite(pinoMotor, 0);
      digitalWrite(ledFrente, LOW);
      digitalWrite(ledTras, LOW);
      Serial.println("PARADO");
    }
  }
}