#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <env.h>

// Definições dos pinos
#define DHT_PIN 4
#define ULTRASONIC_TRIG 22
#define ULTRASONIC_ECHO 23
#define LED_PIN 19
#define LDR_PIN 34
#define LED_R 14
#define LED_G 26
#define LED_B 25

// Variáveis para armazenamento de valores
float temperatura = 0;
float umidade = 0;
float distancia = 0;
int valorLDR = 0;

// Configurações WiFi e MQTT
WiFiClientSecure client;
PubSubClient mqtt(client);

const String BrokerURL = "e0b29cef20fa4c27a7169a414c4a3cf5.s1.eu.hivemq.cloud";
const int BrokerPort = 8883;  // Mudei para 8883 (TLS)
const String BrokerUser = "Placa_S1";
const String BrokerPass = "Placa_s1123";

const String SSID_WIFI = "FIESC_IOT_EDU";
const String PASS_WIFI = "8120gv08";

// Variáveis para leitura do DHT
unsigned long ultimaLeituraDHT = 0;
const unsigned long intervaloLeituraDHT = 2000;

void setup() {
  Serial.begin(115200);
  
  // Configuração dos pinos
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(DHT_PIN, INPUT_PULLUP);
  
  // Desliga todos os LEDs RGB no início
  setRGBColor(0, 0, 0);
  
  // Configuração WiFi e MQTT
  client.setInsecure();  // Para conexão TLS sem verificação de certificado
  Serial.println("Conectando ao WiFi");
  WiFi.begin(SSID_WIFI.c_str(), PASS_WIFI.c_str());
  
  int tentativas = 0;
  while(WiFi.status() != WL_CONNECTED && tentativas < 20){
    Serial.print(".");
    delay(500);
    tentativas++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado com Sucesso!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha na conexão WiFi!");
    return;
  }

  Serial.println("Conectando ao Broker...");
  mqtt.setServer(BrokerURL.c_str(), BrokerPort);
  mqtt.setCallback(callback);
  
  conectarMQTT();
  
  Serial.println("Sistema inicializado!");
  Serial.println("----------------------------------------");
}

void loop() {
  // Mantém a conexão MQTT ativa
  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();

  // Leitura dos sensores
  lerUltrassonico();
  lerLDR();
  
  // Leitura do DHT a cada 2 segundos
  if (millis() - ultimaLeituraDHT > intervaloLeituraDHT) {
    lerDHTReal();
    ultimaLeituraDHT = millis();
  }
  
  // Controle automático do LED baseado na luminosidade
  controleLEDAutomatico();
  
  // Controle do LED RGB baseado na temperatura
  controleRGBPorTemperatura();
  
  // Publica os dados nos tópicos MQTT
  publicarDadosMQTT();
  
  // Exibe todos os valores no monitor serial
  exibirValores();
  
  delay(500);
}

void conectarMQTT() {
  Serial.println("Conectando ao MQTT Broker...");
  
  String clientId = "ESP32-S1-";
  clientId += String(random(0xffff), HEX);
  
  int tentativas = 0;
  while (!mqtt.connected() && tentativas < 5) {
    Serial.print("Tentativa de conexão MQTT... ");
    
    if (mqtt.connect(clientId.c_str(), BrokerUser.c_str(), BrokerPass.c_str())) {
      Serial.println("Conectado!");
      
      // Inscreve nos tópicos MQTT
      mqtt.subscribe("Iluminação");
      mqtt.subscribe("LED_RGB");
      mqtt.subscribe("Controle");
      
      Serial.println("Inscrito nos tópicos:");
      Serial.println("- Iluminação");
      Serial.println("- LED_RGB");
      Serial.println("- Controle");
      
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqtt.state());
      Serial.println(" tentando novamente em 3 segundos");
      delay(3000);
      tentativas++;
    }
  }
}

