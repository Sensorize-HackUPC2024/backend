#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"



//define pins 

#define DHTPIN 4     
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);



//Configuring esp to MQTT

const char* ssid = "Bepes";
const char* password = "aleix1234567890";
const char* mqttServer = "172.20.10.5";
const int mqttPort = 1888;
const char* mqttUser = "public";
const char* mqttPassword = "public";

// Conect client

WiFiClient espClient;
PubSubClient client(espClient);


void setup() { 
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("...................................");
  Serial.print("Connecting to WiFi.");

  while (WiFi.status() != WL_CONNECTED){
          delay(500);
          Serial.print(".") ;
  }

  Serial.println("Connected to the WiFi network");

  client.setServer(mqttServer, mqttPort);
  while (!client.connected()){ 
         Serial.println("Connecting to MQTT...");
       if (client.connect("ESP32Client", mqttUser, mqttPassword ))
           Serial.println("connected");
       else
       {   Serial.print("failed with state ");
           Serial.print(client.state());
           delay(2000);
       }
  }


//---------------------------------
  dht.begin();

}
void loop(){  
  client.loop();
     float t = dht.readTemperature();

     char str[16];
     sprintf(str, "%f", t);
       float h = dht.readHumidity();
  // Read temperature as Celsius (the default)


     client.publish("sensors/temperature_sensor/1",str); //Sento to test data
     Serial.println(str);
     delay(500);
     int test;

     subscribeReceive("actuators/1",,2);
 }
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en el topic: ");
  Serial.println(topic);

  Serial.print("Contenido del mensaje: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}