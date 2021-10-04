#include "DFRobot_EC.h"
#include "GravityTDS.h"
#include <EEPROM.h>
#include <OneWire.h>
#include <dht.h>

#define EcSensorPin A0
#define TdsSensorPin A1
#define TempSensorPin 3
#define LevelSensorPin 4

#define SCOUNT 30 //sample count

int TDSbuff[SCOUNT]; //buffer to hold samples of TDS
int TEMPbuff[SCOUNT]; //buffer to hold samples of TEMPERATURE
float ECbuff[SCOUNT]; //temp buffer

static int buffIdx = 0; 
int cpyIdx = 0;
float avgVolt = 0; //value for average voltage
float tdsValue = 0; //value for tds
float temperature = 25; //reference temperature
float ecvoltage = 5; //ref ec voltage
float ecvalue;
static bool flag = false;
static int timer_count = 0;

const uint16_t t1_load = 0;
const uint16_t t1_comp = 62500;

OneWire ds(TempSensorPin);
DFRobot_EC EC_sensor;
GravityTDS TDS_sensor;

/************************************************
 * Begin the setup
 ************************************************/
void setup() {
  Serial.begin(9600);
  pinMode(TdsSensorPin, INPUT);
  pinMode(TempSensorPin, INPUT);
  pinMode(LevelSensorPin, INPUT);

  //Set up TDS Sensor
  TDS_sensor.setPin(TdsSensorPin);
  TDS_sensor.setAref(5.0);
  TDS_sensor.setAdcRange(1024);
  TDS_sensor.setTemperature(25);
  TDS_sensor.begin();

  //Set up EC sensor
  EC_sensor.begin();
  
  //Reset timer1 control register A
  TCCR1A = 0;

  //Enable PWM mode
  TCCR1B &= ~(1<<WGM13);
  TCCR1B |= (1<<WGM12);

  //Set prescalar to 1024
  TCCR1B |= (1<<CS12);
  TCCR1B |= (1<<CS10);
  TCCR1B &= ~(1<<CS11);

  //Set up timer compare and load values
  TCNT1 = t1_load;
  OCR1A = t1_comp;

  // Enable compare interrupt
  TIMSK1 = (1<<OCIE1A);

  //Enable global interrupts
  sei();  
}

/************************************************
 * Monitoring Loop
 ************************************************/
void loop() {  
  if(flag == true)
  {
    //Read Temperature
    TEMPbuff[buffIdx] = getTemp();

    //Read TDS
    TDS_sensor.update();
    TDSbuff[buffIdx] = TDS_sensor.getTdsValue();

    //Read EC
    ecvoltage = analogRead(EcSensorPin)/1024.0*5000; //read voltage of ec
    ECbuff[buffIdx] = EC_sensor.readEC(ecvoltage, TEMPbuff[buffIdx]);
    
    buffIdx++; //increase buffer indexes
    
    // Print the data every 120 seconds
    if(buffIdx == SCOUNT)
    {      
      // Print Temperature Avg
      float tempAvg = getAvgVal(TEMPbuff, SCOUNT);
      Serial.print("Temperature:");
      Serial.print(tempAvg*1.8 +32);
      Serial.print("°F\n");
      buffIdx = 0;
      
      // Print TDS Avg
      TDS_sensor.setTemperature(tempAvg);
      TDS_sensor.update();
      
      float tdsAvg = getAvgVal(TDSbuff, SCOUNT);
      Serial.print("TDS:");
      Serial.print(tdsAvg,2);
      Serial.println("ppm\n");

      //Print EC
      float ecAvg = getAvgValFlt(ECbuff, SCOUNT);
      Serial.print("EC: ");
      Serial.print(ecAvg, 2);
      Serial.print(" ms/cm\n");      
      
      // Print Liquid Level
      int liquidLvl = digitalRead(4);
      if( liquidLvl )
      {
        Serial.print("\nLiquid level is fine.");
      }
      else
      {
        Serial.print("\nLiquid level needs refill. ");
        Serial.print("Liquid level is at:");
        Serial.print(liquidLvl);
      }

      // Check for unstable points. If we reach any, output error message.
      if(tdsAvg < 300) { Serial.print("\nTDS low. Add nutrient solution.\n");}
      if(tempAvg < 3){ Serial.print("\nTemperature low. Please move plant to warmer area.\n");}
      if(tempAvg > 30){ Serial.print("\nTemperature high. Please move plant to cooler area.\n");}
      
      Serial.print("\n* * * * * * * * * *\n");
    }
    flag = false;
  }
}

ISR(TIMER1_COMPA_vect)
{
flag = true;
}
