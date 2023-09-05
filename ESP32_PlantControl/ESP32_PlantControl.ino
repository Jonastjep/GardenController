#include <DHT.h>
#include <DHT_U.h>

#include <Adafruit_BMP280.h>

#include <MQ135.h>
#include <Adafruit_BME280.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <WiFi.h>
#include <HTTPClient.h> 

String serv_key = "C4Xw4LhlIu";

const int lightRelPin = 26; // Light switching relay
const int humRelPin = 25;   // Humidifier switching relay
const int pumpRelPin = 33;  // Pump switching relay
const int auxRelPin = 32;   // Auxilary fan switching relay

const int exhFanPin = 18;  //exhaust fan pin
const int aux1FanPin = 19; //second aux fan (PWM)

const int gasPin = 35;   //gas (CO2) sensor pin (analog)
const int soilPin = 34; //soil sensor pin (analog)
const int airPin = 4;

//MQ135 SENSOR (CO2 SENSOR)
#define RZERO 76.63
MQ135 gasSensor = MQ135(gasPin, RZERO);

//DHT SENSOR (HUM AND TEMP)
#define DHTTYPE DHT11
DHT dht(airPin, DHTTYPE);

//millis conversions
unsigned long Millis2min = 60000;
unsigned long Millis2hrs = Millis2min * 60;
unsigned long millisInDay = 86400000;
 
// setting PWM properties
const int freq = 25000;
const int ledChannel_exh = 0;
const int ledChannel_aux1 = 1;
const int resolution = 8;
int exh_speed = 0;
int exh_n_speed = 0;
int aux1_speed = 0;
int aux1_n_speed = 0;

// JSON STATUS VARIABLES
int exh_stat = 0;
int hum_stat = 0;
int aux1_stat = 0;
int aux2_stat = 0;
int light_stat = 0;
int pump_stat = 0;
int temp_rec = 0;
int soil_rec = 0;
int co2_rec = 0;

int lightCyc = 18;
int pumpCyc = 20;
int aux1Cyc = 18;
int aux2Cyc = 18;

//WIFI SETTINGS
const char* ssid = "NetworkName";
const char* password =  "Password";

//Timer for the section that verifies new inputs through GET request
unsigned long timerDelay_GET = 500; //checks new input every half second
unsigned long lastTime_GET = 0;

//Timer for the section that sends sensor data with POST
unsigned long timerDelay_POST = 1000*20; //sends data to server every minute
unsigned long lastTime_POST = 0;

//Timer for the section that regulates light cycle
unsigned long lastTime_lightCycle = 0;
bool stage1_light = true;

//Timer for the section that regulates pump cycle
unsigned long timerDelay_pumpCycle = 1000*3; //How long the pumping lasts
unsigned long timerDelay_eqCycle = 1000*9;
unsigned long lastTime_pumpCycle = 0;
unsigned long lastTime_eqCycle = 0; //time to wait for the soil to wet before allowing more pumping
bool pumping = false;
bool soil_eq = false;

//booleans for soil and air humidity
bool air_humi = false;
bool soil_humi = false;

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
  ledcSetup(ledChannel_exh, freq, resolution);
  ledcSetup(ledChannel_aux1, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(exhFanPin, ledChannel_exh);
  ledcAttachPin(aux1FanPin, ledChannel_aux1);

  ledcWrite(ledChannel_exh, exh_speed);
  ledcWrite(ledChannel_aux1, aux1_speed);
 
}

void loop() {
  if ((millis() - lastTime_GET) > timerDelay_GET) {

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


          exh_n_speed = (int) map(doc["exh_stat"],0,100,0,255); // converts percentages to pwm values (8bit)
          aux1_n_speed = (int) map(doc["aux1_stat"],0,100,0,255);
        }
  
      else {
        Serial.println("Error on HTTP request");
      }
  
      http.end();
    }
  
    check_status(aux2_stat, auxRelPin); //checks the state of the relay for the 2nd auxilary fan

    //MODIFYING PWM FAN SPEEDS
    if (exh_n_speed != exh_speed){
      exh_speed = exh_n_speed;
      ledcWrite(ledChannel_exh, exh_speed);
      delay(15);
    }

    if (aux1_n_speed != aux1_speed){
      aux1_speed = aux1_n_speed;
      ledcWrite(ledChannel_aux1, aux1_speed);
      delay(15);
    }

    lastTime_GET = millis();

  }

  if ((millis() - lastTime_POST) > timerDelay_POST) {
    if ((WiFi.status() == WL_CONNECTED)) { 

      //CO2
      float co2_value;
      co2_value = gasSensor.getPPM();
      //SOIL HUM
      int soil_value;
      soil_value = ( 100 - ( (analogRead(soilPin)/4095.00) * 100 ) );
      // soil_value = analogRead(soilPin);
      //AIR HUM
      float h = dht.readHumidity();
      float t = dht.readTemperature();
  
      HTTPClient http;
  
      http.begin("http://192.168.1.88:5000/sensor_data");                                   

      http.addHeader("Content-Type", "application/json");

      String data_json;
      data_json = "{\"key\": \"" + serv_key + "\",\"sensor_data\": {\"DHT_hum\": " + h + ", \"DHT_temp\": " + t + ", \"SOIL_hum\": " + soil_value + ", \"MQ135\": " + co2_value + "}}";
      Serial.println(data_json);
      int httpResponseCode = http.POST(data_json);
    }
    lastTime_POST = millis();
  }

  // LIGHT CYCLE SETUP
  // unsigned long intervalS1 = lightCyc * Millis2hrs;
  // unsigned long intervalS2 = millisInDay - intervalS1;

  unsigned long intervalS1 = lightCyc * 1000;
  unsigned long intervalS2 = (24-lightCyc) * 1000;

  if (stage1_light && (light_stat==1)){
    if ((millis() - lastTime_lightCycle) > intervalS1){
      lastTime_lightCycle = millis();

      stage1_light = !stage1_light;
      
      digitalWrite(lightRelPin, LOW);
    }
  } 
  else if(light_stat==1){
    if ((millis() - lastTime_lightCycle) > intervalS2){
      lastTime_lightCycle = millis();

      stage1_light = !stage1_light;
      
      digitalWrite(lightRelPin, HIGH);
    }
  }
  else{ // if light_stat is OFF, then turn all light off and reset cycle
    digitalWrite(lightRelPin, LOW);
    lastTime_lightCycle = millis();
  }

  // AIR HUMIDITY CYCLE
  if(dht.readHumidity() < (hum_stat-5)){
    digitalWrite(humRelPin, LOW);
  }
  else if(dht.readHumidity() > (hum_stat+5) ){
    digitalWrite(humRelPin, HIGH);
  }

  //SOIL HUMIDITY CYCLE
  if( ((100-((analogRead(soilPin)/4095.00)*100)) < pumpCyc) && !pumping && !soil_eq){
    pumping = true;
    lastTime_pumpCycle = millis();
    digitalWrite(pumpRelPin, LOW); //turn pump on
  }
  if ((millis() - lastTime_pumpCycle) > timerDelay_pumpCycle && pumping) {
    pumping = false;
    soil_eq = true;
    lastTime_eqCycle = millis();
    digitalWrite(pumpRelPin, HIGH); //turn pump off
  }
  if ((millis() - lastTime_eqCycle) > timerDelay_eqCycle && soil_eq) {
    soil_eq = false;
  }

}

void check_status(int stat, int pin){
  if(stat){
    digitalWrite(pin, LOW);
  }else{
    digitalWrite(pin, HIGH);
  }
}