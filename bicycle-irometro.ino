//Coded by Jairo Ivo
#include <MPU9250_asukiaaa.h>
#include <TinyGPS++.h>
#include <ESP32Time.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>


#define LED 2

// ESP32Time rtc;
ESP32Time rtc(-10800);  // offset in seconds GMT-3

#define GPS_BAUDRATE 9600  // the default baudrate of NEO-6M is 9600

TinyGPSPlus gps;  // the TinyGPS++ object

String dataMessage;  // save data to sdcard

File dataFile;


// Internal RTC Variables
int rtcdia, rtcmes, rtcano, rtchora, rtcminuto, rtcsegundo{ 0 };
long rtcmillis;
// GPS Variables
int gpsdia, gpsmes, gpsano, gpshora, gpsminuto, gpssegundo{ 0 };
float gpslat, gpslong{ 0 };
char latitudeStr[15];
char longitudeStr[15];
double gpsalt = 0;
float gpsvel = 0;
// Sensor reading variables
// MPU9052
MPU9250_asukiaaa mySensor;
float aX, aY, aZ, aSqrt, gX, gY, gZ, mDirection, mX, mY, mZ;

int gpsUpdate = 0;  // Informs if the gps was updated on the last data

void setup() {
  Serial.begin(9600);
  Serial2.begin(GPS_BAUDRATE);

  Serial.println(F("AirTemp View - Coded by Jairo Ivo"));

  pinMode(LED, OUTPUT);

  initSDCard();
  checkSDFile();  // Check the data.csv file on the memory card or create it if it does not exist

  Serial.println(F("Initiating synchronization of the internal RTC with the gps!"));
  delay(500);
  while (rtc.getYear() < 2001) {
    Serial.println(F("."));
    rtcSyncWithGps();
    //delay(50);
  }

#ifdef _ESP32_HAL_I2C_H_  // For ESP32
  Wire.begin();
  mySensor.setWire(&Wire);
#else
  Wire.begin();
  mySensor.setWire(&Wire);
#endif

  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();

  // You can set your own offset for mag values
  // mySensor.magXOffset = -50;
  // mySensor.magYOffset = -55;
  // mySensor.magZOffset = -10;
}

void loop() {
  //delay(500);
  if (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      digitalWrite(LED, LOW);
      if (gps.location.isValid()) {
        dtostrf(gps.location.lat(), 12, 8, latitudeStr);
        dtostrf(gps.location.lng(), 12, 8, longitudeStr);
        gpsUpdate = 1;
        if (gps.altitude.isValid()) {
          gpsalt = (gps.altitude.meters());
        } else {
          Serial.println(F("- alt: INVALID"));
          delay(150);
        }
      } else {
        Serial.println(F("- location: INVALID"));
        delay(150);
      }
      if (gps.speed.isValid()) {
        gpsvel = (gps.speed.kmph());
      } else {
        Serial.println(F("- speed: INVALID"));
        delay(150);
      }
      if (gps.date.isValid() && gps.time.isValid()) {
        gpsano = (gps.date.year());
        gpsmes = (gps.date.month());
        gpsdia = (gps.date.day());
        gpshora = (gps.time.hour());
        gpsminuto = (gps.time.minute());
        gpssegundo = (gps.time.second());
      } else {
        Serial.println(F("- gpsDateTime: INVALID"));
        delay(150);
      }
      //Serial.println();
    }
  }

  if (mySensor.accelUpdate() == 0) {
    aX = mySensor.accelX();
    aY = mySensor.accelY();
    aZ = mySensor.accelZ();
    aSqrt = mySensor.accelSqrt();
    Serial.print("accelX: " + String(aX));
    Serial.print("\taccelY: " + String(aY));
    Serial.print("\taccelZ: " + String(aZ));
    Serial.print("\taccelSqrt: " + String(aSqrt));
  }

  if (mySensor.gyroUpdate() == 0) {
    gX = mySensor.gyroX();
    gY = mySensor.gyroY();
    gZ = mySensor.gyroZ();
    Serial.print("\tgyroX: " + String(gX));
    Serial.print("\tgyroY: " + String(gY));
    Serial.print("\tgyroZ: " + String(gZ));
  }

  if (mySensor.magUpdate() == 0) {
    mX = mySensor.magX();
    mY = mySensor.magY();
    mZ = mySensor.magZ();
    mDirection = mySensor.magHorizDirection();
    Serial.print("\tmagX: " + String(mX));
    Serial.print("\tmaxY: " + String(mY));
    Serial.print("\tmagZ: " + String(mZ));
    Serial.print("\thorizontalDirection: " + String(mDirection));
  }

  if ((rtc.getMillis()) - rtcmillis >= 100) {
    digitalWrite(LED, HIGH);
    Serial.print(F("- RTC date&time: "));
    Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));  // (String) returns time with specified format
    
    rtcmes = rtc.getMonth();
    rtcdia = rtc.getDay();
    rtcano = rtc.getYear();
    rtchora = rtc.getHour(true);
    rtcminuto = rtc.getMinute();
    rtcsegundo = rtc.getSecond();

    Serial.print(F("- GPS date&time: "));
    Serial.print(gpsano);
    Serial.print(F("-"));
    Serial.print(gpsmes);
    Serial.print(F("-"));
    Serial.print(gpsdia);
    Serial.print(F(" "));
    Serial.print(gpshora);
    Serial.print(F(":"));
    Serial.print(gpsminuto);
    Serial.print(F(":"));
    Serial.println(gpssegundo);
    Serial.print(F("- latitude: "));
    Serial.println(latitudeStr);
    Serial.print(F("- longitude: "));
    Serial.println(longitudeStr);
    Serial.print(F("- altitude: "));
    Serial.println(gpsalt);
    Serial.print(F("- speed: "));
    Serial.print(gpsvel);
    Serial.println(F(" km/h"));

    saveData();
    gpsUpdate = 0;
    rtcmillis = rtc.getMillis();
    Serial.println();
  }
}

void rtcSyncWithGps() {
  if (Serial2.available() > 0) {
    //delay(150);
    if (gps.encode(Serial2.read())) {
      delay(150);
      Serial.print(F("- GPS date&time: "));
      if (gps.date.isValid() && gps.time.isValid()) {
        Serial.print(gps.date.year());
        Serial.print(F("-"));
        Serial.print(gps.date.month());
        Serial.print(F("-"));
        Serial.print(gps.date.day());
        Serial.print(F(" "));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        Serial.println(gps.time.second());
        rtc.setTime((gps.time.second()), (gps.time.minute()), (gps.time.hour()), (gps.date.day()), (gps.date.month()), (gps.date.year()));  // 17th Jan 2021 15:24:30
        //rtc.setTime(1609459200);  // 1st Jan 2021 00:00:00
        //rtc.offset = 7200; // change offset value
        Serial.print(F("- RTC date&time: "));
        Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));  // (String) returns time with specified format
        // formating options  http://www.cplusplus.com/reference/ctime/strftime/
      } else {
        Serial.println(F("No valid date and time data!"));
      }
      Serial.println();
    }
  }
  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No valid gps data: check connection"));
}