#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char *ssid = "ASB"; // Enter your Wi-Fi name
const char *password = "zxcvbnm9";  // Enter Wi-Fi password

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic_pub = "apie_camp/iot/sensor";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

unsigned long lastTime = 0;
unsigned long intervalTime = 5000;

char cstr[3];

WiFiClient espClient;
PubSubClient client(espClient);

void connectToMqttBroker(){
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        Serial.println("Public EMQX MQTT broker connected");
    } else {
        Serial.print("failed with state ");
        Serial.print(client.state());
        delay(2000);
    }
  }
}

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);

  // Connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the Wi-Fi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);

  connectToMqttBroker();
}

void loop() {
  if ((millis() - lastTime) > intervalTime) {
    if (!client.connected()) {
      connectToMqttBroker();
    }
    
    int temperature = 25;
    itoa(temperature, cstr, 10);

    client.publish(topic_pub, cstr);
    Serial.println("Publish data to MQTT broker");

    lastTime = millis();
  }  
}
