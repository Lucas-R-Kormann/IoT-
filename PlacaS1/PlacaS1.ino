#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>

// Definições dos pinos
#define DHT_PIN 4
#define ULTRASONIC_TRIG 22
#define ULTRASONIC_ECHO 23
#define LED_PIN 19
#define LDR_PIN 34
#define LED_R 14
#define LED_G 26
#define LED_B 25

// Definições do DHT
#define DHT_TYPE DHT11

// Inicialização do sensor DHT
DHT dht(DHT_PIN, DHT_TYPE);

// Variáveis para armazenamento de valores
float temperatura = 0;
float umidade = 0;
float distancia = 0;
int valorLDR = 0;

// Configurações WiFi e MQTT
WiFiClientSecure client;
PubSubClient mqtt(client);

const String BrokerURL = "test.mosquitto.org";
const int BrokerPort = 1883;
const String BrokerUser = "";
const String BrokerPass = "";

const String SSID = "FIESC_IOT_EDU";
const String PASS = "8120gv08";

void setup() {
  Serial.begin(115200);
  
  // Configuração dos pinos
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  
  // O pino LDR é analógico, não precisa de pinMode
  
  // Inicialização do DHT
  dht.begin();
  
  // Desliga todos os LEDs RGB no início
  setRGBColor(0, 0, 0);
  
  // Configuração WiFi e MQTT
  client.setInsecure();
  Serial.println("Conectando ao WiFi");
  WiFi.begin(SSID, PASS);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nConectado com Sucesso!");

  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BrokerURL.c_str(), BrokerPort);
  mqtt.setCallback(callback); // Define a função de callback
  
  String BoardID = "S1";
  BoardID += String(random(0xffff), HEX);
  
  if(mqtt.connect(BoardID.c_str(), BrokerUser.c_str(), BrokerPass.c_str())) {
    Serial.println("\nConectado ao Broker!");
    
    // Inscreve nos tópicos MQTT
    mqtt.subscribe("Iluminação");
    mqtt.subscribe("LED_RGB");
    mqtt.subscribe("Controle");
    
  } else {
    Serial.println("\nFalha na conexão com o Broker!");
  }
  
  Serial.println("Sistema inicializado!");
  Serial.println("----------------------------------------");
}

void loop() {
  // Mantém a conexão MQTT ativa
  if (!mqtt.connected()) {
    reconnectMQTT();
  }
  mqtt.loop();

  // Leitura dos sensores
  lerDHT();
  lerUltrassonico();
  lerLDR();
  
  // Controle automático do LED baseado na luminosidade
  controleLEDAutomatico();
  
  // Controle do LED RGB baseado na temperatura
  controleRGBPorTemperatura();
  
  // Publica os dados nos tópicos MQTT
  publicarDadosMQTT();
  
  // Exibe todos os valores no monitor serial
  exibirValores();
  
  // Aguarda 2 segundos entre as leituras
  delay(2000);
}

void lerDHT() {
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();
  
  // Verifica se a leitura falhou
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Falha na leitura do DHT!");
    temperatura = -1;
    umidade = -1;
  }
}

void lerUltrassonico() {
  // Limpa o trigger
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  
  // Envia pulso de 10μs no trigger
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);
  
  // Lê o tempo de eco
  long duracao = pulseIn(ULTRASONIC_ECHO, HIGH);
  
  // Calcula a distância (velocidade do som = 340 m/s)
  distancia = duracao * 0.034 / 2;
}

void lerLDR() {
  valorLDR = analogRead(LDR_PIN);
}

