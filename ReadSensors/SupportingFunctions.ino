#include <OneWire.h>

/************************************************
 * getTemp()
 *    returns the temperature from one DS18S20 in 
 *    DEG Celsius, src: OneWire library
 ************************************************/
float getTemp(){
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

/************************************************
 * getAvgVal()
 *    returns average value of array, int
 ************************************************/
float getAvgVal(int *arr, int size)
{
  int total = 0;
  for( int i = 0; i < size; i++ )
  {
    total += arr[i];
  }
  return (float)(total/size);
}

/************************************************
 * getAvgValFlt()
 *    returns average value of array, float
 ************************************************/
float getAvgValFlt(float *arr, int size)
{
  float total = 0;
  for( int i = 0; i < size; i++ )
  {
    total += arr[i];
  }
  return (float)(total/size);
}

/************************************************
 * reverse_rotate()
 *    alternates rotation direction, runs 10s
 ************************************************/
void reverse_rotate()
{
  driver_dir = !driver_dir;
  EEPROM.update( EEPROM_DIR, driver_dir );

  /************************************************
   * rotate 10 seconds in opposite direction
   ************************************************/
  for( int i = 0; i < 7142; i++ )
  {
  digitalWrite(DRIVERDIR, driver_dir);
  digitalWrite(DRIVERPUL, HIGH);
  delayMicroseconds(driver_speed);
  digitalWrite(DRIVERPUL, LOW);
  delayMicroseconds(driver_speed);

  }
}

/************************************************
 * getAvgPH()
 *    gets average of PH
 ************************************************/
double getAvgPH(int* arr, int number){
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
