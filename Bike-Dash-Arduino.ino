#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <FreqMeasure.h>
#include <AnalogSmooth.h>

//GPS and HW Serial Speed
static const uint32_t GPSBaud = 9600, HWSerialBaud = 115200;
//Pins for inputs/outputs/sofware serial
static const int pinPowerState = 6, pinRPI = 7, pinMODE = 5, pinTEMP = A3, RXPin = 4, TXPin = 3, pinFanState = 10, pinTPS = A2;
//default mode is 1
int currentMode = 1;

//GPS Objects
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

//Smoothing Lib
AnalogSmooth as = AnalogSmooth();

// This custom version of delay() ensures that the gps object is always being fed
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

//RPM of bike, TBD
String getRPM() {
  return String(0);
}

//Temperature of bike, measured using bike temp sensor and voltage divider to ADC
String getTemp() {
  //at 100deg c, honda cb600 sensor returns 740... factor is 7.4
  float tempState = 1023 - analogRead(pinTEMP);
  float smoothTemp = as.smooth(tempState) / 7.4;
  return String(smoothTemp);
}

//Throtle position of bike in percent
String getTPS() {
  float tpsState = analogRead(pinTPS);
  float smoothTPS = (as.smooth(tpsState) / 1023) * 100;
  return String(smoothTPS);
}

//TODO:Power Management
boolean isBikeOn() {
  return (digitalRead(pinPowerState) == LOW);
}

//build time string from GPS
String buildTime(int offset) {

  String hour;
  String minute;
  String second;
  
  gps.time.hour() < 10 ? hour = "0" + String(gps.time.hour() + offset) : hour = String(gps.time.hour() + offset);
  gps.time.minute() < 10 ? minute = "0" + String(gps.time.minute()) : minute = String(gps.time.minute());   
  gps.time.second() < 10 ? second = "0" + String(gps.time.second()) : second = String(gps.time.second());

  return hour + ":" + minute + ":" + second;

}

boolean isFanOn() {
    return (digitalRead(pinFanState) == HIGH);
}

//build the JSON object to be sent over serial
JsonObject& buildJSON() {
  
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["status"] = "OK";
  root["mode"] = "1";//setMode();
  root["time"] = buildTime(1);

  JsonObject& gpsData = root.createNestedObject("gpsData");

  gpsData["Status"] = "OFFLINE";

  gps.sentencesWithFix() == 0 ? gpsData["Status"] = "ONLINE - NO FIX" : gpsData["Status"] = "ONLINE";

  gpsData["LAT"] = String(gps.location.lat(),6); 
  gpsData["LNG"] = String(gps.location.lng(),6);
  gpsData["MPH"] = String(gps.speed.mph(),6); //String((random(1, 99) / 100.0) + random(1,100));//
  gpsData["KPH"] = String(gps.speed.kmph(),6);

  JsonObject& bikeData = root.createNestedObject("bikeData");
  bikeData["RPM"] = getRPM();
  bikeData["Temp"] = getTemp(); //String((random(1, 99) / 100.0) + random(1,100));
  bikeData["Power"] = isBikeOn();
  bikeData["Fan"] = isFanOn();
  bikeData["TPS"] = getTPS();

  return root;
}

//Arduino Setup
void setup() {
  Serial.begin(HWSerialBaud);
  ss.begin(GPSBaud);

  FreqMeasure.begin();

  pinMode(pinRPI, OUTPUT);
  pinMode(pinMODE, INPUT);
  pinMode(pinTEMP, INPUT);
  pinMode(pinPowerState, INPUT);
  pinMode(pinFanState, INPUT);
  pinMode(pinTPS, INPUT);

}

//Arduino Loop
void loop() {

  if(isBikeOn()) {
      gps.encode(ss.read());
      JsonObject& root = buildJSON();
      root.printTo(Serial);
      Serial.println();
      smartDelay(100);
  }

  else {
    //output once to shutdown RPI
    JsonObject& root = buildJSON();
    root.printTo(Serial);
    Serial.println();
    smartDelay(100);
    while(!isBikeOn()) {
      //TODO: Sleep mode
    }
  }
}

