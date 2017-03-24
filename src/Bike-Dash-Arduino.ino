#include <Arduino.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <FreqMeasure.h>

//GPS and HW Serial Speed
static const uint32_t GPSBaud = 9600, HWSerialBaud = 115200;
//Pins for inputs/outputs/sofware serial
static const int pinPowerState = 6, pinRPI = 4, pinMODE = 5, pinTPS = A2, pinTEMP = A3, RXPin = 3, TXPin =2;
//default mode is 1
int currentMode = 1;

//GPS Objects
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);


// This custom version of delay() ensures that the gps object is always being fed
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

//RPM of bike, using freqmeasure and pin D8
String getRPM() {

  int rpm = 0;

  double sum = 0;
  int count = 0;

  if (FreqMeasure.available()) {
     // average several reading together
     sum = sum + FreqMeasure.read();
     count = count + 1;
     if (count > 30) {
       //float frequency FreqMeasure.countToFrequency(sum / count);
       rpm = (60) * FreqMeasure.countToFrequency(FreqMeasure.read());
       sum = 0;
       count = 0;
     }
   }

  return String(rpm);
}
//Temperature of bike, measured using bike temp sensor and voltage divider to ADC
String getTemp() {
  int tempState = 1023 - analogRead(pinTEMP);
  //at 100deg c, honda cb600 sensor returns 740... factor is 7.4
  tempState = tempState / 7.4;
  //return tempState;
  return String(tempState);
}
//Throtle position of bike, measured using bike TPS and voltage divider to ADC
String getTPS() {
  int tpsState = 1023 - analogRead(pinTPS);
  //at 100deg c, honda cb600 sensor returns 740... factor is 7.4
  //tempState = tempState / 7.4;
  //return tempState;
  return String(tpsState);
}
//returns true while pin is held high by bike being powered on, returns false otherswise.
//pulses pin D4 when state changes from false to true
boolean isBikeOn() {
  //pulses pin to wake up RPI when bike first turns on
  boolean pinPulse = false;
  if( digitalRead(pinPowerState) == LOW ) {
    digitalWrite(pinRPI, HIGH);
    delay(100);
    digitalWrite(pinRPI, LOW);
    pinPulse = true;
  }
  else {
    pinPulse = false;
  }
  return (digitalRead(pinPowerState) == LOW);
}
//build the JSON object to be sent over serial
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
  bikeData["TPS"] = getTPS();
  bikeData["Power"] = isBikeOn();

  return root;
}

void setup() {
  Serial.begin(HWSerialBaud);
  ss.begin(GPSBaud);

  FreqMeasure.begin();

  pinMode(pinRPI, OUTPUT);
  pinMode(pinMODE, INPUT);
  pinMode(pinTPS, INPUT);
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