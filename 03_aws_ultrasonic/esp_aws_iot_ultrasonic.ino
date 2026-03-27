#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
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
// SRF05 ULTRASONIC SENSOR PINS
// ============================================================
#define TRIG_PIN 13  // 🟢 Green wire
#define ECHO_PIN 12  // ⚪ White wire

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
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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
// READ DISTANCE FROM SRF05
// ============================================================
float readDistanceCM() {
  // Take 3 readings and return the median
  long readings[3];
  for (int i = 0; i < 3; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    readings[i] = pulseIn(ECHO_PIN, HIGH, 30000);
    delay(30);
  }

  // Sort and return median
  long tmp;
  if (readings[0] > readings[1]) { tmp = readings[0]; readings[0] = readings[1]; readings[1] = tmp; }
  if (readings[1] > readings[2]) { tmp = readings[1]; readings[1] = readings[2]; readings[2] = tmp; }
  if (readings[0] > readings[1]) { tmp = readings[0]; readings[0] = readings[1]; readings[1] = tmp; }

  long median = readings[1];
  return median == 0 ? 0.0 : median * 0.0343 / 2.0;
}

// ============================================================
// PUBLISH SRF05 SENSOR DATA
// ============================================================
void publishSensorData() {
  StaticJsonDocument<200> doc;

  float distance = readDistanceCM();
  int   detected = (distance > 2.0 && distance < 400.0) ? 1 : 0;

  doc["device_id"]   = client_id;
  doc["distance_cm"] = distance;
  doc["detected"]    = detected;
  doc["msg_count"]   = ++msgCount;
  doc["uptime_ms"]   = millis();

  char buffer[200];
  serializeJson(doc, buffer);

  if (client.publish(pub_topic, buffer)) {
    Serial.print("[PUBLISH] ");
    Serial.print(detected == 1 ? "OBJECT DETECTED" : "CLEAR");
    Serial.print(" | Distance: ");
    Serial.print(distance);
    Serial.print(" cm | ");
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
  Serial.println("=== ESP32 AWS IoT SRF05 Demo ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delay(50);  // Let sensor settle

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