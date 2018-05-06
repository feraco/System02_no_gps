#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>

#include <Adafruit_BME280.h>

// Sensors
Adafruit_BME280 bme; // I2C for barometric sensor

// SD card
const int SD_CS = 10;
File myFile;
char filename[11]; //make sure to define correct # of characters

// General Variables
int lock_minute = -1;
unsigned int adc_value = 0;
float UV_value = 0;

void setup() {
  Serial.begin(115200);
  pinMode(SD_CS, OUTPUT); //Adafruit SD shields and modules: pin 10

  do {
    Serial.println("Requesting time from Pi");
    sync_now();
    delay(2000);

  }  while (timeStatus() == timeNotSet);
  
  Serial.println("Time was set correctly");
  digitalClockDisplay();

  /* check on all components */
  if (!SD.begin(SD_CS)) {
    Serial.println(F("initialization failed!"));
    while(1);
  }

  if (!bme.begin()) {
    Serial.println(F("BME not found, check wiring!"));
    while (1);
  }
  
  Serial.println(F("Good to go"));  
}

void loop() {

  read_uv_index();

  if (timeStatus() != timeNotSet) {
    int index = (hour() * 60) + minute();
    if ((index == 0) && (minute() != lock_minute)) { //check that it's midnight
      lock_minute = minute();
      myFile.close();
      sprintf(filename, "%02d%02d%02d.txt", day(), month(), year());
      myFile = SD.open(filename, FILE_WRITE);
      Log2Card();
    }

    if ((index % 5 == 0) && (minute() != lock_minute)) { //check that minute is 5
      lock_minute = minute();
      sprintf(filename, "%02d%02d%02d.txt", day(), month(), year());
      myFile = SD.open(filename, FILE_WRITE);
      Log2Card();
    }
  }
}

// write to file
void Log2Card() {
  String data = "{\"log\":\"";
  // Add date
  data += String(day());
  data += "/";
  data += String(month());
  data += "/";
  data += String(year());
  data += ",";

  // Add time
  data += String(hour());
  data += ":";
  data += String(minute());
  data += ":";
  data += String(second());
  data += ",";

  // Add UV index
  data += UV_value;
  data += ",";

  // Add temperature
  data += String(bme.readTemperature());
  data += ",";

  // Add pressure
  data += String(bme.readPressure() / 100.0F);
  data += ",";

  // Add humidity
  data += String(bme.readHumidity());

  data += "\"}";

  Serial.println(data); // Send data to Raspberry Pi
  myFile.println(data); // Log to file

  myFile.flush();
  myFile.close();
}

unsigned int adcAverage() {
  unsigned char samples = 64;
  unsigned long avg = 0;
  while (samples > 0) {
    avg = (avg + analogRead(A0));
    delayMicroseconds(4);
    samples--;
  }
  avg >>= 6;
  return avg;
}

void read_uv_index() {
  adc_value = adcAverage();
  UV_value = ((adc_value * 0.0341) - 1.3894);

  if (UV_value > 11) {
    UV_value = 11;
  } else if (UV_value <= 0)   {
    UV_value = 0;
  }
}

String getDateTime() {
  unsigned long timeout = millis();
  String ret = "##/##/####,##:##:##.###";
  Serial.println("#whatTime");
  while (!Serial.available()) {
    if (millis() - timeout > 2000) {
      break;
    }
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'T') {
      ret = Serial.readStringUntil('\n');
      ret.trim();
      break;
    }
  }
  return ret;
}

unsigned long getEpoch() {
  unsigned long timeout = millis();
  unsigned long ret = 0;
  Serial.println("#whatEpoch");
  while (!Serial.available()) {
    if (millis() - timeout > 2000) {
      break;
    }
  }

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'E') {
      ret = Serial.parseInt();
      break;
    }
  }
  return ret;
}

void sync_now() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  pctime = getEpoch();
  Serial.println(pctime);
  if ( pctime >= DEFAULT_TIME) {
    setTime(pctime);
  }
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
}


void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


