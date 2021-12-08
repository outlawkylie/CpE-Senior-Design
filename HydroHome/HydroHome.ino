
/************************************************
 * HydroHome.ino - created by Kylie Outlaw
 * and Maria Abbasi for the HydroHome project.
 ************************************************/

 /************************************************
 * Includes
 ************************************************/
#include <EEPROM.h>
#include <OneWire.h>
#include <Elegoo_GFX.h>       // Core graphics library
#include <Elegoo_TFTLCD.h>    // Hardware-specific library
#include "DFRobot_PH.h"
#include "DFRobot_EC.h"
#include "GravityTDS.h"
#include "HydroHome.h"
#include "TouchScreen.h"

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
   * Set up EC, PH Sensor, LCD Screen
   ************************************************/
  EC_sensor.begin();
  PH_sensor.begin();

  HomeScreen();

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
      tempAvg = getAvgVal(TEMPbuff, SCOUNT);
      Serial.print("Temperature:");
      Serial.print(tempAvg*1.8 +32);
      Serial.print("°F\n");
      buffIdx = 0;
      
      // Print TDS Avg
      TDS_sensor.setTemperature(tempAvg);
      TDS_sensor.update();
      
      tdsAvg = getAvgVal(TDSbuff, SCOUNT);
      Serial.print("TDS:");
      Serial.print(tdsAvg,2);
      Serial.println("ppm\n");

      //Print EC
      ecAvg = getAvgValFlt(ECbuff, SCOUNT);
      Serial.print("EC: ");
      Serial.print(ecAvg, 2);
      Serial.print(" ms/cm\n");      

      //Print pH
      double voltage = getAvgPH(PHbuff, SCOUNT)*5.0/1024;
      pHAvg = 3.5*voltage+PHOFFSET;
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

  /************************************************
   * Data for LCD Screen
   ************************************************/
  TSPoint p = ts.getPoint();
  pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) 
    {
    // scale from 0->1023 to tft.width
    p.x = map(p.x, TS_MIN_X, TS_MAX_X, tft.width(), 0);
    p.y = (tft.height() - map(p.y, TS_MIN_Y, TS_MAX_Y, tft.height(), 0));

    /**********If Home button is pressed on any other screen, it takes you to main menu****/
    if (p.x >= 285 && p.x <= 320 && p.y >= 15 && p.y <= 55 && currentPage != HOME_PAGE )
      {
      currentPage = HOME_PAGE;       
      HomeScreen();  
      delay(100);
      }
    }

if (currentPage == HOME_PAGE)
  {
  if (p.x > 403 && p.x < 525 && p.y > 271 && p.y < 725)
    {
    currentPage = SENSOR_PAGE;
    tft.setCursor(80, 95);
    tft.print("Sensor Data");
    SensorDataScreen();
    delay(100);
    }
  if (p.x > 563 && p.x < 683 && p.y > 275 && p.y < 750)
    {
    tft.print("Rotate Trays");
    currentPage = ROTATE_PAGE;
    x = 0;
    y = 0;
    p.z = 0;
    RotateTrayScreen();
    delay(100);
    }
  else if (p.x > 736 && p.x < 855 && p.y > 255 && p.y < 725)
    {
    currentPage = METER_PAGE;
    tft.setCursor(80, 95);
    tft.print("Meter");
    meterscreen();
    delay(100);
    }
  }
}

ISR(TIMER1_COMPA_vect)
  {
  flag = true;
  }
