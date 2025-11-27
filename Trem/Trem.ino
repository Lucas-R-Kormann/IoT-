#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "env.h" // Arquivo com credenciais

// Criação dos objetos para conexão WiFi segura e cliente MQTT
WiFiClientSecure client;
PubSubClient mqtt(client);

const int pinoMotor = 5;    // Pino PWM para controle do motor
const int ledFrente = 18;   // LED verde - frente
const int ledTras = 19;     // LED vermelho - ré

void setup() {
  // Inicialização da comunicação serial para debug
  Serial.begin(115200);
  // Configuração dos pinos como saída
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
  
  Serial.println("Teste de LEDs concluído");
  // FIM DO TESTE
  
  // Configuração para conexão WiFi insegura (sem certificado)
  client.setInsecure();
  // Conexão à rede WiFi
  Serial.println("Conectando ao WiFi"); 
  WiFi.begin(SSID,PASS); // SSID e senha definidos no arquivo env.h

  // Aguarda até que a conexão WiFi seja estabelecida
  while(WiFi.status() != WL_CONNECTED){
    Serial.print("."); 
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");

  // Conexão ao broker MQTT
  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BROKER_URL, BROKER_PORT); // Configura servidor MQTT

  // Gera um ID único para este dispositivo usando número aleatório
  String BoardID = "Trem";
  BoardID += String(random(0xffff),HEX);

  // Conecta ao broker MQTT com credenciais
  mqtt.connect(BoardID.c_str() , BROKER_USER , BROKER_PASS);

  // Aguarda até que a conexão MQTT seja estabelecida
  while(!mqtt.connected()){
    Serial.print(".");
    delay(200);
  }

  // Inscreve-se no tópico de velocidade e define função de callback
  mqtt.subscribe(TOPIC_VELOCIDADE);
  mqtt.setCallback(callback); // Função que será chamada quando chegar mensagem
  Serial.println("\nConectado ao Broker!");
  
  // Inicializar com tudo desligado
  analogWrite(pinoMotor, 0);
  digitalWrite(ledFrente, LOW);
  digitalWrite(ledTras, LOW);
}

void loop() {
  // Verifica se ainda está conectado ao broker MQTT
  if (!mqtt.connected()) {
    // Se desconectou, tenta reconectar com novo ID
    String BoardID = "Trem";
    BoardID += String(random(0xffff),HEX);
    mqtt.connect(BoardID.c_str(), BROKER_USER, BROKER_PASS);
    mqtt.subscribe(TOPIC_VELOCIDADE); // Renova a inscrição no tópico
  }
  
  // Processa mensagens MQTT recebidas
  mqtt.loop();
  delay(100); // Pequena pausa para estabilidade
}

// Função chamada automaticamente quando chega mensagem MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  // Converte o payload (conteúdo da mensagem) para String
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  // Exibe informações da mensagem recebida no monitor serial
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  // Verifica se a mensagem é do tópico de velocidade
  if (String(topic) == TOPIC_VELOCIDADE) {
    int velocidade = msg.toInt(); // Converte a mensagem para inteiro
    
    // Controle dos LEDs baseado na direção
    if (velocidade > 0) {
      // FRENTE - LED verde
      analogWrite(pinoMotor, velocidade); // PWM proporcional à velocidade
      digitalWrite(ledFrente, HIGH); // Acende LED verde
      digitalWrite(ledTras, LOW); // Apaga LED vermelho
      Serial.print("FRENTE - Velocidade: ");
      Serial.println(velocidade);
      
    } else if (velocidade < 0) {
      // RÉ - LED vermelho (usa valor absoluto)
      analogWrite(pinoMotor, abs(velocidade)); // Usa valor absoluto para PWM
      digitalWrite(ledFrente, LOW); // Apaga LED verde
      digitalWrite(ledTras, HIGH); // Acende LED vermelho
      Serial.print("RÉ - Velocidade: ");
      Serial.println(abs(velocidade));
      
    } else {
      // PARADO
      analogWrite(pinoMotor, 0); // Desliga o motor
      digitalWrite(ledFrente, LOW); // Apaga LED verde
      digitalWrite(ledTras, LOW); // Apaga LED vermelho
      Serial.println("PARADO");
    }
  }
}