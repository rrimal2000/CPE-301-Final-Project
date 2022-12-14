#include <RTClib.h>
#include <Arduino.h>
#include <Wire.h>
#include <dht_nonblocking.h>
#include <LiquidCrystal.h>
unsigned char* port_b = (unsigned char*) 0x25; 
unsigned char* ddr_b = (unsigned char*) 0x24;
unsigned char* const pin_e = (unsigned char*) 0x2C;
unsigned char* const port_e = (unsigned char*) 0x2E;
unsigned char* const ddr_e= (unsigned char*) 0x2D;
unsigned char* const port_h = (unsigned char*) 0x102;
unsigned char* const ddr_h = (unsigned char*) 0x101;
volatile uint16_t* const tcnt_1 = (uint16_t*) 0x84;
unsigned char* const tccr1_a = (unsigned char*) 0x80;
unsigned char* const tccr1_b = (unsigned char*) 0x81;
unsigned char* const timsk_1 = (unsigned char*) 0x6F;
volatile uint16_t* const tcnt_3 = (uint16_t*) 0x94;
unsigned char* const tccr3_a = (unsigned char*) 0x90;
unsigned char* const tccr3_b = (unsigned char*) 0x91;
unsigned char* const timsk_3 = (unsigned char*) 0x71;
volatile uint16_t* const adcValue = (uint16_t*) 0x78;
unsigned char* const admux = (unsigned char*) 0x7c;
unsigned char* const adcsr_a = (unsigned char*) 0x7a;
unsigned char* const adcsr_b = (unsigned char*) 0x7b;
unsigned char* const eimsk = (unsigned char*) 0x3D;
unsigned char* const eicrb = (unsigned char*) 0x6A;
enum State {Disabled, Idle, Running, Error} state;
RTC_DS1307 clock;
LiquidCrystal lcd(37, 35, 33, 31, 29, 27);
#define TEMPERATURE_DISABLE 77.5
#define WATER_LEVEL_DISABLE 400
#define WATER_LEVEL_DISABLE 200

#define DHT_SENSOR_PIN 14
#define DHT_SENSOR_TYPE DHT_TYPE_11
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

ISR(TIMER1_OVF_vect)
{
    *tccr1_b &= 0b11111000;

    if(!(*pin_e & (1 << 4)))
    {
        if(state == Disabled) 
        {
          changeState(Idle);
        }
        else
        {
          changeState(Disabled);
        }
    }
}
ISR(INT4_vect)
{
    *tcnt_1 = 34286; 
    *tccr1_b = (1 << 2);  
}
ISR(TIMER3_OVF_vect)
{
    *tccr3_b &= 0b11111000;
    if(state == Running || state == Idle)
    {
        if(*adcValue <= WATER_LEVEL_DISABLE)
        {
            changeState(Error);
        } 
    }
    else if (state == Error)
    {
      if(*adcValue > WATER_LEVEL_DISABLE) 
      {
         changeState(Idle);
      }
    }
}
ISR(ADC_vect)
{
    if(state == Running || state == Idle)
    {
        if(!timerRunning() && *adcValue <= WATER_LEVEL_DISABLE)
        {
            *tcnt_3 = 34286;
            *tccr3_b = (1 << 2);
        }
    } 
    else if (state == Error)
    {
        if(!timerRunning() && *adcValue > WATER_LEVEL_DISABLE) 
        {
            *tcnt_3 = 34286;
            *tccr3_b = (1 << 2); 
        }
    }
    *adcsr_a |= (1 << 6);
}
void setup(){   
    Serial.begin(9600);
    lcd.begin(16, 2); 
    lcd.print("Disabled");
    clock.begin();
    clock.adjust(DateTime(F(__DATE__), F(__TIME__)));
    *ddr_h = 0b01111000;
    *ddr_b |= 0x01 << 7;
    buttonSetup();
    adcInit(0);
    state = Disabled;

    interrupts();
}
void loop()
{
    static float temp, humid;
    if(state == Running || state == Idle)
    {
        if(dht_sensor.measure(&temp, &humid)) 
        {
            noInterrupts();
            temp = temp * 9.0/5 + 32;
            lcd.setCursor(0, 1);
            lcd.print(temp, 1);
            lcd.print(" F");
            lcd.print(" - ");
            lcd.print(humid, 1); 
            lcd.print("%");

            interrupts();

            if((temp > TEMPERATURE_DISABLE) && (state == Idle))
            {
                changeState(Running);
            }
            else if((temp < TEMPERATURE_DISABLE) && (state == Running))
            {
                changeState(Idle);
            }
        }
    }
    if(state == Running) 
    {
       *port_b |= (0x01 << 7);
    }
    else
    {
       *port_b &= ~(0x01 << 7);
    }
    if(state == Error)
    {
        noInterrupts();
        lcd.setCursor(0, 1);
        lcd.print("Water Left ");
        lcd.print(*adcValue, 1);
        interrupts();
    }
}
char stateCharacter(State state)
{
      if(state == Disabled)
      { 
        return 'D';
      }
      else if (state == Idle)
      {
        return 'I';
      }
      else if(state == Running)
      {
        return 'R';
      }
      else if(state == Error)
      {
        return 'E';
      }
      else
      {
        return 'U';
      }
}
void changeState(State alteredState)
{
    noInterrupts();
    if(state == Disabled)
    {
        adcInit(0);
    }
    switch(alteredState)
    {
    case Disabled:
              adcDisable();
              useLED(6);
              lcd.clear();
              lcd.print("Disabled");
        break;      
    case Idle:
              useLED(5);
              lcd.clear();
              lcd.print("Idling");
        break;case Running:
              useLED(4);
              lcd.clear();
              lcd.print("Running Fan");
        break;
    case Error:
              useLED(3);
              lcd.clear();
              lcd.print("Error: No Water");
        break;
    }
    Serial.print(stateCharacter(state));
    state = alteredState;

    interrupts();
    
    Serial.print(" -> ");
    Serial.print(stateCharacter(state));
    Serial.print(" happens at ");
    printTime();
}void useLED(int pin)
{
    *port_h &= 0b10000111;
    *port_h |= (0x01 << pin); 
}
void buttonSetup()
{
    *tccr1_a = 0;
    *tccr1_b = 0;
    *timsk_1 = 1;
    *ddr_e = 0;
    *port_e = (1 << 4);
    *eimsk = (1 << 4);
    *eicrb = 0b00000010;
    *port_b &= ~(0x01 << 7);
}
void adcInit(uint8_t channel)
{
    *tccr3_b = 0;   // Enable the overflow interrupt
    *timsk_3 = 1; 
    *tccr3_a = 0; 
    *adcsr_a = 0b10001000;
    *admux = 0b01000000;
    *adcsr_b = 0;
    if(channel >= 8) 
    {
        channel -= 8; 
        *adcsr_b |= (1 << 3);
    }
    *admux |= channel;
    *adcsr_a |= (1 << 6);
}

void adcDisable()
{
    *adcsr_a = 0;
}
bool timerRunning()
{
    return *tccr3_b &= 0b0000111;
}
void printTime()
{
    DateTime now = clock.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
}