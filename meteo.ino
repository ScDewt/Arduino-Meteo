
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>;
#include <Adafruit_BMP280.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <SoftwareSerial.h>;

#define DHTPIN 7
#define DHTTYPE DHT22
#define ONE_WIRE_BUS 8
#define MHZ_TX 11
#define MHZ_RX 10
#define LED_RED 9
#define LED_GREEN 6

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
int preheatSec = 180;

byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
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
  pinMode(LED_GREEN, OUTPUT);
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
  lcd.print("wait    ");
  lcd.print(preheatSec);
  lcd.print("s");
  if (preheatSec < 100 and preheatSec > 9) {
    lcd.print(" ");
  }
  if (preheatSec < 9) {
    lcd.print("  ");
  }
  preheatSec--;
}


void loop()
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

  lcd.noCursor();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("CO2  ");

  if (preheatSec > 0) {
    showPreheat();
  } else {
    if (ppm < 1000) {
      lcd.print(" ");
    }
    lcd.print(ppm);
    lcd.print("  ");
    if (ppm < 800) {
      lcd.print("  GOOD");
      analogWrite(LED_RED, 0);
      analogWrite(LED_GREEN, 200);
    } else if(ppm < 1200) {
      lcd.print("  NORM");
      analogWrite(LED_RED, 30);
      analogWrite(LED_GREEN, 70);
    } else if(ppm < 2000) {
      lcd.print("  BAD ");
      analogWrite(LED_RED, 70);
      analogWrite(LED_GREEN, 30);
    } else {
      lcd.print("  HELL");
      analogWrite(LED_RED, 200);
      analogWrite(LED_GREEN, 0);
    }
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Temp ");
  lcd.print(dt);
  lcd.print("   ");
  lcd.print(st);

  lcd.setCursor(0, 2);
  lcd.print("Humd ");
  lcd.print(dh);
  
  lcd.setCursor(0, 3);
  lcd.print("mmHg ");
  lcd.print(bp * 0.0075);
  
  //delay(1000);
}
