/*
  ESP8266 watering system originally based on Rui Santos
  https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/
*/
/******************** LIBRARIES **********************/
// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Wire.h>
#include <DHT.h>
#include <uRTCLib.h>
#include "Day.h"

/******************** DEFINES ************************/
//GPIO Pins - OUTPUT
#define PUMP 14
#define SPRAY 13
#define MIST 12
#define WIFI_OK 16
#define ESPLED 2

//GPIO Pins - INPUT
#define WATER_LEVEL 10

//Data pins
#define RTC_SDA 4
#define RTC_SCL 5

#define DHT_DEV 0
#define DHTTYPE DHT22

/******************** OBJECTS ************************/
// Instantiate the DHT sensor object
DHT sensorOne(DHT_DEV, DHTTYPE);

// Instantiate the uRTCLib rtc object with device address of 0x68
uRTCLib rtc(0x68);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Array of days defining the week
Day *allDay = new Day[7];

/******************** GLOBALS ************************/
// Loop time keeping instead of using delays
unsigned long previousMillis = 0, currentMillis = 0;  // will store last time DHT was updated
long interval = 1000;                                 // interval at which to flash the LED on GPIO (milliseconds)

// SSID and PSK - Replace with your network credentials -- Yeah yeah, I know - tisk tisk!
String ssid = "SSID";
String password = "PSK";

String sprayState, mistState;                         // Stores spraying state
int heartBeat = LOW;                                  // Stores LED Flashing state state 

/******************** METHODS ************************/
String getTemperature(){
  float temperature = sensorOne.readTemperature();
  return String(temperature);
}
  
String getHumidity(){
  float humidity = sensorOne.readHumidity();
  return String(humidity);
}

// Replaces placeholder with state values
String processor(const String& var){
//  Serial.println("WEB_VAR :: " + var);
  if(var == "S_STATE"){
    if(digitalRead(SPRAY)){
      sprayState = "ON";
    }else{
      sprayState = "OFF";
    }
//    Serial.println("WEB_STATE :: " + sprayState);
    return sprayState;
  }
  else if(var == "M_STATE"){
    if(digitalRead(MIST)){
      mistState = "ON";
    }else{
      mistState = "OFF";
    }
  //    Serial.println("WEB_STATE :: " + mistState);
    return mistState;
  }else if (var == "TEMP"){
    return getTemperature();
  }else if (var == "HUMID"){
    return getHumidity();
  } 
}

// This method is to split up the string that is received from the webform. It uses a combination
// of find_first_not_of()and find() functions to split up the string.
// Found from here: https://www.techiedelight.com/split-string-cpp-using-delimiter/
void tokenize(std::string const &str, const char delim, std::vector<std::string> &out){
  size_t start;
  size_t end = 0;
  while ((start = str.find_first_not_of(delim, end)) != std::string::npos){
    end = str.find(delim, start);
    out.push_back(str.substr(start, end - start));
  }
}

// We take a string as a perameter and split it based on that it's a comma delimited string, and 
// pump it into the array for a given day[i]-1
void storeFormValues(std::string &data){
  std::vector<std::string> out;
  tokenize(data, ',', out);
  
  uint8_t dayt = atoi(out[0].c_str());
  bool dayActive = (out[1].compare("true") < 0) ? true : false;
  bool mistActive = (out[2].compare("true") < 0) ? true : false;
  bool sprayActive = (out[3].compare("true") < 0) ? true : false;
  uint8_t startTime_H = atoi(out[4].c_str());
  uint8_t startTime_M = atoi(out[5].c_str());
  uint8_t mistDuration = atoi(out[6].c_str()); 
  uint8_t sprayDuration = atoi(out[7].c_str());
  
  allDay[dayt-1].setAll(dayActive,mistActive,sprayActive,startTime_H,startTime_M,
                            mistDuration,sprayDuration);
}

// Select the valve we would like to trun on and the pump at the same time.
void sparyState(int valve, int onOrOff){
  if(onOrOff == 1){
    digitalWrite(valve, HIGH);
    digitalWrite(PUMP, HIGH);
  }else if(onOrOff == 0){
    digitalWrite(PUMP, LOW);
    digitalWrite(valve, LOW);
  }
}

// Initialise Serial interafce
void initSerial(){
  Serial.begin(115200);
  
  Serial.print("\n\n");
}

