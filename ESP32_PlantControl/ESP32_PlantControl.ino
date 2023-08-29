#include <DHT.h>
#include <DHT_U.h>

#include <Adafruit_BMP280.h>

#include <MQ135.h>
#include <Adafruit_BME280.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <WiFi.h>
#include <HTTPClient.h> 

const int lightRelPin = 26; // Light switching relay
const int humRelPin = 25;   // Humidifier switching relay
const int pumpRelPin = 33;  // Pump switching relay
const int auxRelPin = 32;   // Auxilary fan switching relay

const int exhFanPin = 18;  //exhaust fan pin
const int gasPin = 35;   //gas (CO2) sensor pin (analog)
const int soilPin = 34; //soil sensor pin (analog)
const int airPin = 4;

//MQ135 SENSOR (CO2 SENSOR)
#define RZERO 76.63
MQ135 gasSensor = MQ135(gasPin, RZERO);

//DHT SENSOR (HUM AND TEMP)
#define DHTTYPE DHT11
DHT dht(airPin, DHTTYPE);
 
// setting PWM properties
const int freq = 25000;
const int ledChannel = 0;
const int resolution = 8;


const char* ssid = "***REMOVED***";
const char* password =  "***REMOVED***";


unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 1000;

int exh_stat = 0;
int hum_stat = 0;
int aux1_stat = 0;
int aux2_stat = 0;
int light_stat = 0;
int pump_stat = 0;
int temp_rec = 0;
int soil_rec = 0;
int co2_rec = 0;

int lightCyc = 1;
int pumpCyc = 20;
int aux1Cyc = 1;
int aux2Cyc = 1;

int fan_speed = 0;
int n_speed = 0;

void check_status(int stat, int pin); //function declaration

void setup() {

  Serial.begin(115200);

  //setting the relay pins
  pinMode(lightRelPin, OUTPUT);
  pinMode(auxRelPin, OUTPUT);
  pinMode(humRelPin, OUTPUT);
  pinMode(pumpRelPin, OUTPUT);
  digitalWrite(lightRelPin, HIGH);
  digitalWrite(auxRelPin, HIGH);
  digitalWrite(humRelPin, HIGH);
  digitalWrite(pumpRelPin, HIGH);
  
  //TEMP AND HUM SENSOR
  dht.begin();

  //WIFI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  //FAN PWM 
  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(exhFanPin, ledChannel);
  ledcWrite(ledChannel, fan_speed);
 
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {

    //sensor tests

    //CO2
    float co2_value;
    co2_value = gasSensor.getPPM();
    Serial.print("CO2 ppm: ");
    Serial.println(co2_value);

    //SOIL HUM
    // soil_value = ( 100 - ( (analogRead(soilPin)/4095.00) * 100 ) );
    int soil_value;
    soil_value = analogRead(soilPin);
    Serial.print("Soil hum: ");
    Serial.println(soil_value);

    //AIR HUM
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.println(t);

    if ((WiFi.status() == WL_CONNECTED)) { 
  
      HTTPClient http;
  
      http.begin("http://192.168.1.88:5000/get_data");
      int httpCode = http.GET();                                        
  
      if (httpCode > 0) {
  
          String payload = http.getString();
          // Serial.println(httpCode);
          // Serial.println(payload);

          StaticJsonDocument<384> doc;

          DeserializationError error = deserializeJson(doc, payload);

          if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
          }

          exh_stat = doc["exh_stat"];
          hum_stat = doc["hum_stat"];
          aux1_stat = doc["aux1_stat"];
          aux2_stat = doc["aux2_stat"];
          light_stat = doc["light_stat"];
          pump_stat = doc["pump_stat"];
          temp_rec = doc["temp_rec"];
          soil_rec = doc["soil_rec"];
          co2_rec = doc["co2_rec"];

          lightCyc = doc["lightCyc"];
          pumpCyc = doc["pumpCyc"];
          aux1Cyc = doc["aux1Cyc"];
          aux2Cyc = doc["aux2Cyc"];


          n_speed = (int) map(doc["exh_stat"],0,100,0,255); // converts percentages to pwm values (8bit)
          
          Serial.print(light_stat);
          Serial.print(aux2_stat);
          Serial.print(hum_stat);
          Serial.println(pump_stat);
          Serial.println();
        }
  
      else {
        Serial.println("Error on HTTP request");
      }
  
      http.end();
    }
  
    lastTime = millis();
  }

  check_status(light_stat, lightRelPin);
  check_status(aux2_stat, auxRelPin);
  check_status(hum_stat, humRelPin);
  check_status(pump_stat, pumpRelPin);


  if (n_speed != fan_speed){
    fan_speed = n_speed;
    ledcWrite(ledChannel, fan_speed);
    delay(15);
    // Serial.println("Speed modified.");
  }
}

void check_status(int stat, int pin){
  if(stat){
    digitalWrite(pin, LOW);
  }else{
    digitalWrite(pin, HIGH);
  }
}