
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

    /**********If Home button is pressed on sensor data screen it takes you to main menu****/
    if (p.x >= 285 && p.x <= 320 && p.y >= 15 && p.y <= 55 && currentPage == 2)
      {
      Serial.println("Home button on screen 2 Selected");
      currentPage = 1;       
      HomeScreen();  
      }
    /**********If Home button pressed on Rotate tray screen it takes you to main menu****/
    if (p.x >= 5 && p.x <= 30 && p.y >= 12 && p.y <= 55 && currentPage == 3) 
      {
        Serial.println("Home button on screen 3 Selected");
         currentPage = 1;       
        HomeScreen();  
      }   
    }

if (currentPage == 1)
  {
  if (p.x > 563 && p.x < 683 && p.y > 275 && p.y < 750)
    {
    Serial.println("Rotate Tray");
    tft.print("Rotate Trays");
    delay(70);
  
    currentPage = 3;
    x = 0;
    y = 0;
    p.z = 0;
    RotateTrayScreen();
    }
  if (p.x > 403 && p.x < 525 && p.y > 271 && p.y < 725)
  //if (p.x > 60 && p.x < 270 && p.y > 80 && p.y < 290)
    {
    Serial.println("Sensor Data");
    currentPage = 2;
    tft.setCursor(80, 95);
    tft.print("Sensor Data");
    SensorDataScreen();
    }
  else if (p.x > 736 && p.x < 855 && p.y > 255 && p.y < 725)
    {
    Serial.println("Meter");
    currentPage=4;
    tft.setCursor(80, 95);
    tft.print("Meter");
    
    meterscreen();
    }
  }
}

ISR(TIMER1_COMPA_vect)
{
flag = true;
}
