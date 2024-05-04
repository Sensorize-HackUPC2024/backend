// TO.DO
// Cables schema
// Read interact 
// Send data 






// Importing libraries
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Define Sensors
#define SS_PIN 5
#define RST_PIN 2
#define Green_rfid 26
#define Red_rfid 25
#define Buzzer 33
#define Rele_rfid 27
#define Pir_detector 12
#define DHTPIN 4
  //Paso a paso sensors
  #define IN1 32
  #define IN2 35
  #define IN3 34
  #define IN4 14

// Configurar conexión a MQTT
const char* ssid = "Bepes";
const char* password = "aleix1234567890";
const char* mqttServer = "172.20.10.5";
const int mqttPort = 1888;
const char* mqttUser = "public";
const char* mqttPassword = "public";


//In hex:  C4 AB C4 22
//In dec:  196 171 196 34

// Define variables
int lcdColumns = 16;
int lcdRows = 2;
int warm_up; // For sensor pir
byte LecturaUID[4];      
byte Usuario1[4]= {0xC4, 0xAB, 0xC4, 0x22}; // Usuari targeta 1   

//DHT CONFIG
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
//Declaring LCD (lcd)
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  
//Declaring RFID(rfid)
MFRC522 rfid(SS_PIN, RST_PIN); 
String rfidCard;
MFRC522::MIFARE_Key key; 
byte nuidPICC[4];
//Conect client
WiFiClient espClient;
PubSubClient client(espClient);
// Create callback to read information
void callback(char* topic, byte* payload, unsigned int length);



// Init array that will store new NUID 

void setup(){
  //Define Outputs
  pinMode (Green_rfid, OUTPUT);
  pinMode (Red_rfid, OUTPUT);
  pinMode (Buzzer, OUTPUT);
  pinMode (Rele_rfid, OUTPUT);
  pinMode (IN1, OUTPUT);
  pinMode (IN2, OUTPUT);
  pinMode (IN3, OUTPUT);
  pinMode (IN4, OUTPUT);

  //Define inputs
  pinMode(Pir_detector, INPUT);

  Serial.begin(9600); // open serial connection
  Serial.println(F("Starting the System!"));
  // LCD SETUP
  lcd.init();
  lcd.backlight();

  // RFID SETUP
  SPI.begin(); 
  rfid.PCD_Init(); 

  //DHT SETUP
  dht.begin();
  
  //MQTT SETUP
  WiFi.begin(ssid, password);
  Serial.println("...................................");
  Serial.print("Conectando a WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Conectado a la red WiFi");
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

    while (!client.connected()) {
    Serial.println("Conectando a MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      Serial.println("Conectado");
      client.subscribe("actuators/doorlock/1"); // Suscribirse al tema para recibir datos del actuador
    } else {
      Serial.print("Falló con estado ");
      Serial.print(client.state());
      delay(2000);
    }
  }


  // Warm Up
  Serial.print("All warm up :)");
}

void loop(){
  client.loop();
  waitEntry();
  checkCard();
  detectMotion();
  readT();
  readH();
  sendRFID(); //Testing, later put in waitEntry

  //sendDataToMQTT();

}
void sendRFID(){
// Look for new cards
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Concatenate UID bytes into a single string
  String UIDString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      UIDString += "0";
    }
    UIDString += String(rfid.uid.uidByte[i], HEX);
  }

  // Publish UID to MQTT topic
  client.publish("sensors/card_reader/1", UIDString.c_str());
  Serial.println("UID sent to MQTT: " + UIDString);

  // Halt for a moment to prevent multiple reads of the same card
  delay(1000);
}
void readT(){
  client.loop();
  float t = dht.readTemperature();
  if (!isnan(t)) {
    char str[16];
    sprintf(str, "%.2f", t);
    client.publish("sensors/temperature_sensor/1", str);
    Serial.println(str);
  }
  lcd.setCursor(0, 0);
  lcd.print("Temperature ");
  lcd.print(t);
  lcd.print("C");
  delay(1000);
  lcd.clear();
}
void readH(){
  client.loop();
  float h = dht.readHumidity();
  if (!isnan(h)) {
    char str[16];
    sprintf(str, "%.2f", h);
    client.publish("sensors/humidity/1", str);
    Serial.println(str);
  }
  lcd.setCursor(0, 0);
  lcd.print("Humidity ");
  lcd.print(h);
  lcd.print("%");
  delay(1000);
  lcd.clear();
}
void checkCard(){
  if ( ! rfid.PICC_IsNewCardPresent())   
    return;           
  
  if ( ! rfid.PICC_ReadCardSerial())     
    return; 
        Serial.print("UID:");       
    for (byte i = 0; i < rfid.uid.size; i++) { 
      if (rfid.uid.uidByte[i] < 0x10){   
        Serial.print(" 0");       
        }
        else{           
          Serial.print(" ");        
          }
          Serial.print(rfid.uid.uidByte[i], HEX);    
          LecturaUID[i]=rfid.uid.uidByte[i];          
          }
          
          Serial.print("\t");                     
                    
          if(comparaCard(LecturaUID, Usuario1)){    
            Serial.println("Bienvenido Aleix"); 
            correctEntry();
          }
           else           
            Serial.println("No reconegut");              
            incorrectEntry();
                  rfid.PICC_HaltA();

}

void waitEntry(){
  lcd.setCursor(0, 0);
  lcd.print("Put a card");
  delay(1000);
  lcd.clear();
}

void correctEntry(){
  lcd.setCursor(0, 0);
  lcd.print("Acces Guaranted");
  digitalWrite(Green_rfid, HIGH);
  digitalWrite(Buzzer, HIGH);
  digitalWrite(Rele_rfid, HIGH);
  delay(1000);
  lcd.clear();
  digitalWrite(Buzzer, LOW);
  digitalWrite(Green_rfid, LOW);
  digitalWrite(Rele_rfid, LOW);



}
void incorrectEntry(){
  lcd.setCursor(0, 0);
  lcd.print("Acces Denied");
  digitalWrite(Red_rfid, HIGH);
  delay(1000);
  lcd.clear();
  digitalWrite(Red_rfid, LOW);

}
void detectMotion(){
  client.loop();
  int pir_output;
  pir_output = digitalRead(Pir_detector);

    if( pir_output == LOW )
  {
    if( warm_up == 1 )
     {
      Serial.print("Warming Up\n\n");
      warm_up = 0;
      delay(2000);
    }
    int noDetectet = 0;
    char str[16];
    sprintf(str, "%d", noDetectet);
    client.publish("sensors/pir/1", str);
    Serial.println(str);
    Serial.print("No object in sight\n\n");
    delay(1000);
  }else
  {
    int detectet = 1;
    char str[16];
    sprintf(str, "%d", detectet);
    client.publish("sensors/pir/1", str);
    Serial.println(str);
    Serial.print("Object detected\n\n");    
    warm_up = 1;

    delay(1000);
  }

}


boolean comparaCard(byte lectura[],byte usuario[]) {
  for (byte i=0; i < rfid.uid.size; i++){    
  if(lectura[i] != usuario[i])        
    return(false);          
  }
  return(true);           
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

