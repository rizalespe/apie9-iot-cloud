#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "secrets.h"

// ============================================================
// CONFIGURATION
// ============================================================
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const char* aws_endpoint = AWS_ENDPOINT;
const int   aws_port     = 8883;
const char* client_id    = AWS_CLIENT_ID;
const char* pub_topic    = "esp32/sensor/data";
const char* sub_topic    = "esp32/commands";

// ============================================================
// DHT SENSOR CONFIGURATION
// ============================================================
#define DHTPIN 12       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// ============================================================
// CERTIFICATES (loaded from secrets.h)
// ============================================================
const char* root_ca     = ROOT_CA;
const char* device_cert = DEVICE_CERT;
const char* private_key = PRIVATE_KEY;

// ============================================================
// GLOBAL OBJECTS
// ============================================================
WiFiClientSecure net;
PubSubClient     client(net);

unsigned long lastPublish = 0;
const long    interval    = 5000;  // publish every 5 seconds
int           msgCount    = 0;

// ============================================================
// CALLBACK — Invoked when a message arrives from AWS
// ============================================================
void messageCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[SUBSCRIBE] Message on topic: ");
  Serial.println(topic);

  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(msg);
}

// ============================================================
// WIFI CONNECTION
// ============================================================
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.enableIPv6();
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++retry > 30) {
      Serial.println("\nFailed to connect. Restarting...");
      ESP.restart();
    }
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IPv4 address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
  Serial.print("IPv6 link-local address: ");
  Serial.println(WiFi.linkLocalIPv6());
  
  delay(1000);
  Serial.print("IPv6 global address: ");
  Serial.println(WiFi.globalIPv6());
}

// ============================================================
// AWS IoT CONNECTION
// ============================================================
void connectAWS() {
  net.setCACert(root_ca);
  net.setCertificate(device_cert);
  net.setPrivateKey(private_key);

  client.setServer(aws_endpoint, aws_port);
  client.setCallback(messageCallback);
  client.setBufferSize(512);

  Serial.print("Connecting to AWS IoT Core");
  while (!client.connected()) {
    Serial.print(".");
    if (client.connect(client_id)) {
      Serial.println("\nConnected to AWS IoT Core!");
      client.subscribe(sub_topic);
      Serial.print("Subscribed to topic: ");
      Serial.println(sub_topic);
    } else {
      Serial.print("\nFailed. State: ");
      Serial.print(client.state());
      Serial.println(" — Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// ============================================================
// PUBLISH DHT SENSOR DATA
// ============================================================
void publishSensorData() {
  StaticJsonDocument<200> doc;

  float humidity = dht.readHumidity();              // Read humidity
  float temperature = dht.readTemperature();        // Read temperature in Celsius
  float fahrenheit = dht.readTemperature(true);     // Read temperature in Fahrenheit

  doc["device_id"]               = client_id;
  doc["humidity"]                = humidity;
  doc["temperature_celsius"]     = temperature;
  doc["temperature_fahrenheit"]  = fahrenheit;
  doc["msg_count"]               = ++msgCount;
  doc["uptime_ms"]               = millis();

  char buffer[200];
  serializeJson(doc, buffer);

  if (client.publish(pub_topic, buffer)) {
    Serial.print("[PUBLISH] ");
    Serial.print(humidity);
    Serial.print(" | Temperature: ");
    Serial.print(temperature);
    Serial.print("°C | ");
    Serial.print(fahrenheit);
    Serial.print("°F | ");
    Serial.println(buffer);
  } else {
    Serial.println("[ERROR] Publish failed!");
  }
}

// ============================================================
// SETUP & LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 AWS IoT DHT Demo ===");

  dht.begin();

  connectWiFi();
  connectAWS();
}

void loop() {
  // Reconnect if disconnected
  if (!client.connected()) {
    Serial.println("Connection lost. Reconnecting...");
    connectAWS();
  }

  client.loop();

  // Publish data every 'interval' ms
  unsigned long now = millis();
  if (now - lastPublish >= interval) {
    lastPublish = now;
    publishSensorData();
  }
}