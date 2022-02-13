/*
* HOW TO CONNECT GEIGER KIT?
* The kit 3 wires that should be connected to Arduino UNO board: 5V, GND and INT. PullUp resistor is included on
* kit PCB. Connect INT wire to Digital Pin#2 (INT0), 5V to 5V, GND to GND. Then connect the Arduino with
* USB cable to the computer and upload this sketch. 
* 
*
*/


#include <credentials.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "PubSubClient.h"

#define DEBUG 0

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#define LOG_PERIOD 15000
#else
#define debug(x)
#define debugln(x)
#define LOG_PERIOD 60000
#endif

#define IMPULSE_PIN    2  // GPIO2

#define GM_INDEX     151  // 151 CPM = 1uSv/h for M4011 GM Tube
#define MAX_PERIOD 60000  // minutes definition
unsigned int multiplier;  // standby events multiplier

unsigned long counts;     // GM Tube events
unsigned long cpm;        // counts per minute
unsigned long prevMillis;  

WiFiClient espClient;
PubSubClient client(espClient);


ICACHE_RAM_ATTR void tube_impulse(){       //subprocedure for capturing events from Geiger Kit
  counts++;
}

void setup() {
  Serial.begin(115200);
  
  counts = 0;
  cpm = 0;
  multiplier = MAX_PERIOD / LOG_PERIOD;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  debug("\n\r \n\rConnecting to Wifi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    debug(".");
  }
  
  debugln(WIFI_SSID);
  debug("IP address: ");
  debugln(WiFi.localIP());
   
  client.setServer(MQTT_SERVER, MQTT_PORT);
 
  while (!client.connected()) {
    debug("Attempting MQTT connection...");
    if (client.connect("BR-9B", MQTT_USER, MQTT_PASS)) {
      debugln("connected");
    } else {
      debug("failed, rc=");
      debug(client.state());
      debugln(" try again in 5 seconds");
            
      delay(5000);
    }
  }

  pinMode(IMPULSE_PIN, INPUT_PULLUP);
  
  // external interrupt for GM impulse event
  attachInterrupt(IMPULSE_PIN, tube_impulse, FALLING); 
  
  #if DEBUG == 1
  randomSeed(151);
  #endif
}

void loop() {
  unsigned long currentMillis = millis();
  if(currentMillis - prevMillis > LOG_PERIOD) {
    
    prevMillis = currentMillis;
    cpm = counts * multiplier;
    
    #if DEBUG == 1
    cpm = random(10,50);
    #endif

    float usvh = (float)cpm/GM_INDEX;

    String json = "{ \"cpm\": "+String(cpm)+", \"usvh\": "+String(usvh)+" }";

    client.publish("dosimeter", json.c_str());
    debug("published: ");
    debugln(json);

    counts = 0;
  }
  client.loop();

}
