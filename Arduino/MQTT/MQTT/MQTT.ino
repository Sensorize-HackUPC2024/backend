#include <WiFi.h>
#include <PubSubClient.h>

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
}
void loop(){  
  client.loop();

     char str[16];
     sprintf(str, "%u", random(100));

     client.publish("test", str); //Sento to test data
     Serial.println(str);
     delay(500);
 }