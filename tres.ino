#include <math.h>

//Start Wifi
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "Adafruit_MLX90614.h"
#define WIFI_SSID "Loto"
#define WIFI_PASSWORD "Sapikepala2"
//End Wifi

//Start Firebase
#include "addons/TokenHelper.h" //Provide the token generation process info.
#include "addons/RTDBHelper.h"  //Provide the RTDB payload printing info and other helper functions.
#define API_KEY "AIzaSyAblt42uBeeALZ-SV1Q5FjfyzPgJ3syRfo" // Insert Firebase project API Key
#define DATABASE_URL "https://project-74b65-default-rtdb.firebaseio.com" // Insert RTDB URLefine the RTDB URL */

FirebaseData FirebaseData;  //Firebase data object
FirebaseAuth auth;  //Firebase authentication object
FirebaseConfig config;  //Firebase configuration object
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

TaskHandle_t PostToFirebase;
bool signupOK = false;
// End Firebase

// Start Function Declaration
void SendReadingsToFirebase();
void InitializeWifi();
void SignUpToFirebase();
void InitializePOX();
// End Function Declaration

// Start Pulse Oximeter
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#define POX_REPORTING_PERIOD_MS  1000

PulseOximeter pox;  // Create a PulseOximeter object

TaskHandle_t GetReadings;
uint8_t _spo2 = 0;
uint8_t _heartRate = 0;
float _temp = 0.0;

uint32_t poxLastReport = 0;
uint32_t prevMillis = 0;
// End Pulse Oximeter

void setup() {
  
  Serial.begin(115200); //Begin serial communication

  InitializeWifi();

  SignUpToFirebase();

  // InitializePOX();

  xTaskCreatePinnedToCore(SensorReadings, "GetReadings", 1724, NULL, 0, &GetReadings, 0);
  
  xTaskCreatePinnedToCore(SendReadingsToFirebase, "PostToFirebase", 6268, NULL, 0, &PostToFirebase, 1);
}

void SensorReadings(void * parameter)
{
  mlx.begin();

  for(;;)
  {
    // Read from the sensor
    // pox.update();
      
    if (millis() - poxLastReport > POX_REPORTING_PERIOD_MS) {
      _heartRate = round(0);
      _spo2 = round(0);
      _temp = mlx.readObjectTempC();
    
      Serial.print("Heart rate:");  
      Serial.print(_heartRate);
      Serial.print("bpm / SpO2:");
      Serial.print(_spo2);
      Serial.println("%");
      Serial.print("Temp:");
      Serial.println(_temp);
    
      poxLastReport = millis();
    }
    // Memory Sizing
    //if (millis() - prevMillis > 6000)
    //{
    //  unsigned long remainingStack = uxTaskGetStackHighWaterMark(NULL);
    //  Serial.print("Free stack: ");
    //  Serial.print(remainingStack);
    //  prevMillis = millis();
    //}
    // End Memory Sizing
  }
}

void SendReadingsToFirebase(void * parameter)
{
  for(;;)
  {
    if (Firebase.ready() && signupOK){
      if (!Firebase.RTDB.getString(&FirebaseData, "userId/1/nama")) {
        Firebase.RTDB.setString(&FirebaseData, "userId/1/nama", "Default Name");
      }

      if (!Firebase.RTDB.getString(&FirebaseData, "userId/1/noKamar")) {
        Firebase.RTDB.setInt(&FirebaseData, "userId/1/noKamar", 2);
      }

      FirebaseJson json;
      json.set("beat", int(_heartRate));
      json.set("spo2", int(_spo2));
      json.set("temp", int(_temp));
      json.set("timestamp/.sv", "timestamp");

      Firebase.RTDB.pushJSON(&FirebaseData, "userId/1/nilai", &json);
    }
  }
}

void InitializeWifi()
{
  // Wifi Initialize ...
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
}

void SignUpToFirebase()
{
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void InitializePOX()
{
  Serial.print("Initializing pulse oximeter..");

  // Initialize sensor
  if (!pox.begin()) {
    Serial.println("FAILED");
    for(;;);
  } else {
    Serial.println("SUCCESS");
  }

  // Configure sensor to use 7.6mA for LED drive
  //pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
}

void loop()
{
  delay(1);  
}