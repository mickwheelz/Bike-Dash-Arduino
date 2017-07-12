//GPS
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
//RPM Measurement
#include <FreqMeasure.h>
//Display
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
//DHT22 External Temp
#include <DHT.h>
#include <Adafruit_Sensor.h>
//JSON
#include <ArduinoJson.h>
//Smoothing
#include <SmoothADC.h>


#define OLED_RESET 4
#define DHTPIN  5
#define DHTTYPE DHT22 

SmoothADC ADC_3;

static const int  pinTemp = A3, pinSSRX = 2, pinSSTX = 3, pinFan = 4;

//BMP Images
static const unsigned char PROGMEM engineTempIcon [] = { 
0x0, 0x0, 0x1, 0x80, 0x1, 0x80, 0x1, 0xe0, 0x1, 0x80, 0x1, 0xe0, 0x1, 0x80, 0x1, 
0xe0, 0x1, 0x80, 0x3, 0xc0, 0xcf, 0xf3, 0x7b, 0xde, 0x1, 0x80, 0xce, 0x73, 0x31, 0x8c, 
0x0, 0x0 };
static const unsigned char PROGMEM fanStateIcon [] = { 
0x18, 0x0, 0x3c, 0x3c, 0x7e, 0x7e, 0x7e, 0xff, 0x7e, 0xff, 0x7f, 0xff, 0x3f, 0xfe, 0x1e, 
0x60, 0x6, 0x78, 0x7f, 0xfc, 0xff, 0xfe, 0xff, 0x7e, 0xff, 0x7e, 0x7e, 0x7e, 0x3c, 0x7c, 
0x0, 0x38 };
static const unsigned char PROGMEM externalTempIcon [] = { 
0x0, 0x0, 0x1, 0x0, 0x2, 0xb0, 0x2, 0x80, 0x2, 0x80, 0x2, 0xb0, 0x2, 0x80, 0x2, 
0x80, 0x3, 0xb0, 0x3, 0x80, 0x3, 0x80, 0x7, 0xc0, 0x7, 0xc0, 0x7, 0xc0, 0x3, 0x80, 
0x0, 0x0 };
static const unsigned char PROGMEM clockIcon [] = { 
0x3c, 0x42, 0x89, 0x89, 0x91, 0x81, 0x42, 0x3c };

//Display Object
Adafruit_SSD1306 display(OLED_RESET);

//GPS Objects
TinyGPSPlus gps;
SoftwareSerial ss(pinSSRX, pinSSTX);

//DHT Object
DHT dht(DHTPIN, DHTTYPE);

// This custom version of delay() ensures that the gps object is always being fed
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

/*
 * SENSOR READ METHODS
 */
//RPM of bike, TBD
int getRPM() { //TODO
  //TODO
  return 0;
}
//Temperature of bike, measured using bike temp sensor and voltage divider to ADC
int getTemp() {
  ADC_3.serviceADCPin();
  //int ADC3Value = 1023 - ADC_3.getADCVal();
  int ADC3Value = 1023 - analogRead(pinTemp);

  //int smoothTemp = asTemp.smooth(1023 - analogRead(pinTemp)) / 7.4;
  return (int)ADC3Value / 7.4;//analogRead(pinTemp);//String(smoothTemp);
}
//State of cooling fan, true for on, false for off
boolean getFanState() {
  return digitalRead(pinFan);
}
//gets speed from GPS, if false returns khm, if true returns mph
int getSpeed(boolean isMPH){
  if(isMPH) {
    return gps.speed.mph();
  }
  else {
    return gps.speed.kmph();
  }
}
//gets external temperature from DHT22
int getExternalTemp() {
  if(isnan(dht.readTemperature())){
    return 0;
  }
  else {
      return (int)dht.readTemperature();
  }
}
//build time string from GPS
String getTime() { //TODO
  return "";
}

/*
 * DISPLAY METHODS
 */
void displayTemp() {
  int temp = getTemp();
  display.drawBitmap(0, 0,  engineTempIcon, 16, 16, 1);
  display.setCursor(20,0);
  display.setTextColor(1,0);
  display.setTextSize(2);
  display.print(temp);
}
void displayFan() {
  if(getFanState()) {
    display.drawBitmap(0, 16,  fanStateIcon, 16, 16, 1);
  }
  else {
    display.setTextColor(0);
    display.setCursor(112,0);
    display.setTextSize(3);
    display.print("    ");
  }
}
void displayTime() {
  String timeS = "00:00:00";
  display.drawBitmap(68, 24,  clockIcon, 8, 8, 1);
  display.setTextColor(1,0);
  display.setTextSize(1);
  display.setCursor(78, 24);
  display.print(timeS);
}
void displayExternalTemp() {
  int externalTemp = getExternalTemp();
  display.drawBitmap(84,0,  externalTempIcon, 16, 16, 1);
  display.setCursor(104,0);
  display.setTextColor(1,0);
  display.setTextSize(2);
  display.print(externalTemp);
}
void displaySpeed(int speedS) { //TODO
  //display.drawBitmap(64,0,  externalTempIcon, 16, 16, 1);
  //display.setCursor(84,0);
  //display.setTextColor(white, black);
  //display.setTextSize(3);
  display.print(speedS);
}
void displayRPM(int rpm) { //TODO
  //display.drawBitmap(64,0,  externalTempIcon, 16, 16, 1);
  //display.setCursor(84,0);
  //display.setTextColor(white, black);
  //display.setTextSize(3);
  display.print(rpm);
}
//final method to print all on display
void outputDisplay() {
  displayTemp();
  displayFan();
  displayTime();
  displayExternalTemp();
  //displaySpeed
  //displayRPM
  display.display();
}
/*
 * JSON String Methods
 */
//build JSON Object
JsonObject& buildJSON() {
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["tmp"] = getTemp();
  root["fan"] = getFanState();
  root["rpm"] = getRPM();
  root["spd"] = getSpeed(true);
  root["ext"] = getExternalTemp();
  root["time"] = "00:00:00";
  return root;
}
//send to serial port
void outputJSON() {
 JsonObject& root = buildJSON();
  root.printTo(Serial);
  Serial.println();
}

/*
 * STANDARD ARDUINO METHODS
 */
void setup() {

  ADC_3.init(A3, 50); // Init ADC0 attached to A0 with a 50ms acquisition period
  if (ADC_3.isDisabled()) { ADC_3.enable(); }

  //init Pins
  pinMode(pinTemp, INPUT);
  pinMode(pinFan, INPUT);
  
  //init RPM
  FreqMeasure.begin();
    
  //Init Serial ports
  Serial.begin(115200);
  ss.begin(9600);
  
  //init Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  display.clearDisplay();
  
  //init DHT
  dht.begin();
}

//Arduino Loop
void loop() {
  //read GPS data
  gps.encode(ss.read());
  //output JSON
  outputJSON();
  //output to I2C Display
  outputDisplay();
  //wait 100ms
  smartDelay(100);
}