void controleLEDAutomatico() {
  // Se estiver escuro (valor LDR baixo), acende o LED
  if (valorLDR < 1000) { // Ajuste este valor conforme necessário
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void controleRGBPorTemperatura() {
  // Controla a cor do LED RGB baseado na temperatura
  if (temperatura < 15) {
    // Azul - Frio
    setRGBColor(0, 0, 255);
  } else if (temperatura >= 15 && temperatura < 25) {
    // Verde - Agradável
    setRGBColor(0, 255, 0);
  } else if (temperatura >= 25 && temperatura < 30) {
    // Amarelo - Quente
    setRGBColor(255, 255, 0);
  } else {
    // Vermelho - Muito quente
    setRGBColor(255, 0, 0);
  }
}

void setRGBColor(int red, int green, int blue) {
  analogWrite(LED_R, red);
  analogWrite(LED_G, green);
  analogWrite(LED_B, blue);
}

void publicarDadosMQTT() {
  // Publica temperatura
  String tempStr = String(temperatura);
  mqtt.publish("sensor/temperatura", tempStr.c_str());
  
  // Publica umidade
  String umidStr = String(umidade);
  mqtt.publish("sensor/umidade", umidStr.c_str());
  
  // Publica distância
  String distStr = String(distancia);
  mqtt.publish("sensor/distancia", distStr.c_str());
  
  // Publica luminosidade
  String ldrStr = String(valorLDR);
  mqtt.publish("sensor/luminosidade", ldrStr.c_str());
  
  // Publica status do LED
  String ledStatus = digitalRead(LED_PIN) ? "LIGADO" : "DESLIGADO";
  mqtt.publish("atuador/led_status", ledStatus.c_str());
}

void exibirValores() {
  Serial.println("=== LEITURAS DOS SENSORES ===");
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  
  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");
  
  Serial.print("Distância: ");
  Serial.print(distancia);
  Serial.println(" cm");
  
  Serial.print("Luminosidade (LDR): ");
  Serial.println(valorLDR);
  
  Serial.print("LED: ");
  Serial.println(digitalRead(LED_PIN) ? "LIGADO" : "DESLIGADO");
  
  Serial.println("----------------------------");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for(int i = 0; i < length; i++) {
    msg += (char) payload[i];
  }
  
  Serial.print("Mensagem recebida: [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);
  
  // Controle do LED simples
  if(String(topic) == "Iluminação") {
    if(msg == "Acender") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED ligado via MQTT");
    } else if(msg == "Apagar") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED desligado via MQTT");
    }
  }
  
  // Controle do LED RGB
  else if(String(topic) == "LED_RGB") {
    if(msg == "Vermelho") {
      setRGBColor(255, 0, 0);
      Serial.println("LED RGB: Vermelho");
    } else if(msg == "Verde") {
      setRGBColor(0, 255, 0);
      Serial.println("LED RGB: Verde");
    } else if(msg == "Azul") {
      setRGBColor(0, 0, 255);
      Serial.println("LED RGB: Azul");
    } else if(msg == "Branco") {
      setRGBColor(255, 255, 255);
      Serial.println("LED RGB: Branco");
    } else if(msg == "Desligar") {
      setRGBColor(0, 0, 0);
      Serial.println("LED RGB: Desligado");
    }
  }
  
  // Controle geral do sistema
  else if(String(topic) == "Controle") {
    if(msg == "Reset") {
      Serial.println("Reiniciando sistema...");
      ESP.restart();
    } else if(msg == "Status") {
      // Publica status completo
      publicarDadosMQTT();
      Serial.println("Status publicado via MQTT");
    }
  }
}

void reconnectMQTT() {
  Serial.println("Reconectando ao MQTT...");
  String BoardID = "S1";
  BoardID += String(random(0xffff), HEX);
  
  if (mqtt.connect(BoardID.c_str(), BrokerUser.c_str(), BrokerPass.c_str())) {
    Serial.println("Reconectado ao Broker!");
    
    // Re-inscreve nos tópicos
    mqtt.subscribe("Iluminação");
    mqtt.subscribe("LED_RGB");
    mqtt.subscribe("Controle");
    
  } else {
    Serial.print("Falha na reconexão, rc=");
    Serial.print(mqtt.state());
    Serial.println(" tentando novamente em 5 segundos");
    delay(5000);
  }
}

// Função adicional para testar o LED RGB manualmente
void testarRGB() {
  Serial.println("Testando LED RGB...");
  
  setRGBColor(255, 0, 0);   // Vermelho
  delay(1000);
  
  setRGBColor(0, 255, 0);   // Verde
  delay(1000);
  
  setRGBColor(0, 0, 255);   // Azul
  delay(1000);
  
  setRGBColor(255, 255, 0); // Amarelo
  delay(1000);
  
  setRGBColor(255, 0, 255); // Magenta
  delay(1000);
  
  setRGBColor(0, 255, 255); // Ciano
  delay(1000);
  
  setRGBColor(255, 255, 255); // Branco
  delay(1000);
  
  setRGBColor(0, 0, 0);     // Desligado
  Serial.println("Teste RGB concluído!");
}