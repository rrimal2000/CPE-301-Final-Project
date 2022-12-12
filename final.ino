#include "DHT.h"
#include <LiquidCrystal.h>
#define TERMPERATURE_THRESHOLD 80
#define WATER_LEVEL_THRESHOLD 100 
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(8, 7, 6, 5, 4, 3); 
 volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
 volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
 volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
 volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
 volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
enum state {
   off = 0,
   idle = 1,
   temp = 2,
   water = 3
};
enum state stat = off;
void setup() {
  adc_init();
  Serial.begin(9600);
  lcd.begin(16, 2) 
}
void loop() {
  delay(1000);
  unsigned int w = adc_read(WATER_LEVEL_PORT);
  float temperature = tempRead(true);
  float humid = humidRead();
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(f);
  Serial.print(F(" Water: "));
  Serial.print(w);
  Serial.print('\n');
  switch(stat) {
    case off:
      Serial.println("Disabled State");
      disabled_state();
      break;
    case idle:
      Serial.println("Idle State");
      idle_state();
      break;
    case water:
      Serial.println("Error State");
      error_state();
      break;
    case temp:
      Serial.println("Running State");
      running_state();
      break;
    default:
      break;
  }
}
float tempRead() 
{
  float t;
  t = dht.readTemperature(true);   //Fahrenheit
  if (isnan(t)) Serial.println(F("Failed to read temperature."));
  return t;
}
float humidRead()
{
  float h = dht.readHumidity();
  if (isnan(h)) Serial.println(F("Failed to read humidity."));
  return h;
}
void lcd_th(float t, float h) {
  lcd.setCursor(0, 0);
  lcd.print("Temp:  Humidity:");
  lcd.setCursor(0, 1);
  lcd.print(t);
  lcd.setCursor(7, 1);
  lcd.print(h);
}