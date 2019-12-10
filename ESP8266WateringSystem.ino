/*
  ESP8266 watering system originally based on Rui Santos
  https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/
*/
/*-----------------------------------------------------------------------------------------------
 * LIBRARIES
 * ----------------------------------------------------------------------------------------------*/
// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Wire.h>
#include <DHT.h>
#include <uRTCLib.h>
#include "Day.h"

/*-----------------------------------------------------------------------------------------------
 * DEFINES
 * ----------------------------------------------------------------------------------------------*/
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

/*-----------------------------------------------------------------------------------------------
 * OBJECTS
 * ----------------------------------------------------------------------------------------------*/
DHT sensorOne(DHT_DEV, DHTTYPE);              // Instantiate the DHT sensor object
uRTCLib rtc(0x68);                            // Instantiate the uRTCLib rtc object with device address of 0x68
AsyncWebServer server(80);                    // Create AsyncWebServer object on port 80
Day *allDay = new Day[7];                     // Array of days defining the week

/*-----------------------------------------------------------------------------------------------
 * GLOBALS
 * ----------------------------------------------------------------------------------------------*/
// Loop time keeping instead of using delays
unsigned long currentMillis = 0;              // This will store the current time in milliseconds
unsigned long previousMillisLED = 0;          // Store last time LED was updated
unsigned long previousMillisTimer = 0;        // Store last time Timer was updated
long intervalLED = 1000;                      // Intervals at which the LED on GPIO2 flash in (milliseconds)
const long intervalTimer = 1000;              // Timer interval to check when it's time to water

// SSID and PSK - Replace with your network credentials -- Yeah yeah, I know - tisk tisk!
String ssid = "SSID";
String password = "PSK";

String sprayState, mistState;                 // Stores spraying state
uint8_t wateringFlag = 0;                     // Flag to store the current timed watering state
uint8_t manualFlag = 0;                       // Flag to store the current manual watering state
uint8_t dow = 0;                              // Day Of the week stored in RTC chip - 1=Sun, 2=Mon...7=Sat
uint8_t hour = 0, minute = 0;                 // Variables to store time from RTC chip
int sparyingTime = 0, mistingTime = 0;        // Variables to store time to apply water from user input

/*-----------------------------------------------------------------------------------------------
 * METHODS
 * ----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------
 * Function: initSerial
 * Description: Initialise Serial interafce and prints a couple new lines to move the cursor
 * away from the garbage after a reboot.
 * Arguments: none
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void initSerial(){
  Serial.begin(115200);
  Serial.print("\n\n");
}

/*-----------------------------------------------------------------------------------------------
 * Function: initGPIO
 * Description: Configures the ESP's GPIO directions and initial states
 * Arguments: none
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
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

/*-----------------------------------------------------------------------------------------------
 * Function: initI2C
 * Description: Initiate the i2c bus with GPIO04=SDA and GPIO05=SCL on the ESP8266 12E.
 * Arguments: none
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void initI2C(){
  Wire.begin(RTC_SDA, RTC_SCL);
}

/*-----------------------------------------------------------------------------------------------
 * Function: initWifi
 * Description: Initialise and connect to Wi-Fi. This will also print to Serial the IP address.
 * Arguments: none
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void initWifi(){
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting to WiFi."));
  int timeOut = 0;
  while (WiFi.status() != WL_CONNECTED){
    if(timeOut == 15){
      Serial.println(F("An Error has occurred while trying to connect to your wireless network.\n"
        "Please check your SSID and PSK or ensure AP is in range!"));
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

/*-----------------------------------------------------------------------------------------------
 * Function: initDHT
 * Description: Setup sensor pins and set pull timings then test if MCU can communicate with
 * the sensor. If theres no sensor, the LED on the ESP will flash every 500ms.
 * Arguments: none
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
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
    intervalLED = 500;
  }
}

/*-----------------------------------------------------------------------------------------------
 * Function: initSPIFFS
 * Description: Initialize SPIFFS - Serial Peripheral Interface Flash File System on the ESP
 * Arguments: none
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void initSPIFFS(){
  Serial.println(F("Mounting SPIFFS."));
  if(!SPIFFS.begin()){
    Serial.println(F("An Error has occurred while mounting SPIFFS"));
    return;
  }
}

/*-----------------------------------------------------------------------------------------------
 * Function: updateDayTime
 * Description: When invoked, this will update the GLOBALS with new RTC data
 * Arguments: none
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void updateDayTime(){
    rtc.refresh();
    dow = rtc.dayOfWeek();
    hour = rtc.hour();
    minute = rtc.minute();
}

/*-----------------------------------------------------------------------------------------------
 * Function: sparyState
 * Description: This is to cotrol what valve we would like to have on as well as the pump. If any
 * valve is on, that valve will be turned off as onle one valve can be on at one time.
 * Arguments: int, int
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void sparyState(int valve, int onOrOff){
//  if(waterLevelOK() != 0){
    if(onOrOff == 1){
//      if(checkValvesState() > 0){
//        digitalWrite(checkValvesState(), LOW);
//      }
      digitalWrite(valve, HIGH);
      digitalWrite(PUMP, HIGH);
      wateringFlag = 1;
    }else if(onOrOff == 0){
      digitalWrite(PUMP, LOW);
      digitalWrite(valve, LOW);
//      if(checkValvesState() > 0){
//        digitalWrite(checkValvesState(), LOW);
//      }
      wateringFlag = 0;
    }
//  }
}

/*-----------------------------------------------------------------------------------------------
 * Function: tokenize
 * Description: This method is to split up the string that is received from the webform. It uses
 * a combination of find_first_not_of()and find() functions to split up the string.
 * Found from here: https://www.techiedelight.com/split-string-cpp-using-delimiter/
 * Arguments: std::string const, const char, std::vector<std::string>
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void tokenize(std::string const &str, const char delim, std::vector<std::string> &out){
  size_t start;
  size_t end = 0;
  while ((start = str.find_first_not_of(delim, end)) != std::string::npos){
    end = str.find(delim, start);
    out.push_back(str.substr(start, end - start));
  }
}

/*-----------------------------------------------------------------------------------------------
 * Function: storeFormValues
 * Description: We take a string as a perameter and split it based on that it's a comma 
 * delimited string, and pump it into the array for a given day[i]-1.
 * Arguments: std::string &data
 * Returns: None
 * ----------------------------------------------------------------------------------------------*/
