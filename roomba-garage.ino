#include "SinricPro.h"
#include "SinricProGarageDoor.h"
#include <WiFi.h>
#include <TimeLib.h>

#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define SINRIC_APP_KEY "YOUR_SINRIC_AP_KEY"
#define SINRIC_APP_SECRET "YOUR_SINRIC_APP_SECRET"
#define SINRIC_GARAGEDOOR_ID "YOUR_SINRIC_GARAGEDOOR_ID"

bool setupDone = false;
bool garageState = false;
int garageActionTime;

// The actuator stroke is 100mm-8mm/s
// that means it takes 12.5 seconds to extend/retract the stroke
// 12.5 seconds is equal to 12500 mills
// set the duration in 12000 to give it some margin
int ACTUATOR_OPERATION_DURATION = 12000;

int GARAGE_TIMEOUT = 42; // seconds. 12 seconds from ACTUATOR_OPERATION_DURATION + arbitrary 30 seconds

int IR_INPUT = 5;
int INTERNAL_LED = 2;
int RELAY_1 = 18;
int RELAY_2 = 19;

void blinkLed() {
  digitalWrite(INTERNAL_LED, HIGH);
  delay(150);
  digitalWrite(INTERNAL_LED, LOW);
  delay(150);
  digitalWrite(INTERNAL_LED, HIGH);
  delay(150);
  digitalWrite(INTERNAL_LED, LOW);
  delay(150);
  digitalWrite(INTERNAL_LED, HIGH);
  delay(150);
  digitalWrite(INTERNAL_LED, LOW);
};

void stopActuator() {
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
}

void openGarage(int source) {
  if (garageState == false) {
    Serial.print("Opening from ");
    if (source == 1) {
      Serial.println("Alexa");
      garageActionTime = now();
    } else {
      Serial.println("IR Sensor");
    }

    SinricProGarageDoor &roombaGarage = SinricPro[SINRIC_GARAGEDOOR_ID];
    roombaGarage.sendDoorStateEvent(false);

    garageState = true;
    digitalWrite(INTERNAL_LED, HIGH);
    digitalWrite(RELAY_1, HIGH);
    digitalWrite(RELAY_2, LOW);
    
    delay(ACTUATOR_OPERATION_DURATION);
    stopActuator();
    Serial.println("Opening done!");
  }
};

void closeGarage(int source) {
  if (garageState == true) {
    Serial.print("Closing from ");
    if (source == 1) {
      Serial.println("Alexa");
      garageActionTime = now();
    } else {
      Serial.println("IR Sensor");
    }

    SinricProGarageDoor &roombaGarage = SinricPro[SINRIC_GARAGEDOOR_ID];
    roombaGarage.sendDoorStateEvent(true);

    garageState = false;
    digitalWrite(INTERNAL_LED, LOW);

    digitalWrite(RELAY_1, LOW);
    digitalWrite(RELAY_2, HIGH);
    
    delay(ACTUATOR_OPERATION_DURATION);
    stopActuator();
    Serial.println("Closing done!");
  }
};

bool onDoorState(const String& deviceId, bool &doorState) {
  if (doorState) {
    closeGarage(1);
  } else {
    openGarage(1);
  }
  return true;
}

void setupWifi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("");
  Serial.println("WiFi connected!");
}

void setupSinricPro() {
  SinricProGarageDoor &roombaGarage = SinricPro[SINRIC_GARAGEDOOR_ID];
  roombaGarage.onDoorState(onDoorState);
  Serial.println("Connecting to SinricPro...");
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(SINRIC_APP_KEY, SINRIC_APP_SECRET);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=============================");
  Serial.println("======= ROOMBA GARAGE =======");
  Serial.println("=============================");
  Serial.println("Init setup...");

  setupWifi();
  setupSinricPro();

  pinMode(IR_INPUT, INPUT);
  pinMode(INTERNAL_LED, OUTPUT);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);

  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
}

void loop() {
  SinricPro.handle();

  int timeElapsed = now() - garageActionTime;
  
  if ((timeElapsed > GARAGE_TIMEOUT - 2) && !setupDone) {
    Serial.println("Setup done!");
    blinkLed();
    setupDone = true;
  }

  if (timeElapsed > GARAGE_TIMEOUT) {
    
    int sensorStatus = digitalRead(IR_INPUT);
    if (sensorStatus == 1) {
      openGarage(2);
    } else {
      closeGarage(2);
    }
  }
}
