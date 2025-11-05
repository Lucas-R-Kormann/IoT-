#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient client;
PubSubClient mqtt(client);

const String BrokerURL = "test.mosquitto.org";
const int BrokerPort = 1883;
const String BrokerUser = "";
const String BrokerPass = "";

const String SSID = "FIESC_IOT_EDU";
const String PASS = "8120gv08";

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

void loop() {
  mqtt.publish("Iluminação")

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