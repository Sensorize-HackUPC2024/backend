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
#define IN1 32
#define IN2 35
#define IN3 34
#define IN4 14

// MQTT Configuration
const char* ssid = "Bepes";
const char* password = "aleix1234567890";
const char* mqttServer = "172.20.10.5";
const int mqttPort = 1888;
const char* mqttUser = "public";
const char* mqttPassword = "public";

// Define variables
int lcdColumns = 16;
int lcdRows = 2;
int warm_up; // For PIR sensor
byte LecturaUID[4];      
byte Usuario1[4]= {0xC4, 0xAB, 0xC4, 0x22}; // User card 1   

// DHT Configuration
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Declaring LCD (lcd)
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

// Declaring RFID (rfid)
MFRC522 rfid(SS_PIN, RST_PIN); 
String rfidCard;

// Client setup
WiFiClient espClient;
PubSubClient client(espClient);
bool accesState = false;

// Callback function to read information
volatile int doorlockState = -1;

void callback(char* topic, byte* payload, unsigned int length) {
Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Serial.print(" "); // Print a space for clarity
    Serial.println(payload[i]); // Print the byte value
  }

  Serial.print("Length: ");
  Serial.println(length);

  // Reset doorlockState before checking payload
  doorlockState = -1;

  for (int i = 0; i < length; i++) {
    if ((char)payload[i] == '1') {
      doorlockState = 1; // Set doorlockState to 1 when payload is '1'
      break;
    } else if ((char)payload[i] == '0') {
      doorlockState = 0; // Set doorlockState to 0 when payload is '0'
      break;
    }
  }

  Serial.println();
}

void setup(){
  // Define Outputs
  pinMode(Green_rfid, OUTPUT);
  pinMode(Red_rfid, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(Rele_rfid, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Define Inputs
  pinMode(Pir_detector, INPUT);

  Serial.begin(9600); // Open serial connection
  Serial.println(F("Starting the System!"));

  // LCD SETUP
  lcd.init();
  lcd.backlight();

  // RFID SETUP
  SPI.begin(); 
  rfid.PCD_Init(); 

  // DHT SETUP
  dht.begin();
  
  // MQTT SETUP
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi network");
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // MQTT connection
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      Serial.println("Connected to MQTT");
      client.subscribe("actuators/doorlock/1"); // Subscribe to the topic to receive data from the actuator
    } else {
      Serial.print("Failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Warm Up
  Serial.println("System warmed up!");
}

void requestCard(){
  lcd.setCursor(0, 0);
  lcd.print("Scan a valid Card");
  delay(1000);
  lcd.clear();
}

void sendDataToMQTT(){
  sendT();
  sendH();
}

void sendT(){
  float t = dht.readTemperature();
  if (!isnan(t)) {
    char str[16];
    sprintf(str, "%.2f", t);
    client.publish("sensors/temperature_sensor/1", str);
    Serial.println(str);
  }
}

void sendH(){
  float h = dht.readHumidity();
  if (!isnan(h)) {
    char str[16];
    sprintf(str, "%.2f", h);
    client.publish("sensors/humidity/1", str);
    Serial.println(str);
  }
}
void readT(){
  float t = dht.readTemperature();
  lcd.setCursor(0, 0);
  lcd.print("Temperature ");
  lcd.print(t);
  lcd.print("C");
  delay(2000);
  lcd.clear();
}

void readH(){
  float h = dht.readHumidity();
  lcd.setCursor(0, 0);
  lcd.print("Humidity ");
  lcd.print(h);
  lcd.print("%");
  delay(2000);
  lcd.clear();
}
void waitEntry(){
  lcd.setCursor(0, 0);
  lcd.print("Put a card");
  delay(2000);
  lcd.clear();
}

void guaranteedAccess(){
  lcd.setCursor(0, 0);
  lcd.print("Access Granted");
  digitalWrite(Green_rfid, HIGH);
  digitalWrite(Buzzer, HIGH);
  digitalWrite(Rele_rfid, HIGH);
  delay(2000);
  lcd.clear();
  digitalWrite(Buzzer, LOW);
  digitalWrite(Green_rfid, LOW);
  digitalWrite(Rele_rfid, LOW);
}

void deniedAccess(){
  lcd.setCursor(0, 0);
  lcd.print("Access Denied");
  digitalWrite(Buzzer, HIGH);
  digitalWrite(Red_rfid, HIGH);
  delay(2000);
  digitalWrite(Buzzer, LOW);
  lcd.clear();
  digitalWrite(Red_rfid, LOW);
}

void detectMotion(){
  client.loop();
  int pir_output;
  pir_output = digitalRead(Pir_detector);

  if (pir_output == LOW) {
    if (warm_up == 1) {
      Serial.println("Warming Up\n\n");
      warm_up = 0;
      delay(2000);
    }
    int noDetected = 0;
    char str[16];
    sprintf(str, "%d", noDetected);
    client.publish("sensors/pir/1", str);
    Serial.println(str);
    Serial.println("No object in sight\n\n");
    delay(1000);
  } else {
    int detected = 1;
    char str[16];
    sprintf(str, "%d", detected);
    client.publish("sensors/pir/1", str);
    Serial.println(str);
    Serial.println("Object detected\n\n");    
    warm_up = 1;
    delay(1000);
  }
}
void sendCardToMQTT(String card) {
  client.publish("sensors/card_reader/1", card.c_str());
  Serial.println("RFID Card sent to MQTT: " + card);
}

String readRFID() {
  // Check if a card is present
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Build the card ID as a string
    String cardID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) {
        cardID += "0";
      }
      cardID += String(rfid.uid.uidByte[i], HEX);
    }
    return cardID;
  } else {
    return ""; // Return an empty string if no card is detected
  }
}
void waitingScreen(){
  lcd.setCursor(0, 0);
  lcd.print("----WAITING----");
  delay(1000);
  lcd.clear();
}

void checkCorrect(String card){
  sendCardToMQTT(card);
  waitingScreen();
  // Wait for the actuator response
  unsigned long startTime = millis();
  while ((millis() - startTime) < 5000) {
    client.loop();
    if (doorlockState != -1) {
      accesState = doorlockState == 1; // Asignar el valor de doorlockState a accesState
      return; // Salir del bucle una vez que se ha recibido una respuesta
    }
    delay(100); // Esperar 100 milisegundos antes de volver a verificar
  }
  // Si no se recibe respuesta después de 5 segundos, asumir que el acceso es denegado
  accesState = true;
}

void loop(){
  client.loop();
  requestCard();
  String card = readRFID();
  delay(2000);
  if (card != "") {
    checkCorrect(card); // Check if the card is correct

    if (accesState) {
      guaranteedAccess();
    } else {
      deniedAccess();
    }
  } else {
    readT();
    readH();
  }
  detectMotion();
  sendDataToMQTT();
}
