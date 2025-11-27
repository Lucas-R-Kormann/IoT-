/*
 * PROJETO IoT - PLACA S1
 * Sistema completo de monitoramento e controle com MQTT
 * Componentes: DHT11, Sensor Ultrassônico, LDR, LED RGB, LED simples
 * Broker: HiveMQ Cloud
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <env.h>

// ==============================
// DEFINIÇÕES DE PINOS
// ==============================

#define DHT_PIN 4           // Pino do sensor de temperatura e umidade DHT11
#define ULTRASONIC_TRIG 22  // Pino TRIG do sensor ultrassônico (envia sinal)
#define ULTRASONIC_ECHO 23  // Pino ECHO do sensor ultrassônico (recebe sinal)
#define LED_PIN 19          // Pino do LED simples (controle de iluminação)
#define LDR_PIN 34          // Pino do sensor de luz LDR (entrada analógica)
#define LED_R 14            // Pino do LED RGB - Cor Vermelha
#define LED_G 26            // Pino do LED RGB - Cor Verde
#define LED_B 25            // Pino do LED RGB - Cor Azul

// ==============================
// VARIÁVEIS GLOBAIS
// ==============================

// Variáveis para armazenar leituras dos sensores
float temperatura = 0;      // Valor da temperatura em °C
float umidade = 0;          // Valor da umidade em %
float distancia = 0;        // Distância medida em cm
int valorLDR = 0;           // Valor de luminosidade do LDR (0-4095)

// ==============================
// CONFIGURAÇÕES DE REDE E MQTT
// ==============================

// Objetos para comunicação de rede
WiFiClientSecure client;    // Cliente WiFi seguro (para TLS)
PubSubClient mqtt(client);  // Cliente MQTT

// Configurações do Broker HiveMQ Cloud
const String BrokerURL = "e0b29cef20fa4c27a7169a414c4a3cf5.s1.eu.hivemq.cloud";
const int BrokerPort = 8883;        // Porta TLS para conexão segura
const String BrokerUser = "Placa_S1";     // Usuário do broker
const String BrokerPass = "Placa_s1123";  // Senha do broker

// Configurações da rede WiFi
const String SSID_WIFI = "FIESC_IOT_EDU";  // Nome da rede WiFi
const String PASS_WIFI = "8120gv08";       // Senha da rede WiFi

// ==============================
// VARIÁVEIS DE CONTROLE DE TEMPO
// ==============================

unsigned long ultimaLeituraDHT = 0;               // Último tempo de leitura do DHT
const unsigned long intervaloLeituraDHT = 2000;   // Intervalo entre leituras do DHT (2 segundos)

// ==============================
// FUNÇÃO SETUP - INICIALIZAÇÃO
// ==============================

void setup() {
  // Inicializa comunicação serial para debug
  Serial.begin(115200);
  Serial.println("\n=== INICIALIZANDO SISTEMA PLACA S1 ===");
  
  // ==============================
  // CONFIGURAÇÃO DOS PINOS
  // ==============================
  
  // Sensores e atuadores digitais
  pinMode(ULTRASONIC_TRIG, OUTPUT);   // Pino TRIG como saída
  pinMode(ULTRASONIC_ECHO, INPUT);    // Pino ECHO como entrada
  pinMode(LED_PIN, OUTPUT);           // LED simples como saída
  pinMode(LED_R, OUTPUT);             // LED RGB Vermelho como saída
  pinMode(LED_G, OUTPUT);             // LED RGB Verde como saída
  pinMode(LED_B, OUTPUT);             // LED RGB Azul como saída
  pinMode(DHT_PIN, INPUT_PULLUP);     // DHT com pull-up interno
  
  // Nota: LDR_PIN (34) é analógico e não precisa de pinMode
  
  // Inicializa todos os LEDs desligados
  setRGBColor(0, 0, 0);  // RGB desligado
  digitalWrite(LED_PIN, LOW);  // LED simples desligado
  
  // ==============================
  // CONEXÃO COM REDE WiFi
  // ==============================
  
  Serial.println("Conectando ao WiFi: " + String(SSID_WIFI));
  WiFi.begin(SSID_WIFI.c_str(), PASS_WIFI.c_str());
  
  // Aguarda conexão com timeout de 10 segundos
  int tentativas = 0;
  while(WiFi.status() != WL_CONNECTED && tentativas < 20){
    Serial.print(".");
    delay(500);
    tentativas++;
  }
  
  // Verifica se conectou com sucesso
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nCONECTADO - WiFi Conectado com Sucesso!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nERRO - Falha na conexão WiFi!");
    return;  // Para execução se não conectar ao WiFi
  }

  // ==============================
  // CONFIGURAÇÃO DO MQTT
  // ==============================
  
  // Configura conexão TLS insegura (para testes - aceita qualquer certificado)
  client.setInsecure();
  
  Serial.println("Configurando MQTT...");
  mqtt.setServer(BrokerURL.c_str(), BrokerPort);  // Define servidor MQTT
  mqtt.setCallback(callback);                     // Define função para mensagens recebidas
  
  // Tenta conectar ao broker MQTT
  conectarMQTT();
  
  Serial.println("Sistema inicializado com sucesso!");
  Serial.println("========================================");
}

// ==============================
// FUNÇÃO LOOP - EXECUÇÃO PRINCIPAL
// ==============================

void loop() {
  // ==============================
  // GERENCIAMENTO DE CONEXÃO MQTT
  // ==============================
  
  // Verifica e reconecta ao MQTT se necessário
  if (!mqtt.connected()) {
    conectarMQTT();
  }
  mqtt.loop();  // Processa mensagens MQTT recebidas
  
  // ==============================
  // LEITURA DOS SENSORES
  // ==============================
  
  lerUltrassonico();  // Lê distância do sensor ultrassônico
  lerLDR();           // Lê luminosidade do LDR
  
  // Leitura do DHT11 a cada 2 segundos (evita sobrecarga do sensor)
  if (millis() - ultimaLeituraDHT > intervaloLeituraDHT) {
    lerDHTReal();            // Lê temperatura e umidade
    ultimaLeituraDHT = millis();  // Atualiza tempo da última leitura
  }
  
  // ==============================
  // CONTROLE AUTOMÁTICO
  // ==============================
  
  controleLEDAutomatico();      // Controla LED baseado na luminosidade
  controleRGBPorTemperatura();  // Controla cor do RGB baseado na temperatura
  
  // ==============================
  // COMUNICAÇÃO MQTT
  // ==============================
  
  publicarDadosMQTT();  // Publica dados dos sensores no broker
  exibirValores();      // Exibe valores no monitor serial
  
  // Pequena pausa para estabilidade do sistema
  delay(500);
}

// ==============================
// FUNÇÃO: CONEXÃO MQTT
// ==============================

void conectarMQTT() {
  Serial.println("Conectando ao Broker MQTT...");
  
  // Gera um ClientID único para esta placa
  String clientId = "ESP32-S1-";
  clientId += String(random(0xffff), HEX);  // Adiciona número randomico
  
  Serial.print("ClientID: ");
  Serial.println(clientId);
  
  // Tenta conexão até 5 vezes
  int tentativas = 0;
  while (!mqtt.connected() && tentativas < 5) {
    Serial.print("Tentativa " + String(tentativas + 1) + " de conexão MQTT... ");
    
    // Tenta conectar com credenciais
    if (mqtt.connect(clientId.c_str(), BrokerUser.c_str(), BrokerPass.c_str())) {
      Serial.println("CONECTADO!");
      
      // ==============================
      // INSCRIÇÃO NOS TÓPICOS MQTT
      // ==============================
      
      // Tópico para controle do LED simples
      if (mqtt.subscribe("Iluminação")) {
        Serial.println("Inscrito em: Iluminação");
      }
      
      // Tópico para controle do LED RGB
      if (mqtt.subscribe("LED_RGB")) {
        Serial.println("Inscrito em: LED_RGB");
      }
      
      // Tópico para comandos gerais do sistema
      if (mqtt.subscribe("Controle")) {
        Serial.println("Inscrito em: Controle");
      }
      
    } else {
      // Exibe erro detalhado
      Serial.print("ERRO - Falha, rc=");
      Serial.print(mqtt.state());
      Serial.println(" - " + getMQTTErrorString(mqtt.state()));
      Serial.println("Tentando novamente em 3 segundos...");
      delay(3000);
      tentativas++;
    }
  }
}

// ==============================
// FUNÇÃO: DESCRIÇÃO DE ERROS MQTT
// ==============================

String getMQTTErrorString(int state) {
  switch(state) {
    case -4: return "Tempo limite de conexão";
    case -3: return "Conexão perdida";
    case -2: return "Conexão falhou";
    case -1: return "Cliente desconectado";
    case 0: return "Conectado";
    case 1: return "Versão de protocolo inválida";
    case 2: return "ClientID rejeitado";
    case 3: return "Servidor indisponível";
    case 4: return "Credenciais inválidas";
    case 5: return "Não autorizado";
    default: return "Erro desconhecido";
  }
}

// ==============================
// FUNÇÃO: LEITURA DO SENSOR DHT11
// ==============================

void lerDHTReal() {
  byte data[5] = {0, 0, 0, 0, 0};  // Array para armazenar os 5 bytes de dados
  byte i = 0;
  
  // ==============================
  // PROTOCOLO DE COMUNICAÇÃO DHT11
  // ==============================
  
  // Fase 1: Inicia comunicação
  pinMode(DHT_PIN, OUTPUT);     // Define pino como saída
  digitalWrite(DHT_PIN, LOW);   // Sinal baixo para iniciar
  delay(18);                    // Aguarda 18ms (exigência do sensor)
  digitalWrite(DHT_PIN, HIGH);  // Sinal alto
  delayMicroseconds(40);        // Aguarda 40μs
  pinMode(DHT_PIN, INPUT_PULLUP); // Muda para entrada com pull-up
  
  // Fase 2: Aguarda resposta do sensor
  unsigned long timeout = micros() + 10000;  // Timeout de 10ms
  while(digitalRead(DHT_PIN) == LOW) {
    if (micros() > timeout) {
      Serial.println("ERRO DHT: Timeout na resposta inicial");
      temperatura = -1;
      umidade = -1;
      return;
    }
  }
  
  // Fase 3: Aguarda sinal de preparação
  timeout = micros() + 10000;
  while(digitalRead(DHT_PIN) == HIGH) {
    if (micros() > timeout) {
      Serial.println("ERRO DHT: Timeout no sinal de preparação");
      temperatura = -1;
      umidade = -1;
      return;
    }
  }
  
  // ==============================
  // LEITURA DOS 40 BITS DE DADOS
  // ==============================
  
  for (i = 0; i < 40; i++) {
    // Aguarda pulso baixo
    timeout = micros() + 10000;
    while(digitalRead(DHT_PIN) == LOW) {
      if (micros() > timeout) {
        Serial.println("ERRO DHT: Timeout no pulso baixo");
        temperatura = -1;
        umidade = -1;
        return;
      }
    }
    
    // Mede duração do pulso alto
    unsigned long t = micros();
    timeout = micros() + 10000;
    while(digitalRead(DHT_PIN) == HIGH) {
      if (micros() > timeout) {
        Serial.println("ERRO DHT: Timeout no pulso alto");
        temperatura = -1;
        umidade = -1;
        return;
      }
    }
    
    // Se pulso alto > 40μs, é bit 1, senão é bit 0
    if ((micros() - t) > 40) {
      data[i/8] |= (1 << (7 - (i % 8)));  // Define bit como 1
    }
  }
  
  // ==============================
  // VERIFICAÇÃO E PROCESSAMENTO
  // ==============================
  
  // Verifica checksum (soma dos 4 primeiros bytes deve igualar o 5º byte)
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    umidade = data[0];     // Byte 0: Umidade inteira
    temperatura = data[2]; // Byte 2: Temperatura inteira
    
    // Verifica se temperatura é negativa (bit mais significativo = 1)
    if (data[2] & 0x80) {
      temperatura = -1 - (data[2] & 0x7F);  // Converte para negativo
    }
    
    Serial.println("Leitura DHT bem-sucedida");
  } else {
    Serial.println("ERRO DHT: Erro no checksum do DHT");
    temperatura = -1;  // Marca como erro
    umidade = -1;      // Marca como erro
  }
}

// ==============================
// FUNÇÃO: LEITURA SENSOR ULTRASSÔNICO
// ==============================

void lerUltrassonico() {
  // Fase 1: Prepara o sensor
  digitalWrite(ULTRASONIC_TRIG, LOW);  // Garante sinal baixo
  delayMicroseconds(2);                // Pequena pausa
  
  // Fase 2: Envia pulso de trigger
  digitalWrite(ULTRASONIC_TRIG, HIGH);  // Sinal alto
  delayMicroseconds(10);                // Pulso de 10μs
  digitalWrite(ULTRASONIC_TRIG, LOW);   // Volta para baixo
  
  // Fase 3: Mede tempo de eco
  long duracao = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);  // Timeout de 30ms
  
  // Fase 4: Calcula distância
  if (duracao > 0) {
    // Fórmula: distância = (tempo × velocidade_som) / 2
    // velocidade_som = 340 m/s = 0.034 cm/μs
    distancia = duracao * 0.034 / 2;
    
    // Filtro: valores acima de 400cm são considerados erro
    if (distancia > 400) {
      distancia = -1;  // Marca como erro
    }
  } else {
    distancia = -1;  // Timeout ou erro na leitura
  }
}

// ==============================
// FUNÇÃO: LEITURA SENSOR LDR (LUZ)
// ==============================

void lerLDR() {
  // Lê valor analógico do LDR (0 = escuro, 4095 = muito claro)
  valorLDR = analogRead(LDR_PIN);
  
  // Verifica se valor está dentro da faixa válida
  if (valorLDR < 0 || valorLDR > 4095) {
    valorLDR = -1;  // Marca como erro
  }
}

// ==============================
// FUNÇÃO: CONTROLE AUTOMÁTICO DO LED
// ==============================

void controleLEDAutomatico() {
  // Se estiver escuro (LDR < 1000) E leitura for válida, acende o LED
  if (valorLDR != -1 && valorLDR < 1000) {
    digitalWrite(LED_PIN, HIGH);  // Acende LED
  } else {
    digitalWrite(LED_PIN, LOW);   // Apaga LED
  }
}

// ==============================
// FUNÇÃO: CONTROLE LED RGB POR TEMPERATURA
// ==============================

void controleRGBPorTemperatura() {
  // Se temperatura for inválida, indica erro com magenta
  if (temperatura == -1) {
    setRGBColor(255, 0, 255);  // Magenta = erro
    return;
  }
  
  // Define cores baseado na temperatura:
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

// ==============================
// FUNÇÃO: CONTROLE COR LED RGB
// ==============================

void setRGBColor(int red, int green, int blue) {
  // Define intensidade de cada cor do LED RGB
  analogWrite(LED_R, red);    // Intensidade vermelho (0-255)
  analogWrite(LED_G, green);  // Intensidade verde (0-255)
  analogWrite(LED_B, blue);   // Intensidade azul (0-255)
}

// ==============================
// FUNÇÃO: PUBLICA DADOS NO MQTT
// ==============================

void publicarDadosMQTT() {
  static unsigned long ultimaPublicacao = 0;
  
  // Publica a cada 5 segundos (evita sobrecarga do broker)
  if (millis() - ultimaPublicacao > 5000 && mqtt.connected()) {
    
    Serial.println("PUBLICANDO - === PUBLICANDO DADOS NO MQTT ===");
    
    // Publica temperatura no tópico específico
    String tempStr = String(temperatura);
    if (mqtt.publish("sensor/temperatura", tempStr.c_str())) {
      Serial.println("OK - Temperatura: " + tempStr + "°C");
    }
    
    // Publica umidade no tópico específico
    String umidStr = String(umidade);
    if (mqtt.publish("sensor/umidade", umidStr.c_str())) {
      Serial.println("OK - Umidade: " + umidStr + "%");
    }
    
    // Publica distância no tópico específico
    String distStr = String(distancia);
    if (mqtt.publish("sensor/distancia", distStr.c_str())) {
      Serial.println("OK - Distância: " + distStr + "cm");
    }
    
    // Publica luminosidade no tópico específico
    String ldrStr = String(valorLDR);
    if (mqtt.publish("sensor/luminosidade", ldrStr.c_str())) {
      Serial.println("OK - Luminosidade: " + ldrStr);
    }
    
    // Publica status do LED
    String ledStatus = digitalRead(LED_PIN) ? "LIGADO" : "DESLIGADO";
    if (mqtt.publish("atuador/led_status", ledStatus.c_str())) {
      Serial.println("OK - LED: " + ledStatus);
    }
    
    Serial.println("=================================");
    
    ultimaPublicacao = millis();  // Atualiza tempo da última publicação
  }
}

// ==============================
// FUNÇÃO: EXIBE VALORES NO SERIAL
// ==============================

void exibirValores() {
  static unsigned long ultimaExibicao = 0;
  
  // Exibe a cada 2 segundos (evita sobrecarga do serial)
  if (millis() - ultimaExibicao > 2000) {
    Serial.println("\n=== LEITURAS DOS SENSORES ===");
    
    // Exibe temperatura com tratamento de erro
    Serial.print("Temperatura: ");
    if (temperatura == -1) Serial.print("ERRO");
    else Serial.print(temperatura);
    Serial.println(" °C");
    
    // Exibe umidade com tratamento de erro
    Serial.print("Umidade: ");
    if (umidade == -1) Serial.print("ERRO");
    else Serial.print(umidade);
    Serial.println(" %");
    
    // Exibe distância com tratamento de erro
    Serial.print("Distância: ");
    if (distancia == -1) Serial.print("ERRO");
    else Serial.print(distancia);
    Serial.println(" cm");
    
    // Exibe luminosidade com tratamento de erro
    Serial.print("Luminosidade (LDR): ");
    if (valorLDR == -1) Serial.print("ERRO");
    else Serial.print(valorLDR);
    Serial.println("");
    
    // Exibe status do LED
    Serial.print("LED: ");
    Serial.println(digitalRead(LED_PIN) ? "LIGADO" : "DESLIGADO");
    
    // Exibe status das conexões
    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "CONECTADO" : "DESCONECTADO");
    
    Serial.print("MQTT: ");
    Serial.println(mqtt.connected() ? "CONECTADO" : "DESCONECTADO");
    
    Serial.println("----------------------------");
    
    ultimaExibicao = millis();  // Atualiza tempo da última exibição
  }
}

// ==============================
// FUNÇÃO: CALLBACK MQTT (MENSAGENS RECEBIDAS)
// ==============================

void callback(char* topic, byte* payload, unsigned int length) {
  // Converte payload para string
  String msg = "";
  for(int i = 0; i < length; i++) {
    msg += (char) payload[i];
  }
  
  // Exibe mensagem recebida
  Serial.print("Mensagem recebida: [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);
  
  // ==============================
  // CONTROLE DO LED SIMPLES
  // ==============================
  if(String(topic) == "Iluminação") {
    if(msg == "Acender") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println(">>> LED ligado via MQTT");
    } else if(msg == "Apagar") {
      digitalWrite(LED_PIN, LOW);
      Serial.println(">>> LED desligado via MQTT");
    }
  }
  
  // ==============================
  // CONTROLE DO LED RGB
  // ==============================
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
  
  // ==============================
  // COMANDOS GERAIS DO SISTEMA
  // ==============================
  else if(String(topic) == "Controle") {
    if(msg == "Reset") {
      Serial.println(">>> Reiniciando sistema...");
      ESP.restart();  // Reinicia o ESP32
    } else if(msg == "Status") {
      Serial.println(">>> Solicitando status...");
      publicarDadosMQTT();  // Publica dados imediatamente
    }
  }
}

// ==============================
// FIM DO CÓDIGO
// ==============================