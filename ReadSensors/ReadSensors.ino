#include "DFRobot_PH.h"
#include "DFRobot_EC.h"
#include "GravityTDS.h"
/************************************************
 * readsensors.ino - created by Kylie Outlaw
 * and Maria Abbasi for the HydroHome project.
 ************************************************/

 /************************************************
 * Includes
 ************************************************/
#include <EEPROM.h>
#include <OneWire.h>

/************************************************
 * Definitions
 ************************************************/
#define PHSENSORPIN     A5    /* pH sensor              */
#define LEVELSENSORPIN  A6    /* water level sensor     */
#define ECSENSORPIN     A7    /* EC sensor              */
#define TDSSENSORPIN    A8    /* TDS sensor             */
#define TEMPSENSORPIN   A9    /* temp sensor            */

#define REEDSWITCH      22    /* reed switch, tray loc  */
#define DRIVERDIR       A14   /* motor direction        */
#define DRIVERPUL       A15

#define SCOUNT          30
#define EEPROM_DIR      0
#define PHOFFSET        0.04

/************************************************
 * Global Variables
 ************************************************/
int   TDSbuff[SCOUNT]; //TDS buffer to hold samples of TDS
int   TEMPbuff[SCOUNT]; //Temp buffer to hold samples of TEMPERATURE
float ECbuff[SCOUNT]; //EC buffer to hold samples of EC
int   PHbuff[SCOUNT]; //pH buffer to hold samples of PH

static int  buffIdx     = 0; 
int         cpyIdx      = 0;
float       avgVolt     = 0; //value for average voltage
float       tdsValue    = 0; //value for tds
float       temperature = 25; //reference temperature
float       ec_voltage   = 5; //ref ec voltage
static int  timer_count = 0;
static bool flag    = false;
float       ph_voltage  = 0;


float ecvalue;

int     driver_speed  = 700;
boolean driver_dir    = EEPROM.read( EEPROM_DIR );

const uint16_t t1_load = 0;
const uint16_t t1_comp = 62500;

OneWire ds(TEMPSENSORPIN);
DFRobot_EC EC_sensor;
GravityTDS TDS_sensor;
DFRobot_PH PH_sensor;


/************************************************
 * Begin the setup
 ************************************************/
void setup() {
  Serial.begin(9600);

  /************************************************
   * Set up I/O
   ************************************************/
  pinMode( TDSSENSORPIN,    INPUT );
  pinMode( TEMPSENSORPIN,   INPUT );
  pinMode( LEVELSENSORPIN,  INPUT );
  pinMode( PHSENSORPIN,     INPUT );
  pinMode( DRIVERPUL,       OUTPUT );
  pinMode( DRIVERDIR,       OUTPUT );

  /************************************************
   * Set up TDS Sensor
   ************************************************/
  TDS_sensor.setPin(TDSSENSORPIN);
  TDS_sensor.setAref(5.0);
  TDS_sensor.setAdcRange(1024);
  TDS_sensor.setTemperature(25);
  TDS_sensor.begin();

  /************************************************
   * Set up EC, PH Sensor
   ************************************************/
  EC_sensor.begin();
  PH_sensor.begin();

  /************************************************
   * Set up timer, 1024 prescalar, PWM
   * compare interrupt
   ************************************************/
  TCCR1A = 0;

  TCCR1B &= ~(1<<WGM13);
  TCCR1B |= (1<<WGM12);

  TCCR1B |= (1<<CS12);
  TCCR1B |= (1<<CS10);
  TCCR1B &= ~(1<<CS11);

  TCNT1 = t1_load;
  OCR1A = t1_comp;

  TIMSK1 = (1<<OCIE1A);
  sei();  
}

/************************************************
 * Monitoring Loop
 ************************************************/
void loop() {  

  /************************************************
   * Check if reed switch tripped
   ************************************************/
  if( digitalRead(REEDSWITCH) == 1 )
  {
    reverse_rotate();
  }
  
  if(flag == true)
  {
    /************************************************
     * Flag indicating sensor read happens every 4s
     ************************************************/
    TEMPbuff[buffIdx] = getTemp();

    TDS_sensor.update();
    TDSbuff[buffIdx] = TDS_sensor.getTdsValue();

    ec_voltage = analogRead(ECSENSORPIN)/1024.0*5000; //read voltage of ec
    ECbuff[buffIdx] = EC_sensor.readEC(ec_voltage, TEMPbuff[buffIdx]);

    ph_voltage = analogRead(PHSENSORPIN);
    PHbuff[buffIdx] = ph_voltage;
        
    buffIdx++; //increase buffer indexes

    /************************************************
     * Print data every 4*SCOUNT seconds
     * TODO: Update with print to LCD
     ************************************************/
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

      //Print pH
      double voltage = getAvgPH(PHbuff, SCOUNT)*5.0/1024;
      double pHAvg = 3.5*voltage+PHOFFSET;
      Serial.print("pH: ");
      Serial.print(pHAvg, 2);
      Serial.print("\n");
      
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

      //KO TODO: Replace these statements with equivalent LCD Outputs
      // Check for unstable points. If we reach any, output error message.
      if(tdsAvg < 300){ Serial.print("\nTDS low. Add nutrient solution.\n");}
      if(tempAvg < 3) { Serial.print("\nTemperature low. Please move plant to warmer area.\n");}
      if(tempAvg > 30){ Serial.print("\nTemperature high. Please move plant to cooler area.\n");}
      
      Serial.print("\n* * * * * * * * * *\n");


      //Run the motor just a little
      // Each "j" is 1.4ms
      for( int j = 0; j < 10000; j++ )
        {
        digitalWrite(DRIVERDIR, driver_dir);
        digitalWrite(DRIVERPUL, HIGH);
        delayMicroseconds(driver_speed);
        digitalWrite(DRIVERPUL, LOW);
        delayMicroseconds(driver_speed);
        }
    }
    flag = false;
  }
}

ISR(TIMER1_COMPA_vect)
{
flag = true;
}
