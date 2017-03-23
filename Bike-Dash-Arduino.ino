#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

//TX and RX pins for GPS
static const int RXPin = 3, TXPin = 2; 
//GPS Speed
static const uint32_t GPSBaud = 9600;

int pinPowerState = 6;
int pinRPI = 4;
int pinMODE = 5;

int pinRPM = A0;
int pinTEMP = A1;

int currentMode = 1;

TinyGPSPlus gps;
///SS for GPS
SoftwareSerial ss(RXPin, TXPin);

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);

  pinMode(pinRPI, OUTPUT);
  pinMode(pinMODE, INPUT);
  pinMode(pinRPM, INPUT);
  pinMode(pinTEMP, INPUT);
  pinMode(pinPowerState, INPUT);
}

void loop() {
  
    if(isBikeOn()) {
        JsonObject& root = buildJSON();
        root.printTo(Serial);
        Serial.println();
        delay(500);
    }
    else {
      //output once to shutdown RPI
      JsonObject& root = buildJSON();
      root.printTo(Serial); 
      Serial.println();
      delay(500);
      while(!isBikeOn()) {
        //TODO: Sleep mode
      }
    }
}

// This custom version of delay() ensures that the gps object is always being fed
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

String getRPM() {
  int rpmState = analogRead(pinRPM);
  //TODO: some math to get correct value
  //return rpmState;
  return String(random(1000, 13000));
}

String getTemp() {
  int tempState = analogRead(pinTEMP) / 10;
  //TODO: some math to get correct value
  //return tempState;
  return String(tempState);
}

boolean isBikeOn() {
  //pulses pin to wake up RPI when bike first turns on
  boolean pinPulse = false;
  if(digitalRead(pinPowerState)) {
    digitalWrite(pinRPI, HIGH);
    delay(100);
    digitalWrite(pinRPI, LOW);
    pinPulse = true;
  }
  else {
    pinPulse = false;
  }
  return (digitalRead(pinPowerState) == HIGH);
}

JsonObject& buildJSON() {

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  root["status"] = "OK";
  root["mode"] = "1";//setMode();
  root["time"] = gps.time.value();

  JsonObject& gpsData = root.createNestedObject("gpsData");
  
  gpsData["Status"] = "OFFLINE";
  
  if(gps.sentencesWithFix() == 0) {
    gpsData["Status"] = "ONLINE - NO FIX";
  }
  else {
     gpsData["Status"] = "ONLINE";
  }
  
  gpsData["LAT"] = gps.location.lat(); 
  gpsData["LNG"] = gps.location.lng();
  gpsData["MPH"] = gps.speed.mph();
  gpsData["KPH"] = gps.speed.kmph();

  JsonObject& bikeData = root.createNestedObject("bikeData");
  bikeData["RPM"] = getRPM(); 
  bikeData["Temp"] = getTemp();
  bikeData["Power"] = isBikeOn();

  return root;
}
