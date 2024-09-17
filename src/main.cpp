#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <DHT.h>
#include <Battery18650Stats.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define ADC_PIN 35
//SSD1306Wire display(0x3c, 5, 4);

Battery18650Stats battery(ADC_PIN);

// DHT
DHT dht(33, DHT11);

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client = PubSubClient(net);

void messageHandler(char* topic, byte* payload, unsigned int length);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Set the MQTT server (AWS IoT endpoint) and the message handler
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);

  Serial.print("Connecting to AWS IoT");

  while (!client.connected()) {
    Serial.print(".");
    if (client.connect(THINGNAME)) {
      Serial.println("AWS IoT Connected!");
      // Subscribe to a topic once connected
      client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishMessage()
{
  DynamicJsonDocument doc(200);
  doc["Temp"] = dht.readTemperature();
  doc["Humidity"] = dht.readHumidity();
  doc["Volts"] = battery.getBatteryVolts();
  doc["Charge Level"] = battery.getBatteryChargeLevel();
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}


void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("Incoming message: ");
  Serial.print(topic);
  Serial.print(" - ");
  
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Optionally deserialize the JSON payload here
  // StaticJsonDocument<200> doc;
  // deserializeJson(doc, message);
  // const char* msg = doc["message"];
}

void setup() {
  Serial.begin(9600);
  connectAWS();
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    connectAWS();
  }
  publishMessage();
  client.loop();

  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" C | ");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" % ");

  delay(5000);
}