void lerDHTReal() {
  byte data[5] = {0, 0, 0, 0, 0};
  byte i = 0;
  
  // Inicia a comunicação
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(18);
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHT_PIN, INPUT_PULLUP);
  
  // Aguarda resposta do sensor
  unsigned long timeout = micros() + 10000;
  while(digitalRead(DHT_PIN) == LOW) {
    if (micros() > timeout) {
      Serial.println("Timeout na resposta do DHT");
      temperatura = -1;
      umidade = -1;
      return;
    }
  }
  
  timeout = micros() + 10000;
  while(digitalRead(DHT_PIN) == HIGH) {
    if (micros() > timeout) {
      Serial.println("Timeout no sinal do DHT");
      temperatura = -1;
      umidade = -1;
      return;
    }
  }
  
  // Lê os 40 bits de dados
  for (i = 0; i < 40; i++) {
    timeout = micros() + 10000;
    while(digitalRead(DHT_PIN) == LOW) {
      if (micros() > timeout) {
        Serial.println("Timeout na leitura do DHT");
        temperatura = -1;
        umidade = -1;
        return;
      }
    }
    
    unsigned long t = micros();
    timeout = micros() + 10000;
    while(digitalRead(DHT_PIN) == HIGH) {
      if (micros() > timeout) {
        Serial.println("Timeout no pulso do DHT");
        temperatura = -1;
        umidade = -1;
        return;
      }
    }
    
    if ((micros() - t) > 40) {
      data[i/8] |= (1 << (7 - (i % 8)));
    }
  }
  
  // Verifica checksum
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    umidade = data[0];
    temperatura = data[2];
    
    // Converte para valores negativos se necessário
    if (data[2] & 0x80) {
      temperatura = -1 - (data[2] & 0x7F);
    }
    
    Serial.println("Leitura DHT bem-sucedida");
  } else {
    Serial.println("Erro no checksum do DHT");
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
  long duracao = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
  
  // Calcula a distância (velocidade do som = 340 m/s)
  if (duracao > 0) {
    distancia = duracao * 0.034 / 2;
    // Filtro para valores muito altos (acima de 400cm considera erro)
    if (distancia > 400) {
      distancia = -1;
    }
  } else {
    distancia = -1; // Indica erro na leitura
  }
}

void lerLDR() {
  // Faz a leitura do LDR (0-4095)
  valorLDR = analogRead(LDR_PIN);
  
  // Filtro para valores inconsistentes
  if (valorLDR < 0 || valorLDR > 4095) {
    valorLDR = -1;
  }
}

