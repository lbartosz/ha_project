#include <OneWire.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include "secrets.h"


///////////////////////////////////////////
/////// DEBUG LOGGING /////////////////
#define DEBUG

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

// SWITCH ID
#define TEST_NODE
// #define OTHER

#if defined(TEST_NODE)
#define nodeId "test_node"
#elif defined(OTHER)
#define nodeId "livingroom"
#endif
// END OF NODE CONFIGURATION //////////////

///////////////////////////////////////////
// HW CONFIG //////////////////////////////
#define RELAY_TRIGGER_PIN 2
#define RELAY_STATUS_PIN 4


///////////////////////////////////////////
// OTHER DEFINITIONS

///////////////////////////////////////////
// FUNCTIONS DECLARATIONS ////////////////
void setup_wifi();
void mqtt_connect();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void trigger_relay(unsigned int pin);



//////////////////////////////////////////
// GLOBALS AND STARTING VALUES //////////
WiFiClient espClient;
PubSubClient client(espClient);

unsigned int relay_status;
unsigned int current_status;

///////////////////////////////////////////
// SYSTEM SETUP //////////////////////////
void setup() {

#ifdef DEBUG
	Serial.begin(115200);
#endif  // DEBUG

	setup_wifi();
	client.setServer(mqtt_server, 1883);
	client.setCallback(mqtt_callback);

	pinMode(RELAY_TRIGGER_PIN, OUTPUT);
	digitalWrite(RELAY_TRIGGER_PIN, HIGH);

	pinMode(RELAY_STATUS_PIN, INPUT_PULLUP);
	relay_status = digitalRead(RELAY_STATUS_PIN);
}

///////////////////////////////////////////
// MAIN LOOP //////////////////////////
void loop() {
	if (!client.connected()) {
		mqtt_connect();
	}

	client.loop();

	current_status = digitalRead(RELAY_STATUS_PIN);
	if (relay_status != current_status) {
		relay_status = current_status;
		if (current_status == LOW) {
			client.publish("test/knefel/status", "ON", true);
		}
		else
		{
			client.publish("test/knefel/status", "OFF", true);
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

void mqtt_connect() {
	// Loop until we're reconnected
	while (!client.connected()) {
		DEBUG_PRINT("Attempting MQTT connection...");
		// Attempt to connect
		if (client.connect(nodeId, mqtt_user, mqtt_pass)) {
			DEBUG_PRINTLN("connected");
			client.subscribe("test/knefel/set");
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

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
	DEBUG_PRINT("Message on topic: ");
	DEBUG_PRINTLN(topic);
	trigger_relay(RELAY_TRIGGER_PIN);

}

void trigger_relay(unsigned int pin) {
	DEBUG_PRINT("Triggering relay on PIN: ");
	DEBUG_PRINTLN(pin);
	digitalWrite(pin, LOW);
	delay(100);
	digitalWrite(pin, HIGH);
}