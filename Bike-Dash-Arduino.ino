#include <TinyGPS++.h>
#include <SoftwareSerial.h>

static const int RXPin = 3, TXPin = 2; //TX and RX pins for GPS
static const uint32_t GPSBaud = 9600;

int pinPowerState = 6;
int pinRPI = 4;
int pinMODE = 5;

int pinRPM = A0;
int pinTEMP = A1;

int currentMode = 1;


// The TinyGPS++ object
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin); //SS for GPS

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);

  Serial.println("Format");
  Serial.println("RPM|SPEED|LAT|LNG|TEMP|MODE|TIME");

  pinMode(pinRPI, OUTPUT);
  pinMode(pinMODE, INPUT);
  pinMode(pinRPM, INPUT);
  pinMode(pinTEMP, INPUT);
}

void loop() {
  //           RPM         |    MPH                |    LATITUDE              |    LONGETUDE             |    TEMPERATUE    |    MODE         |    TIME
  Serial.print(getRPM() + "|" + gps.speed.mph() + "|" + gps.location.lat() + "|" + gps.location.lng() + "|" + getTemp() + "|" + setMode() + "|" + gps.time.value() ) ;
  Serial.println(); //NEW LINE
  
  smartDelay(500);

  if (millis() > 5000 && gps.charsProcessed() < 10) {
     //   Serial.println(F("No GPS data received: check wiring"));
  }
  
  doRaspberryPiPowerMgmt();

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
  return "00000";
}

String getTemp() {
  int tempState = analogRead(pinTEMP) / 10;
  //TODO: some math to get correct value
  //return tempState;
  return String(tempState);
}

String setMode() {
    if(digitalRead(pinMode) == HIGH) {
      if(currentMode < 4) {
          currentMode++;
          delay(1000); //wait a second before accepting another press
      }
      else {
        currentMode = 1;
        delay(1000); //wait a second before accepting another press
      }
    }
    return String(currentMode);
}

void doRaspberryPiPowerMgmt() {
  bool hasGoneHigh = false; //check if we have pulsed the pin
    if(!hasGoneHigh && digitalRead(pinPowerState) == HIGH) { //if we havent and thw powerState pin is high (meaning bike is on) pulse the pin to boot the rpi
      digitalWrite(pinRPI, HIGH);
      hasGoneHigh = true;
    }
  if(hasGoneHigh && digitalRead(pinPowerState) == LOW) { //if we have pulsed it, and we lose power to the powerState pin, pulse again to init shutdown of rpi
    delay(60000); //wait a minute after bike power off
    digitalWrite(pinRPI, HIGH);
  }
}