// Initialise and connect to Wi-Fi
void initWifi(){
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WiFi."));
  int timeOut = 0;
  while (WiFi.status() != WL_CONNECTED){
    if(timeOut == 15){
      Serial.println(F("An Error has occurred while trying to connect to your wireless network.\nPlease check your SSID and PSK or ensure AP is in range!"));
      return;
    }else{
      delay(1000);
      Serial.print(F("."));
    }
    timeOut++;
  }

  // Check if we have connected to an AP and print ESP Local IP Address
  if(WiFi.status() == WL_CONNECTED){
    Serial.print(F("\nConnected!\nIP Address :: "));
    Serial.println(WiFi.localIP());
    digitalWrite(WIFI_OK, HIGH);
  }
}

void initGPIO(){
  // Setting GPIO direction - OUTPUT
  pinMode(PUMP, OUTPUT);
  pinMode(MIST, OUTPUT);
  pinMode(SPRAY, OUTPUT);
  pinMode(WIFI_OK, OUTPUT);
  pinMode(ESPLED, OUTPUT);

  // Init pin state
  digitalWrite(PUMP, LOW);
  digitalWrite(MIST, LOW);
  digitalWrite(SPRAY, LOW);
  digitalWrite(WIFI_OK, LOW);
  digitalWrite(ESPLED, LOW);

  // Setting GPIO direction - INPUT
  pinMode(WATER_LEVEL, INPUT);
}

// Initiate the i2c bus with GPIO04=SDA and GPIO05=SCL on the ESP8266.
void initI2C(){
  Wire.begin(RTC_SDA, RTC_SCL);
}

// Setup sensor pins and set pull timings and test if MCU can communicate with the sensor
void initDHT(){
  Serial.println(F("Connecting to DHT Sensors."));
  for(int i = 0; i < 5 ; ++i){
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\n");
  sensorOne.begin();
  if(isnan(sensorOne.readHumidity())||isnan(sensorOne.readTemperature())){
    Serial.println(F("Failed to read from DHT sensor. Try checking the connection!"));
    interval = 500;
  }
}

// Initialize SPIFFS
void inisSPIFFS(){
  Serial.println(F("Mounting SPIFFS."));
  if(!SPIFFS.begin()){
    Serial.println(F("An Error has occurred while mounting SPIFFS"));
    return;
  }
}

/********************* SET UP ************************/
void setup(){
  initSerial();
  initGPIO();
  initI2C();
  initWifi();
  initDHT();
  inisSPIFFS();

  for (int i = 0; i < (sizeof(allDay)/sizeof(allDay[0])); ++i){
    allDay[i].setAll(false,false,false,12,00+i,5,5);
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  Serial.println(F("Loading :: CSS file."));
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // For Spray control
  // Route to set GPIO to HIGH
  // Serial.println(F("Route to set GPIO's for SPRAY to HIGH."));
  server.on("/s_on", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(SPRAY, 1);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  // Serial.println(F("Route to set GPIO's for SPRAY to LOW."));
  server.on("/s_off", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(SPRAY, 0);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // For misting control
  // Route to set GPIO to HIGH
  // Serial.println(F("Route to set GPIO's for MIST to HIGH."));
  server.on("/m_on", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(MIST, 1);   
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  // Serial.println(F("Route to set GPIO's for MIST to LOW."));
  server.on("/m_off", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(MIST, 0);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getTemperature().c_str());
  });
  
  server.on("/humid", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getHumidity().c_str());
  });

   server.on("/formString", HTTP_POST,[](AsyncWebServerRequest * request){},NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
      std::string formData;
      for (size_t i = 0; i < len; i++){
        // Serial.write(data[i]);
        formData += data[i];
      }
      // Serial.println();
      request->send(200);
      storeFormValues(formData);
   });
  
  // Start server
  Serial.println(F("Starting web server.."));
  server.begin();
}

/********************** LOOP *************************/
void loop(){  
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval){
    // save the last time we printed to serial
    previousMillis = currentMillis;

    if(isnan(sensorOne.readHumidity())||isnan(sensorOne.readTemperature())){
      interval = 500;
    }else{
      interval = 1000;
    }

    // Serial.printf("DHT22 :: Humidity: %.2f%% Temperature: %.2f°C\n", h_01, t_01);
    
    // Toggle the ESP LED on and off buy reading its current state and inverting that value.
    digitalWrite(ESPLED, !digitalRead(ESPLED));
    
    // Limiting the number of web socket clients
    // server.cleanupClients();      
  }
}