void storeFormValues(std::string &data){
  std::vector<std::string> out;
  tokenize(data, ',', out);
  
  uint8_t dayt = atoi(out[0].c_str());
  bool dayActive = (out[1].compare("true") < 0) ? false : true;
  bool mistActive = (out[2].compare("true") < 0) ? false : true;
  bool sprayActive = (out[3].compare("true") < 0) ? false : true;
  uint8_t startTime_H = atoi(out[4].c_str());
  uint8_t startTime_M = atoi(out[5].c_str());
  uint8_t mistDuration = atoi(out[6].c_str()); 
  uint8_t sprayDuration = atoi(out[7].c_str());
  
  allDay[dayt-1].setAll(dayActive,mistActive,sprayActive,startTime_H,startTime_M,
                            mistDuration,sprayDuration);
}

/*-----------------------------------------------------------------------------------------------
 * Function: processor
 * Description: Response template for the that takes a coming from a web Stream and processes
 * the data received. This can trigger a valve to turn or off and get String data to pass back
 * to the webpage.
 * Arguments: const String& address
 * Returns: String
 * ----------------------------------------------------------------------------------------------*/
String processor(const String& var){
//  Serial.println("WEB_VAR :: " + var);
  if(var == "S_STATE"){
    if(digitalRead(SPRAY)){
      sprayState = "ON";
      manualFlag = 1;
    }else{
      sprayState = "OFF";
      manualFlag = 0;
    }
//    Serial.println("WEB_STATE :: " + sprayState);
    return sprayState;
  }
  else if(var == "M_STATE"){
    if(digitalRead(MIST)){
      mistState = "ON";
      manualFlag = 1;
    }else{
      mistState = "OFF";
      manualFlag = 0;
    }
  //    Serial.println("WEB_STATE :: " + mistState);
    return mistState;
  }else if (var == "TEMP"){
    return getTemperature();
  }else if (var == "HUMID"){
    return getHumidity();
  } 
}

/*-----------------------------------------------------------------------------------------------
 * Function: checkValvesState
 * Description: We check to see if any valves are on by looking at the pin state on the ESP
 * Arguments: None
 * Returns: uint8_t
 * ----------------------------------------------------------------------------------------------*/
uint8_t checkValvesState(){
  uint8_t valve = 0;
  if(digitalRead(SPRAY) != 0){
    valve = SPRAY;
  }
  else if(digitalRead(MIST) != 0){
    valve = MIST;
  }
  return valve;
}

/*-----------------------------------------------------------------------------------------------
 * Function: getTemperature
 * Description: Gets the temperature reading from the DHT22 sensor.
 * Arguments: None
 * Returns: String
 * ----------------------------------------------------------------------------------------------*/
String getTemperature(){
  float temperature = sensorOne.readTemperature();
  return String(temperature);
}

/*-----------------------------------------------------------------------------------------------
 * Function: getHumidity
 * Description: Gets the humidity reading from the DHT22 sensor.
 * Arguments: None
 * Returns: String
 * ----------------------------------------------------------------------------------------------*/
String getHumidity(){
  float humidity = sensorOne.readHumidity();
  return String(humidity);
}

/*-----------------------------------------------------------------------------------------------
 * Function: getDayTime
 * Description: This is used to pass day-of-week and time to the webpage in the following
 * format 1,12:34 for the user to see.
 * Arguments: None
 * Returns: String
 * ----------------------------------------------------------------------------------------------*/
