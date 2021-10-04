#include <OneWire.h>
#include <dht.h>

#define TdsSensorPin A0
#define TempSensorPin 3
#define LevelSensorPin 4

#define VREF 5.0 //reference voltage for TDS
#define SCOUNT 30 //sample count
int TDSbuff[SCOUNT]; //buffer to hold samples of TDS
int TEMPbuff[SCOUNT]; //buffer to hold samples of TEMPERATURE
int bufferTemp[SCOUNT]; //temp buffer
static int buffIdx = 0; 
int cpyIdx = 0;
float avgVolt = 0; //value for average voltage
float tdsValue = 0; //value for tds
float temperature = 25; //reference temperature
static bool flag = false;
static int timer_count = 0;

const uint16_t t1_load = 0;
const uint16_t t1_comp = 62500;

OneWire ds(TempSensorPin);
DFRobot_EC EC_sensor;

/************************************************
 * Begin the setup
 ************************************************/
void setup() {
  Serial.begin(9600);
  pinMode(TdsSensorPin, INPUT);
  pinMode(TempSensorPin, INPUT);
  pinMode(LevelSensorPin, INPUT);

  //Reset timer1 control register A
  TCCR1A = 0;

  //Enable PWM mode
  TCCR1B &= ~(1<<WGM13);
  TCCR1B |= (1<<WGM12);

  //Set prescalar to 1024
  //TCCR1B |= (1<<CS12);
  //TCCR1B |= (1<<CS10);
  //TCCR1B &= ~(1<<CS11);

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
    TDSbuff[buffIdx] = analogRead(TdsSensorPin);
    TEMPbuff[buffIdx] = getTemp();
    buffIdx++; //increase buffer indexes
    
    // Print the data every 120 seconds
    if(buffIdx == SCOUNT)
    {
      // Print TDS Avg
      float tdsAvg = getAvgTDS(TDSbuff, SCOUNT);
      Serial.print("TDS:");
      Serial.print(tdsAvg,2);
      Serial.println("ppm\n");
      
      // Print Temperature Avg
      float tempAvg = getAvgTemp(TEMPbuff, SCOUNT);
      Serial.print("Temperature:");
      Serial.print(tempAvg*1.8 +32);
      Serial.print("°F\n");
      buffIdx = 0;

      // Print Liquid Level
      int liquidLvl = digitalRead(4);
      if( liquidLvl )
      {
        Serial.print("Liquid level is fine.");
      }
      else
      {
        Serial.print("Liquid level needs refill.");
        Serial.print("Liquid level is at:");
        Serial.print(liquidLvl);
      }

      // Check for unstable points. If we reach any, output error message.
      if(tdsAvg < 300) { Serial.print("TDS low. Add nutrient solution.\n");}
      if(tempAvg < 3){ Serial.print("Temperature low. Please move plant to warmer area.\n");}
      if(tempAvg > 30){ Serial.print("Temperature high. Please move plant to cooler area.\n");}
      
      Serial.print("\n* * * * * * * * * *\n");
    }
    flag = false;
  }
}

ISR(TIMER1_COMPA_vect)
{
flag = true;
}
