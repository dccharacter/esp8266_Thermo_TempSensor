/*
  ESP8266 --> ThingSpeak Channel via MKR1000 Wi-Fi

  This sketch sends the Wi-Fi Signal Strength (RSSI) of an ESP8266 to a ThingSpeak
  channel using the ThingSpeak API (https://www.mathworks.com/help/thingspeak).

  Requirements:

     ESP8266 Wi-Fi Device
     Arduino 1.6.9+ IDE
     Additional Boards URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
     Library: esp8266 by ESP8266 Community

  ThingSpeak Setup:

     Sign Up for New User Account - https://thingspeak.com/users/sign_up
     Create a new Channel by selecting Channels, My Channels, and then New Channel
     Enable one field
     Note the Channel ID and Write API Key

  Setup Wi-Fi:
    Enter SSID
    Enter Password

  Tutorial: http://nothans.com/measure-wi-fi-signal-levels-with-the-esp8266-and-thingspeak

  Created: Feb 1, 2017 by Hans Scharler (http://nothans.com)
*/
#include<EthernetClient.h>
#include <ESP8266WiFi.h>
#include "config.h"

#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT11   // DHT 11

WiFiClient client;

// ThingSpeak Settings
String server = "api.thingspeak.com";
const unsigned int moduleID = 1;

unsigned long lastUpdateTime = 0; // Track the last update time using millis()
const unsigned long updateInterval = 60 * 1000; // Frequency to update

/** Pin number for DHT11 1 data pin */
int dhtPin1 = 2;
/** Initialize DHT sensor 1 */
DHT dht = DHT(dhtPin1, DHTTYPE);

void setup() {
  pinMode(dhtPin1, INPUT_PULLUP);
  
  Serial.begin(9600);

  Serial.println("Initializing DHT");
  // Initialize temperature sensor 1
  dht.begin();
  Serial.println("Sensor successfully Initialized");

  

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Still connecting to WiFi...");
    delay(500);
  }
  Serial.println("Connected to WiFi");

  // %TODO: send module ID to "reboot" field to signal reboot - all modules can report reboots to the same field
}

void loop() {
  if (millis() - lastUpdateTime >=  updateInterval) {
    Serial.println("Sensor 1 data:");
    float temperature = dht.readTemperature(1) - 2.3;
    Serial.println("Temp: " + String(temperature));
    float humidity = dht.readHumidity();
    Serial.println("Humidity: " + String(humidity) + "%");
    
    String jsonBuffer = ",\"updates\":[{\"delta_t\":\"0\",\"field" + String(moduleID) + "\":\"" + String(temperature)+ "\""; //start data set
    
    int resp = 0;
    do {
      jsonBuffer += "}]";
      httpRequest(jsonBuffer);
      delay(250); //Wait to receive the response
      client.parseFloat();
      resp = client.parseInt();
      Serial.println("Response is " + String(resp));
    } while (resp != 202);
    
    lastUpdateTime = millis(); // Update the last update time
  }
}

void httpRequest(String jsonBuffer) {
  /* JSON format for data buffer in the API
      This example uses the relative timestamp as it uses the "delta_t". If your device has a real-time clock, you can also provide the absolute timestamp using the "created_at" parameter
      instead of "delta_t".
       "{\"write_api_key\":\"YOUR-CHANNEL-WRITEAPIKEY\",\"updates\":[{\"delta_t\":0,\"field1\":-60},{\"delta_t\":15,\"field1\":200},{\"delta_t\":15,\"field1\":-66}]
  */
  // Format the data buffer as noted above
  String data = "{\"write_api_key\":\"" + writeAPIKey + "\""; //Replace YOUR-CHANNEL-WRITEAPIKEY with your ThingSpeak channel write API key
  data += jsonBuffer;
  data += "}\0";
  Serial.println("Data is: " + data);
  // Close any connection before sending a new request
  client.stop();
  int data_length = data.length() + 1; //Compute the data buffer length
  // POST data to ThingSpeak
  if (client.connect(server, 80)) {
    client.println("POST /channels/" + channelID + "/bulk_update.json HTTP/1.1"); //Replace YOUR-CHANNEL-ID with your ThingSpeak channel ID
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(data_length));
    client.println();
    char bufChar[500]; // if data is not converted to Char Array, then the encoding is not correct; server cannot read the API key and returns 401 - unauthorized
    data.toCharArray(bufChar, data_length);
    client.println(bufChar);
    Serial.println(bufChar);
  }
}