String getDayTime(){
  return String(dow)+","+String(hour)+":"+String(minute);
}

/*-----------------------------------------------------------------------------------------------
 * Function: waterLevelOK
 * Description: Checks to see if there is water in the bucket - full = 1 and empty = 0.
 * Arguments: None
 * Returns: Boolean
 * ----------------------------------------------------------------------------------------------*/
bool waterLevelOK(){
  Serial.println("Checking water level!");
  return (digitalRead(WATER_LEVEL) != 0) ? false : true;
}

/*-----------------------------------------------------------------------------------------------
 * SET UP
 * ----------------------------------------------------------------------------------------------*/
void setup(){
  initSerial();
  initGPIO();
  initI2C();
  initWifi();
  initDHT();
  initSPIFFS();

  // Initialise Day objects in the array visualise that its working
  for(int i = 0; i < 7; ++i){
    allDay[i].setAll(false,false,false,12,00+i,0,0);
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

  // Route to set GPIO to HIGH - SPRAY
  server.on("/s_on", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(SPRAY, 1);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW - SPRAY
  server.on("/s_off", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(SPRAY, 0);
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to set GPIO to HIGH - MIST
  server.on("/m_on", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(MIST, 1);   
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW - MIST
  server.on("/m_off", HTTP_GET, [](AsyncWebServerRequest *request){
    sparyState(MIST, 0);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  //Sending temp data as a string from PROGMEM
  server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getTemperature().c_str());
  });
  
  //Sending humidity data as a string from PROGMEM
  server.on("/humid", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getHumidity().c_str());
  });

  // Respond with content using a callback for when the user submits data to store in the Day object array
  server.on("/formString", HTTP_POST,[](AsyncWebServerRequest * request){},NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    std::string formData;
    for (size_t i = 0; i < len; i++){
      formData += data[i];
    }
    request->send(200, "text/plain", "Server: Got it!");
    storeFormValues(formData);
  });

  //Sending day and time data as a string from PROGMEM
  server.on("/dayTime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getDayTime().c_str());
  });

  // Respond with content using a callback for a given day stored in the Day object array
  server.on("/formStringBack", HTTP_POST,[](AsyncWebServerRequest * request){},NULL,[](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    request->send(200, "text/plain", allDay[data[0]-49].getData().c_str());
  });

  //Sending waterLevelOK status data as a string from PROGMEM
  server.on("/waterLevel", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(waterLevelOK()).c_str());
  });
  
  // Start server
  Serial.println(F("Starting web server.."));
  server.begin();
}

/*-----------------------------------------------------------------------------------------------
 * LOOP
 * ----------------------------------------------------------------------------------------------*/
void loop(){  
  currentMillis = millis();

  if(currentMillis - previousMillisLED >= intervalLED){
//    Serial.println("In the LED check");
    // Save the last time we printed to serial
    previousMillisLED = currentMillis;
    // If the sensor disconnects, change the speed of the heartBeat to indicate to the user
    if(isnan(sensorOne.readHumidity())||isnan(sensorOne.readTemperature())){
      intervalLED = 500;
    }else{
      intervalLED = 1000;
    }
    // Toggle the ESP LED on and off buy reading its current state and inverting that value.
    digitalWrite(ESPLED, !digitalRead(ESPLED));    
  }

  if(currentMillis - previousMillisTimer >= intervalTimer){
//    Serial.println("In the timer check!");
    previousMillisTimer = currentMillis;
    updateDayTime();
    // This block is to turn the valve and pump on
    if(allDay[dow-1].getDayActive() && waterLevelOK() != 0 && wateringFlag == 0){
      if(hour == allDay[dow-1].getStartTime_H() && minute == allDay[dow-1].getStartTime_M()){
        if(allDay[dow-1].getMistActive()){
          sparyState(MIST, 1);
          mistingTime = allDay[dow-1].getMistDuration()*60;
        }else if(allDay[dow-1].getSprayActive()){
          sparyState(SPRAY, 1);
          sparyingTime = allDay[dow-1].getSprayDuration()*60;
        }
      }
    }

    // This block is to trun the valve and pump off after a given amount of spray time
    if(allDay[dow-1].getMistActive() && wateringFlag != 0){
      if(mistingTime > 0){
        --mistingTime;
      }else{
        sparyState(MIST, 0);
      }
    }else if(allDay[dow-1].getSprayActive() && wateringFlag != 0){
      if(sparyingTime > 0){
        --sparyingTime;
      }else{
        sparyState(SPRAY, 0);
      }
    }

    // This block is to monitor the water level and turn the pump off if the water gets too low
    if(waterLevelOK() == 0){
      Serial.println("Warn! Turn water off!");
      sparyState(MIST, 0);
      sparyState(SPRAY, 0);
    }
  }
}
