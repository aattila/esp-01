
#include <credentials.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include "PubSubClient.h"
#include <OneWire.h>
#include <DallasTemperature.h>


#define DHTTYPE AM2301
#define DHTPIN  2
#define ONE_WIRE_PIN 0
#define DONE_PIN 3

WiFiClient espClient;
PubSubClient client(espClient);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature DS18B20(&oneWire);
 
DHT dht(DHTPIN, DHTTYPE, 11);
  

void shutDown() {
  // TPL5110 shut down
  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);
  // if shut down not worked loop in each 5 minutes
  delay(300000);
}
 
void setup(void) {

  Serial.begin(115200);
  dht.begin();
  DS18B20.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\n\r \n\rConnecting to Wifi");

  int wifiConnectCredit = 0;

  // 10 credits for connection or shut down
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    wifiConnectCredit++;
    if(wifiConnectCredit >= 10) {
      shutDown();
    }
  }
  
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
   
  client.setServer(MQTT_SERVER, MQTT_PORT);
  
}
 
void loop(void) {

  int mqttConnectCredit = 0;

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MQTT_DEV_SOLAR, MQTT_DEV_SOLAR_TOKEN, NULL)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // 5 credits for connection or shut down
      mqttConnectCredit++;
      if(mqttConnectCredit >= 5) {
        shutDown();
      }
      
      delay(5000);
    }
  }

  // wait for sensor warmup
  delay(2000);
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();

  DS18B20.requestTemperatures();
  float stemp = DS18B20.getTempCByIndex(0);
  
  if (isnan(humidity) || isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    String json = "{\"temperature\": "+String((int)temp)+", \"humidity\": "+String((int)humidity)+", \"solartemp\": "+String((int)stemp)+" }";
    client.publish(MQTT_TELEMETRY_TOPIC, json.c_str());
    Serial.print("published: ");
    Serial.println(json);
    client.loop();
    // wait to complete the http loop
    delay(2000);
  }

  shutDown();
} 
