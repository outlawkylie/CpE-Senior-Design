#include <OneWire.h>

#define TdsSensorPin A0
#define TempSensorPin 3

#define VREF 5.0
#define SCOUNT 30
int TDSbuff[SCOUNT];
int TEMPbuff[SCOUNT];
int bufferTemp[SCOUNT];
int buffIdx = 0;
int cpyIdx = 0;
float avgVolt = 0;
float tdsValue = 0;
float temperature = 25;

OneWire ds(TempSensorPin);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(TdsSensorPin, INPUT);
  pinMode(TempSensorPin, INPUT);
}

void loop() {  
  // put your main code here, to run repeatedly:
  static unsigned long analogSampleTime = millis();
  if( millis()-analogSampleTime > 100U)
  {
    analogSampleTime = millis();
    TDSbuff[buffIdx] = analogRead(TdsSensorPin);
    TEMPbuff[buffIdx] = getTemp();
    buffIdx++;
    if(buffIdx == SCOUNT)
    {
      buffIdx = 0;
    }
  }
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 1000U)
   {
      printTimepoint = millis();
      for(cpyIdx=0;cpyIdx<SCOUNT;cpyIdx++)
        bufferTemp[cpyIdx]= TDSbuff[cpyIdx];
      avgVolt = getMedianNum(bufferTemp,SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=avgVolt/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(avgVolt,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");

      // Temperature Avg
      float temperature = getTemp();
      Serial.print("Temp Value:");
      Serial.print(temperature*1.8 +32);
      Serial.print("°F\n");
      Serial.print("* * * * * * * * * *\n");
   }
}

//For TDS
int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}

float getTemp(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;
}
