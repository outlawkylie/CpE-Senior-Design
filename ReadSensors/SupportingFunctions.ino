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
  digitalWrite(driverDIR, driver_dir);
  digitalWrite(driverPUL, HIGH);
  delayMicroseconds(driver_speed);
  digitalWrite(driverPUL, LOW);
  delayMicroseconds(driver_speed);

  }
}
