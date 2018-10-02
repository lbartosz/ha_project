#include <OneWire.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include "secrets.h"


///////////////////////////////////////////
/////// DEBUG LOGGING /////////////////
// #define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTDEC(x) Serial.print(x, DEC)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
#endif // DEBUG

///////////////////////////////////////////
/////// NODE CONFIGURATION /////////////////
// uncomment correct define

// LOCATION
// #define KITCHEN
// #define LIVING_ROOM
#define WARDROBE

// SENSOR TYPES
// #define DHT_11
#define DHT_22

// END OF NODE CONFIGURATION //////////////


#if defined(KITCHEN)
  #define nodeId "kitchen"
  #define tHumidity "sensor/kitchenHumidity"
  #define tTempIn "sensor/kitchenTempIn"
  #define tTempOut "sensor/kitchenTempOut"
#elif defined(LIVING_ROOM)
  #define nodeId "livingroom"
  #define tHumidity "sensor/livingroomHumidity"
  #define tTempIn "sensor/livingroomTempIn"
  #define tTempOut "sensor/livingroomTempOut"
#elif defined(WARDROBE)
  #define nodeId "wardrobe"
  #define tHumidity "sensor/wardrobeHumidity"
  #define tTempIn "sensor/wardrobeTempIn"
  #define tTempOut "sensor/wardrobeTempOut"
#endif


///////////////////////////////////////////
// HW CONFIG //////////////////////////////
#if defined(DHT_11)
  #define DHTTYPE DHT11
#elif defined(DHT_22)
  #define DHTTYPE DHT22
#endif

#define DHTPIN 2
#define ONE_WIRE_BUS 4

///////////////////////////////////////////
// OTHER DEFINITIONS
#if defined(DHT_11)
  #define minDiffTempDHT 1.0
  #define minDiffHumDHT 4.0
#elif defined(DHT_22)
  #define minDiffTempDHT 0.2
  #define minDiffHumDHT 1.5
#endif

#define minDiffTempDS 0.2

///////////////////////////////////////////
// FUNCTIONS DECLARATIONS ////////////////
void setup_wifi();
void reconnect();
bool hasValueChanged(float, float, float);


//////////////////////////////////////////
// GLOBALS AND STARTING VALUES //////////
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE, 15);
OneWire onewire(ONE_WIRE_BUS);
DallasTemperature sensors(&onewire);

long lastMsg = 0;
float inTemp = 0.0;
float inHumidity = 0.0;
float outTemp = 0.0;

///////////////////////////////////////////
// SYSTEM SETUP //////////////////////////
void setup() {
  dht.begin();
  sensors.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  #ifdef DEBUG
    Serial.begin(115200);
  #endif  // DEBUG
}

///////////////////////////////////////////
// MAIN LOOP //////////////////////////
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;


    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    sensors.requestTemperatures();
    delay(500);
    float new_outtemp = sensors.getTempCByIndex(0);
    
    // read inside tempratature
    if (hasValueChanged(newTemp, inTemp, minDiffTempDHT)) {
      inTemp = newTemp;
      DEBUG_PRINT("New temperature:");
      DEBUG_PRINTLN(String(inTemp).c_str());
      client.publish(tTempIn, String(inTemp).c_str(), true);
    }

    // read inside humidity
    if (hasValueChanged(newHum, inHumidity, minDiffHumDHT)) {
      inHumidity = newHum;
      DEBUG_PRINT("New humidity:");
      DEBUG_PRINTLN(String(inHumidity).c_str());
      client.publish(tHumidity, String(inHumidity).c_str(), true);
    }

    // read outside temperature
    if (hasValueChanged(new_outtemp, outTemp, minDiffTempDS)) {
      outTemp = new_outtemp;
      DEBUG_PRINT("Outside temperature:");
      DEBUG_PRINTLN(outTemp);
      client.publish(tTempOut, String(outTemp).c_str(), true);
    }
  }
}

/////////////////////////////////////
// FUNCTIONS DEFINITIONS ////////////
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  DEBUG_PRINT("Connecting to: ");
  DEBUG_PRINTLN(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {  
    delay(500);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(nodeId, mqtt_user, mqtt_pass)) {
      DEBUG_PRINTLN("connected");
    }
    else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(client.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool hasValueChanged(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
    (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}