void controleLEDAutomatico() {
  // Se estiver escuro (valor LDR baixo) e leitura for válida, acende o LED
  if (valorLDR != -1 && valorLDR < 1000) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void controleRGBPorTemperatura() {
  // Só controla se a temperatura for válida
  if (temperatura == -1) {
    setRGBColor(255, 0, 255); // Magenta para indicar erro
    return;
  }
  
  // Controla a cor do LED RGB baseado na temperatura
  if (temperatura < 15) {
    setRGBColor(0, 0, 255);     // Azul - Frio
  } else if (temperatura < 25) {
    setRGBColor(0, 255, 0);     // Verde - Agradável
  } else if (temperatura < 30) {
    setRGBColor(255, 255, 0);   // Amarelo - Quente
  } else {
    setRGBColor(255, 0, 0);     // Vermelho - Muito quente
  }
}

void setRGBColor(int red, int green, int blue) {
  analogWrite(LED_R, red);
  analogWrite(LED_G, green);
  analogWrite(LED_B, blue);
}

void publicarDadosMQTT() {
  static unsigned long ultimaPublicacao = 0;
  
  // Publica a cada 5 segundos para não sobrecarregar o broker
  if (millis() - ultimaPublicacao > 5000 && mqtt.connected()) {
    
    Serial.println("=== PUBLICANDO DADOS NO MQTT ===");
    
    // Publica temperatura
    String tempStr = String(temperatura);
    if (mqtt.publish("sensor/temperatura", tempStr.c_str())) {
      Serial.println("✓ Temperatura: " + tempStr + "°C");
    }
    
    // Publica umidade
    String umidStr = String(umidade);
    if (mqtt.publish("sensor/umidade", umidStr.c_str())) {
      Serial.println("✓ Umidade: " + umidStr + "%");
    }
    
    // Publica distância
    String distStr = String(distancia);
    if (mqtt.publish("sensor/distancia", distStr.c_str())) {
      Serial.println("✓ Distância: " + distStr + "cm");
    }
    
    // Publica luminosidade
    String ldrStr = String(valorLDR);
    if (mqtt.publish("sensor/luminosidade", ldrStr.c_str())) {
      Serial.println("✓ Luminosidade: " + ldrStr);
    }
    
    // Publica status do LED
    String ledStatus = digitalRead(LED_PIN) ? "LIGADO" : "DESLIGADO";
    if (mqtt.publish("atuador/led_status", ledStatus.c_str())) {
      Serial.println("✓ LED: " + ledStatus);
    }
    
    Serial.println("=================================");
    
    ultimaPublicacao = millis();
  }
}

void exibirValores() {
  static unsigned long ultimaExibicao = 0;
  
  // Exibe a cada 2 segundos para não sobrecarregar a serial
  if (millis() - ultimaExibicao > 2000) {
    Serial.println("=== LEITURAS DOS SENSORES ===");
    
    Serial.print("Temperatura: ");
    if (temperatura == -1) Serial.print("ERRO");
    else Serial.print(temperatura);
    Serial.println(" °C");
    
    Serial.print("Umidade: ");
    if (umidade == -1) Serial.print("ERRO");
    else Serial.print(umidade);
    Serial.println(" %");
    
    Serial.print("Distância: ");
    if (distancia == -1) Serial.print("ERRO");
    else Serial.print(distancia);
    Serial.println(" cm");
    
    Serial.print("Luminosidade (LDR): ");
    if (valorLDR == -1) Serial.print("ERRO");
    else Serial.print(valorLDR);
    Serial.println("");
    
    Serial.print("LED: ");
    Serial.println(digitalRead(LED_PIN) ? "LIGADO" : "DESLIGADO");
    
    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "CONECTADO" : "DESCONECTADO");
    
    Serial.print("MQTT: ");
    Serial.println(mqtt.connected() ? "CONECTADO" : "DESCONECTADO");
    
    Serial.println("----------------------------");
    
    ultimaExibicao = millis();
  }
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
      Serial.println(">>> LED ligado via MQTT");
    } else if(msg == "Apagar") {
      digitalWrite(LED_PIN, LOW);
      Serial.println(">>> LED desligado via MQTT");
    }
  }
  
  // Controle do LED RGB
  else if(String(topic) == "LED_RGB") {
    if(msg == "Vermelho") {
      setRGBColor(255, 0, 0);
      Serial.println(">>> LED RGB: Vermelho");
    } else if(msg == "Verde") {
      setRGBColor(0, 255, 0);
      Serial.println(">>> LED RGB: Verde");
    } else if(msg == "Azul") {
      setRGBColor(0, 0, 255);
      Serial.println(">>> LED RGB: Azul");
    } else if(msg == "Branco") {
      setRGBColor(255, 255, 255);
      Serial.println(">>> LED RGB: Branco");
    } else if(msg == "Desligar") {
      setRGBColor(0, 0, 0);
      Serial.println(">>> LED RGB: Desligado");
    }
  }
  
  // Controle geral do sistema
  else if(String(topic) == "Controle") {
    if(msg == "Reset") {
      Serial.println(">>> Reiniciando sistema...");
      ESP.restart();
    } else if(msg == "Status") {
      Serial.println(">>> Solicitando status...");
      publicarDadosMQTT();
    }
  }
}