#include <OneWire.h>
#include <dht.h>

#define TdsSensorPin A0
#define TempSensorPin 3
#define HumSensorPin 12

#define VREF 5.0 //reference voltage for TDS
#define SCOUNT 30 //sample count
int TDSbuff[SCOUNT]; //buffer to hold samples of TDS
int TEMPbuff[SCOUNT]; //buffer to hold samples of TEMPERATURE
float HUMbuff[SCOUNT]; //buffer to hold samples of HUMIDITY
int bufferTemp[SCOUNT]; //temp buffer
static int buffIdx = 0; 
int cpyIdx = 0;
float avgVolt = 0; //value for average voltage
float tdsValue = 0; //value for tds
float temperature = 25; //reference temperature
static bool flag = false;

const uint16_t t1_load = 0;
const uint16_t t1_comp = 31250;

OneWire ds(TempSensorPin);
dht DHT;

void setup() {
  Serial.begin(9600);
  pinMode(TdsSensorPin, INPUT);
  pinMode(TempSensorPin, INPUT);
  pinMode(HumSensorPin, INPUT);

  //Reset timer1 control register A
  TCCR1A = 0;

  //Enable CTC mode
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

void loop() {  
delay(500);
  if(flag == true)
  {
    int DHTSample = DHT.read11(HumSensorPin);
    HUMbuff[buffIdx] = DHT.humidity;
    TDSbuff[buffIdx] = analogRead(TdsSensorPin);
    TEMPbuff[buffIdx] = getTemp();
    buffIdx++; //increase buffer indexes
    
    // Print the data every 60 seconds
    if(buffIdx == SCOUNT)
    {
      // Print TDS Avg
      float tds = getAvgTDS(TDSbuff, SCOUNT);
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
      
      // Print Temperature Avg
      float temperature = getAvgTemp(TEMPbuff, SCOUNT);
      Serial.print("Temp Value:");
      Serial.print(temperature*1.8 +32);
      Serial.print("°F\n");
  
      //Print Humidity Avg
      float humidityAvg = getAvgHum(HUMbuff, SCOUNT);
      Serial.print("Humidity Value:");
      Serial.print(humidityAvg);
      Serial.print("\n* * * * * * * * * *\n");
      buffIdx = 0;
    }
    flag = false;
  }
}

ISR(TIMER1_COMPA_vect)
{
flag = true;
}
