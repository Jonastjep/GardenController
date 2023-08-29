#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <WiFi.h>
#include <HTTPClient.h> 

const int lightRelPin = 26;
const int fanRelPin = 25;
const int humRelPin = 33;
const int pumpRelPin = 32;

const int fanPin = 27;  //exhaust fan pin

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
unsigned long timerDelay = 100;

int light_status = 0;
int fan_status = 0;
int pump_status = 0;
int hum_status = 0;

int fan_speed = 0;
int n_speed = 0;

void check_status(int stat, int pin);

void setup() {
  //setting the relay pins
  pinMode(lightRelPin, OUTPUT);
  pinMode(fanRelPin, OUTPUT);
  pinMode(humRelPin, OUTPUT);
  pinMode(pumpRelPin, OUTPUT);
  digitalWrite(lightRelPin, HIGH);
  digitalWrite(fanRelPin, HIGH);
  digitalWrite(humRelPin, HIGH);
  digitalWrite(pumpRelPin, HIGH);

  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");

  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(fanPin, ledChannel);
  ledcWrite(ledChannel, fan_speed);
 
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if ((WiFi.status() == WL_CONNECTED)) { 
  
      HTTPClient http;
  
      http.begin("http://192.168.1.73:5000/get_data");
      int httpCode = http.GET();                                        
  
      if (httpCode > 0) {
  
          String payload = http.getString();
          // Serial.println(httpCode);
          // Serial.println(payload);

          StaticJsonDocument<256> doc;

          DeserializationError error = deserializeJson(doc, payload);

          if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
          }

          light_status = doc["light_status"];
          fan_status = doc["fan_status"];
          hum_status = doc["hum_status"];
          pump_status = doc["pump_status"];

          n_speed = (int) map(doc["exh_speed"],0,100,0,255); // converts percentages to pwm values (8bit)
          
          Serial.print(light_status);
          Serial.print(fan_status);
          Serial.print(hum_status);
          Serial.println(pump_status);
        }
  
      else {
        Serial.println("Error on HTTP request");
      }
  
      http.end();
    }
  
    lastTime = millis();
  }

  check_status(light_status, lightRelPin);
  check_status(fan_status, fanRelPin);
  check_status(hum_status, humRelPin);
  check_status(pump_status, pumpRelPin);


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
