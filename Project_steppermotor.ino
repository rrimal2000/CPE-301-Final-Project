#include "Stepper.h"
#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"


volatile unsigned char *myEICRA = (unsigned char *)0x69;
volatile unsigned char *myEIMSK = (unsigned char *)0x3D;
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
volatile unsigned char* port_a = (unsigned char*) 0x22; 
volatile unsigned char* ddr_a  = (unsigned char*) 0x21; 
volatile unsigned char* pin_a  = (unsigned char*) 0x20; 
volatile unsigned char* port_b = (unsigned char*) 0x25; 
volatile unsigned char* ddr_b  = (unsigned char*) 0x24; 
volatile unsigned char* pin_b  = (unsigned char*) 0x23; 
volatile unsigned char* port_d = (unsigned char*) 0x2B; 
volatile unsigned char* ddr_d  = (unsigned char*) 0x2A; 
volatile unsigned char* pin_d  = (unsigned char*) 0x29; 
volatile unsigned char *admux = (unsigned char *) 0x7C;
volatile unsigned char *adcsrb = (unsigned char *) 0x7B;
volatile unsigned char *adcsra = (unsigned char *) 0x7A;
volatile unsigned char *adcl = (unsigned char *) 0x78;
volatile unsigned char *adch = (unsigned char *) 0x79;

RTC_DS1307 rtc;

Stepper small_stepper(500, 10, 12, 11, 13); 
volatile long ventPos = 0;

const int rs = 9, en = 8, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
bool transition = false;
unsigned long prevStartTime = 0;
unsigned long prevResetTime = 0;
ISR (INT2_vect){
  unsigned long newTime = millis();
  if(newTime - prevStartTime > 250){
    if(!(*port_a & 0x04)){
      toDisabled();
    }
    else{
      toIdle();
    }    
    prevStartTime = newTime;
  }
}
ISR (INT3_vect){
	
}
void setup() {
  *ddr_b |= 0xF0;
  *ddr_a |= 0x1F;
  *ddr_d &= 0xF3;
  *port_d |= 0x0C;
  *myEICRA |= 0xA0;
  *myEICRA &= 0xAF; 
  *myEIMSK |= 0x0C; 
  small_stepper.setSpeed(30);
  U0init(9600);
  adc_init();
  lcd.begin(16, 2);
  rtcSetup();
  
  toDisabled();
}
void loop() {

}
void adc_init(){
  *adcsra = 0x80;
  *adcsrb = 0x00;
  *admux = 0x40;
}
unsigned int adc_read(unsigned char adc_channel){
  *admux |= (adc_channel & 0x7);
  *adcsrb |= (adc_channel & 0x8);
  *adcsra |= 0x40;
  while(*adcsra & 0x40);
  uint8_t l = ADCL;
  uint8_t h = ADCH;
  *admux = 0x40;
  *adcsrb = 0x00;
  return (h << 8) | l;
}
void U0init(unsigned long U0baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
void putStr(char str[]){
  int i = 0;
  while(str[i] != '\0'){
     while((*myUCSR0A & 0x20) == 0);
     *myUDR0 = str[i];
     i++;
  }
}
void putInt(int in){
  char str[20];
  sprintf(str, "%d", in);
  putStr(str);
}
void moveVent(unsigned int newPos){
  long steps = newPos - ventPos;
  if(steps > 100 || steps < -100){
    Serial.print(steps);
    Serial.print("\n");
     rtcPrint();
     putStr(" Vent Movement ");
     putInt(steps);
     putStr("\n");
     small_stepper.step(steps);
     ventPos = newPos;
     *port_b &= 0x0F;
  }
}
void rtcSetup(){
  if (! rtc.begin()) {
    Serial.flush();
    while (1) delay(10);
  }
  
  if (! rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}
void rtcPrint(){
    DateTime now = rtc.now();

    putInt(now.year());
    putStr("/");
    putInt(now.month());
    putStr("/");
    putInt(now.day());
    putStr(" ");
    putInt(now.hour());
    putStr(":");
    putInt(now.minute());
    putStr(":");
    putInt(now.second());
}
void toDisabled(){
  transition = true;
  *port_a &= 0xE0;
  *port_a |= 0x04;
}
void toIdle(){
  transition = true;
  *port_a &= 0xE0;
  *port_a |= 0x02;
}
void toRunning(){
  transition = true;
  *port_a &= 0xE0;
  *port_a |= 0x11;
}
void toError(){
  transition = true;
  *port_a &= 0xE0;
  *port_a |= 0x08;
}