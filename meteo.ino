#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>;
#include <Adafruit_BMP280.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <SoftwareSerial.h>;

// --- PIN define
#define DHTPIN 8
#define DHTTYPE DHT22
#define ONE_WIRE_BUS 4
#define MHZ_TX 11
#define MHZ_RX 10
#define LED_RED 9
#define LED_YELLOW 6
#define LED_GREEN 5

// --- CONST logic
// период синхронизации - сколько тактов loop должно пройти для синхронизации данных с датчиков
#define PERIOD_SYNC 5


LiquidCrystal_I2C lcd(0x3F, 20, 4);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp; // I2C
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
SoftwareSerial mhz(MHZ_TX, MHZ_RX);


float dh;  
float dt; 
float dtv; 
float bt; 
float bp; 
float ba; 
float st;
int ppm;
int currentLoop = 0;
int preheatSec = 10;
String CO2Status;


byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
byte cmdABCOff[9] = {0xFF,0x01,0x79,0x00,0x00,0x00,0x00,0x00,0x86}; 
byte cmdZeroCalibration[9] = {0xFF,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78}; 
unsigned char response[9];


void setup()
{
  Serial.begin(9600);
  lcd.begin();
  dht.begin();
  bmp.begin(0x76);
  sensors.begin(); 
  mhz.begin(9600);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  lcd.noCursor();
  lcd.backlight();
}


int readPPM()
{
  unsigned int lppm = 0;

  mhz.write(cmd, 9);
  memset(response, 0, 9);
  mhz.readBytes(response, 9);
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=response[i];
  crc = 255 - crc;
  crc++;

  if ( !(response[0] == 0xFF && response[1] == 0x86 && response[8] == crc) ) {
    Serial.println("CRC error: " + String(crc) + " / "+ String(response[8]));
  } else {
    unsigned int responseHigh = (unsigned int) response[2];
    unsigned int responseLow = (unsigned int) response[3];
    lppm = (256*responseHigh) + responseLow;
  }

  return lppm;
}


void showPreheat()
{
  lcd.print(
    stringFixLength("CO2  wait    " + String(preheatSec) + "s", 20)
  );
  preheatSec--;
}


void showLed(int level)
{
  switch (level) {
    case 1: // GOOD
      analogWrite(LED_RED, 0);
      analogWrite(LED_YELLOW, 0);
      analogWrite(LED_GREEN, 200);
      break;
    case 2: // NORM
      analogWrite(LED_RED, 0);
      analogWrite(LED_YELLOW, 200);
      analogWrite(LED_GREEN, 50);
      break;
    case 3: // HARD
      analogWrite(LED_RED, 100);
      analogWrite(LED_YELLOW, 200);
      analogWrite(LED_GREEN, 0);
      break;
    case 4: // HELL
      analogWrite(LED_RED, 200);
      analogWrite(LED_YELLOW, 0);
      analogWrite(LED_GREEN, 0);
      break;
    case 9: // ERROR CO2
      analogWrite(LED_RED, 200);
      analogWrite(LED_YELLOW, 200);
      analogWrite(LED_GREEN, 200);
      break;
    case 0:
    default: 
      analogWrite(LED_RED, 0);
      analogWrite(LED_YELLOW, 0);
      analogWrite(LED_GREEN, 0);
    break;
  }
}

String stringFixLength(String str, int length){
    for(int i = str.length(); i < length; i++)
        str += ' '; 
    return str;
}


void readSensors()
{
  dh = dht.readHumidity();
  dt = dht.readTemperature();
  dtv = dht.computeHeatIndex(dt, dh, false);
  bt = bmp.readTemperature();
  bp = bmp.readPressure();
  ba = bmp.readAltitude(1013.25);
  sensors.requestTemperatures();
  st = sensors.getTempCByIndex(0);
  ppm = readPPM();
}


void loop()
{ 
  if (currentLoop == 0) {
    readSensors();
  } else {
    delay(1000);
  }

  if (currentLoop++ >= PERIOD_SYNC) {
    currentLoop = 0;
  }

  if (preheatSec > 0) {
    CO2Status = "CO2  wait    " + String(preheatSec) + " s";
    preheatSec--;
  } else {
    CO2Status = "CO2  " + stringFixLength(String(ppm), 8); 
    if (ppm < 800) {
      CO2Status += "GOOD";
      showLed(1);
    } else if(ppm < 1200) {
      CO2Status += "NORM";
      showLed(2);
    } else if(ppm < 2000) {
      CO2Status += "BAD";
      showLed(3);
    } else {
      CO2Status += "HELL";
      showLed(4);
    }
  }

  lcd.setCursor(0, 0);
  lcd.print(
    stringFixLength(CO2Status, 20)
  );
  
  lcd.setCursor(0, 1);
  lcd.print(
    stringFixLength("Temp " + stringFixLength(String(dt), 8) + String(st), 20)
  );
  
  lcd.setCursor(0, 2);
  lcd.print(
    stringFixLength("Humd " + String(dh), 20)
  );
  
  lcd.setCursor(0, 3);
  lcd.print(
    stringFixLength("mmHg " + String(bp * 0.0075), 20)
  );
}
