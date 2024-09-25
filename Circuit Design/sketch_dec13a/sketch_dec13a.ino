/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com  
  https://www.electronicshub.org/wp-content/uploads/2021/02/ESP32-Pinout-1.jpg
  https://randomnerdtutorials.com/esp32-ds18b20-temperature-arduino-ide/
  https://randomnerdtutorials.com/esp32-esp8266-firebase-authentication/
*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "ragab"
#define WIFI_PASSWORD "mra12345"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAJzqLQByxFqbwvAH8YEhZ9gngh_3eA_Lg"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "mohammedr25102005@gmail.com"
#define USER_PASSWORD "123456789"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://dvt-predictor-default-rtdb.europe-west1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

unsigned long sendDataPrevMillis = 0;
int count = 0;

#define REPORTING_PERIOD_MS     20000

// Create a PulseOximeter object
PulseOximeter pox;

// Time at which the last beat occurred
uint32_t tsLastReport = 0;

// Callback routine is executed when a pulse is detected
void onBeatDetected()
{
  Serial.print("Beat!");
}

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

void heartBeatSensorSetup()
{
  if (!pox.begin()) {
    Serial.println("FAILED");
    for(;;);
  } else {
      Serial.println("SUCCESS");
  }

  pox.setIRLedCurrent(MAX30100_LED_CURR_46_8MA);

  // Register a callback routine
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void setup() {
  // Start the Serial Monitor
  //Serial.begin(9600);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);

  // Start the DS18B20 sensor
  sensors.begin();

  heartBeatSensorSetup();
}

void loop() {
  if (Firebase.isTokenExpired()){
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }
  // Grab the updated heart rate and SpO2 levels
  Serial.print("measuring...");
  while (millis() - tsLastReport < REPORTING_PERIOD_MS)
  {
    pox.update();
    if ((millis() - tsLastReport) % 1000 == 0)
      Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Heart rate:");
  Serial.print(pox.getHeartRate());
  Serial.print("bpm / SpO2:");
  Serial.print(pox.getSpO2());
  Serial.println("%");

  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");

  if (Firebase.ready()){
    
    if (Firebase.RTDB.setFloat(&fbdo, "row/Heart Rate", pox.getHeartRate() + 0.00001) 
    && Firebase.RTDB.setFloat(&fbdo, "row/Oxygen Saturation", pox.getSpO2() + 0.00001) 
    && Firebase.RTDB.setFloat(&fbdo, "row/Temperature", sensors.getTempCByIndex(0)) + 0.00001) {
      Serial.println("PASSED");
    }
    else {
      Serial.println("FAILED");
    }
  }
  tsLastReport = millis();
  heartBeatSensorSetup();
}
