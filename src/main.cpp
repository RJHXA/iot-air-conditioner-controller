#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <DHT.h>
#include <Battery18650Stats.h>
#include <Wire.h>
#include "SSD1306Wire.h" 
#include "Aclonica_Regular_36.h"
#include "Aclonica_Regular_16.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "sala12/sensors"
#define AWS_IOT_SUBSCRIBE_TOPIC "sala12/display"
#define ADC_PIN 35
#define SENSOR_LOCATION "By the door corner"

SSD1306Wire display(0x3c, 5, 4);

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
  doc["Sensor location"] = SENSOR_LOCATION;
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
}

void setup() {
  Serial.begin(9600);
  connectAWS();
  dht.begin();

  // Initialising the UI will init the display too.
  display.init();
  display.resetDisplay();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

void showDisplay() {
  int temp = dht.readTemperature();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Aclonica_Regular_36);
  display.drawString(0, 0, String(temp) + "ºC");

  int humidity = dht.readHumidity();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Aclonica_Regular_16);
  display.drawString(0, 40, "hum: " + String(humidity) + "%");

  int progress = battery.getBatteryChargeLevel();

  display.drawProgressBar(95, 52, 30, 10, progress);

  // draw the percentage as String
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(111, 40, String(progress) + "%");
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
  Serial.print("Battery level: ");
  Serial.print(battery.getBatteryChargeLevel());
  Serial.println("%");
   Serial.print("Battery level: ");
  Serial.print(battery.getBatteryVolts());
  Serial.println(".");
  // clear the display
  display.clear();
  // draw the current demo method
  //demos[demoMode]();
  showDisplay();
  display.display();
  delay(5000);
}
