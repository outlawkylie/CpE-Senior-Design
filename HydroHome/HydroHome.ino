
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
#include <SoftwareSerial.h>

SoftwareSerial hblue(0,1);

/************************************************
* Begin the setup
************************************************/
void setup() {
  Serial.begin(9600);
  hblue.begin(38400); 
  
  /************************************************
  * Set up I/O
  ************************************************/
  pinMode( TDSSENSORPIN,    INPUT );
  pinMode( TEMPSENSORPIN,   INPUT );
  pinMode( LEVELSENSORPIN,  INPUT );
  pinMode( PHSENSORPIN,     INPUT );
  pinMode( DRIVERPUL,       OUTPUT );
  pinMode( DRIVERDIR,       OUTPUT );
  pinMode( TRAY1_DRAIN,     OUTPUT );
  pinMode( TRAY2_DRAIN,     OUTPUT );
  pinMode( TRAY3_DRAIN,     OUTPUT );
  pinMode( TRAY4_DRAIN,     OUTPUT );
  pinMode( FILL_SWITCH,     OUTPUT );
  pinMode( GROWLIGHTPIN,    OUTPUT );
  

  /************************************************
  * Set up TDS Sensor
  ************************************************/
  TDS_sensor.setPin(TDSSENSORPIN);
  TDS_sensor.setAref(5.0);
  TDS_sensor.setAdcRange(1024);
  TDS_sensor.setTemperature(25);
  TDS_sensor.begin();

  /************************************************
  * Set up Grow Light to Always Be On
  ************************************************/
  digitalWrite( GROWLIGHTPIN, HIGH );

  /************************************************
  * Set up EC, PH Sensor, LCD Screen
  ************************************************/
  EC_sensor.begin();
  PH_sensor.begin();

  HomeScreen();
  currentPage = HOME_PAGE;

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
void loop() 
  {  
    char ph;
char ec;
char tds;
char temp;
char liquid;

ph= Serial.read();
ec= Serial.read();
tds=Serial.read();
temp= Serial.read();
liquid= Serial.read();
    while (hblue.available())
    {
  
  /************************************************
  * Check if reed switch tripped, change direction
  ************************************************/
  if( digitalRead(REEDSWITCH) == 1 && prv_reed == 0 )
    {
    prv_reed = 1;
    reverse_rotate_dir();
    }
  if( digitalRead(REEDSWITCH) == 0 && prv_reed == 1 )
    {
    prv_reed = 0;
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
      buffIdx = 0;
      
      // Print TDS Avg
      TDS_sensor.setTemperature(tempAvg);
      TDS_sensor.update();
      
      tdsAvg = getAvgVal(TDSbuff, SCOUNT);

      //Print EC
      ecAvg = getAvgValFlt(ECbuff, SCOUNT);      

      //Print pH
      double voltage = getAvgPH(PHbuff, SCOUNT)*5.0/1024;
      pHAvg = 3.5*voltage+PHOFFSET;
      
      // Print Liquid Level
      liquidLvl = digitalRead(4);
      
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
    else if (p.x > 563 && p.x < 683 && p.y > 275 && p.y < 750)
      {
      tft.print("Rotate Trays");
      currentPage = ROTATE_PAGE;
      x = 0;
      y = 0;
      p.z = 0;      
      RotateTrayScreen();
      rotate();
      delay(100);
      }
    }
  }

  if(hblue.available())
  {
    char input;
    input = Serial.read();
    
    if( input == 'p' ) {
      hblue.write( pHAvg );
    }
    if( input == 'e' ){
      hblue.write(ecAvg);
    }
    if( input=='d' ){
      hblue.write(tdsAvg);
    }
    
    if (input =='t'){
      hblue.write(tempAvg);
    }
    if(input =='l'){
      
    }
    
 
    // Feed all data from termial to bluetooth
    if (Serial.available()){
      hblue.write(Serial.read());
    }
  }
}
  
ISR(TIMER1_COMPA_vect)
  {
  flag = true;
  }
