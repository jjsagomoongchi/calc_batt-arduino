#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"
#define API_KEY "API_KEY"
#define DATABASE_URL "https://DATABASE_NAME.firebaseio.com"
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

#include <Wire.h>
#include <INA219_WE.h>
#define I2C_ADDRESS_A 0x40
#define maxV 3.3
INA219_WE ina219A = INA219_WE(I2C_ADDRESS_A);//센서의 I2C주소
float inputVolA;
float calVolA;
void setup() {
  pinMode(A0, INPUT);
  Serial.begin(115200);
  Wire.begin();

  if(!ina219A.init()){
    Serial.println("INA219 not connected!");
  }
  Serial.println("INA219 Current Sensor Example Sketch - Continuous");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);
}

void loop() {
  //충전인경우///////////////
  float shuntVoltage_mV_A = 0.0; //데이터 입력
  float loadVoltage_V_A = 0.0;
  float busVoltage_V_A = 0.0;
  float current_mA_A = 0.0;
  float power_mW_A = 0.0; 
  bool ina219_overflow_A = false;
  
  shuntVoltage_mV_A = ina219A.getShuntVoltage_mV();
  busVoltage_V_A = ina219A.getBusVoltage_V();
  current_mA_A = ina219A.getCurrent_mA();
  power_mW_A = ina219A.getBusPower();
  loadVoltage_V_A  = busVoltage_V_A + (shuntVoltage_mV_A/1000);
  ina219_overflow_A = ina219A.getOverflow();
  ///////////////////////////

  inputVolA = analogRead(A0); //데이터
  calVolA = map(inputVolA, 0, 1023, 0, maxV);

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    //충전인 경우//////////////
    Serial.printf("Set Current... %s\n", Firebase.setFloat(fbdo, "/product/calc_batt/env/Current", current_mA_A) ? "ok" : fbdo.errorReason().c_str()); //Firebase로 데이터
    Serial.printf("Set Voltage... %s\n", Firebase.setFloat(fbdo, "/product/calc_batt/env/Voltage", loadVoltage_V_A) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set Power... %s\n", Firebase.setFloat(fbdo, "/product/calc_batt/env/Power", power_mW_A) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set OverFlow... %s\n", Firebase.setBool(fbdo, "/product/calc_batt/env/OverFlow", ina219_overflow_A) ? "ok" : fbdo.errorReason().c_Str());
    //////////////////////////
    Serial.printf("Set BatteryVoltage… %s\n", Firebase.setFloat(fbdo, "/product/calc_batt/env/batteryV", calVolA) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set BatteryLevel... %s\n", Firebase.setFloat(fbdo, "/product/calc_batt/env/BatteryLv", calVolA/4.2*100) ? "ok" : fbdo.errorReason().c_str());
  }
}