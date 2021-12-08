
/************************************************
 * SupportingFunctions.ino - created by Kylie Outlaw
 * and Maria Abbasi for the HydroHome project.
 ************************************************/
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

/************************************************
 * HomeScreen()
 *    prints home screen on LCD
 ************************************************/
 void   HomeScreen() {
  tft.reset();
  getIdentifierScreen();
  tft.begin(identifier);
  tft.setRotation(3);
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 319, 240, 8, LSEAGREEN1);
  tft.setCursor(80, 10);
  tft.setTextColor(LSEAGREEN1);  
  tft.setTextSize(3);
  tft.println("Main Menu");
  tft.setCursor(3, 40);
  tft.setTextSize(2);
  tft.setTextColor(SEAGREEN);
  tft.println("Please tap an option below");
  tft.fillRoundRect(60, 80, 200, 40, 8, LSEAGREEN1 );
  tft.drawRoundRect(60, 80,200,40,8, GREEN); 
  tft.setCursor(65, 90);
  tft.setTextSize(3);
  tft.setTextColor(BLACK);
  tft.print("Sensor Data");
  tft.fillRoundRect(60, 130, 200, 40, 8, LSEAGREEN1);   
  tft.drawRoundRect(60, 130, 200, 40, 8, GREEN);
  tft.setCursor(65, 145);
  tft.setTextSize(3);
  tft.setTextColor(BLACK); 
  tft.print("Rotate Tray");
   tft.fillRoundRect(60, 180, 200, 40, 8, LSEAGREEN1); 
  tft.drawRoundRect(60, 180, 200, 40, 8, GREEN);
  tft.setCursor(65,195);
  tft.setTextSize(3);
  tft.setTextColor(BLACK);
  tft.print("Test");
}

/************************************************
 * ringMeter()
 *    Draw the meter on the screen, returns x coord of righthand side
 ************************************************/
int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring
  int w = r / 4;    // Width of outer ring is 1/4 of radius
  int angle = 150;  // Half the sweep angle of meter (300 degrees)
  int text_colour = 0; // To hold the text colour
  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v
  byte seg = 5; // Segments are 5 degrees wide = 60 segments for 300 degrees
  byte inc = 5; // Draw segments every 5 degrees, increase to 10 for segmented ring
  // Draw colour blocks every inc degrees
  for (int i = -angle; i < angle; i += inc) {
    // Choose colour from scheme
    int colour = 0;
    switch (scheme) {
      case 0: colour = RED; break; // Fixed colour
      case 1: colour = GREEN; break; // Fixed colour
      case 2: colour = BLUE; break; // Fixed colour
      case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
      case 4: colour = rainbow(map(i, -angle, angle, 63, 127)); break; // Green to red (high temperature etc)
      case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
      default: colour = BLUE; break; // Fixed colour
 
 }
  }
}

/************************************************
 * SensorDataScreen()
 *    Draws Sensor Data Screen
 ************************************************/
void SensorDataScreen() {

  tft.reset();
  getIdentifierScreen();
  tft.begin(identifier);
  tft.setRotation(3);
  tft.fillScreen(BLACK);
  tft.setCursor(70, 10);
  tft.setTextColor(GREENYELLOW);  tft.setTextSize(3);
  tft.println("Sensor Data");
  tft.drawLine(10, 40, 310, 40, CYAN);//
  tft.setCursor(20, 50);
  tft.setTextColor(TURQ); tft.setTextSize(2); tft.println("Temperature");
  tft.setCursor(188, 50);
  tft.setTextColor(GREEN); tft.setTextSize(2); tft.println("EC (ms/cm2)");
  /********************************************** DIVIDERS LINES*********************************/
  tft.drawLine(160, 40, 160, 230, CYAN);//vertical line
  tft.drawLine(10, 135, 310, 135, CYAN); //horizontal line
  tft.setCursor(40, 150);
  tft.setTextSize(2);
  tft.println("pH level");
  if(buffIdx == SCOUNT)
  {
    
    }
  tft.setCursor(50, 185);
  tft.setTextSize(3);
  tft.println("6.9");
  tft.setTextSize(2);
  tft.setCursor(196, 150); 
  tft.setTextColor(TURQ);
  tft.println("TDS");
tft.setCursor(245, 150);
  tft.setTextColor(TURQ);
  tft.println("(ppm)");
  tft.setCursor(215, 185);
  tft.setTextSize(3);
  tft.setTextColor(TURQ); 
  tft.println("690");
  tft.setCursor(220, 170);
  tft.setTextSize(2);
  //tft.println("ppm");
 /****************** Back to Home screen Button******************/
//CLASSBUTTON[index].initButton( &tft, BUTON_X_pos, BUTTON_Y_pos, X_WIDTH, Y_LARGE, BORDER_COLOR, TEXT_COLOR, BUTTON_COLOR, TEXT, FONT_SIZE );
// buttons.initButton( &tft, 50, 220, 70, 30, DARKGREY, BLACK, LSEAGREEN1, "Home", 1 );
 buttons.initButton( &tft, 30, 20, 70, 30, DARKGREY, BLACK, LSEAGREEN1, "Home", 1 );
 buttons.drawButton(true);

}

/************************************************
 * RotateTrayScreen()
 *    Draws rotate tray screen
 ************************************************/
void RotateTrayScreen()
  {
  tft.reset();
  getIdentifierScreen();
  tft.begin(identifier);
  tft.setRotation(3);
  tft.fillScreen(BLACK);
  tft.drawPixel(300,239,ORANGE);
  
  /****************** Back to Home screen Button******************/
  buttons.initButton( &tft, 30, 20, 70, 30, DARKGREY, BLACK, LSEAGREEN1, "Home", 1 );
  buttons.drawButton(true);
  }
  
void meterscreen()
  {
  tft.reset();
  getIdentifierScreen();
  tft.begin(identifier);
  tft.setRotation(3);
  tft.fillScreen(DARKCYAN);
  // Set the the position, gap between meters, and inner radius of the meters
  int xpos = 0, ypos = 5,gap=4, radius = 52;
  ringMeter(reading,0,100,xpos,ypos,radius,"degC",BLUE2RED);
  buttons.initButton( &tft, 30, 20, 70, 30, DARKGREY, BLACK, LSEAGREEN1, "Home", 1 );
  buttons.drawButton(true);
  }

  /************************************************
 * getIdentifierScreen()
 *    unsure
 ************************************************/
void getIdentifierScreen() {

  identifier = tft.readID();
  if (identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if (identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if (identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  } else if (identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if (identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if (identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if (identifier == 0x0101)
  {
    identifier = 0x9341;
    Serial.println(F("Found 0x9341 LCD driver"));
  } else {
    identifier = 0x9341;
  }
}

/************************************************
 * rainbow()
 *    return 16 bit rainbow value
 ************************************************/
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}
