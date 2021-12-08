
/************************************************
 * HydroHome.h - created by Kylie Outlaw
 * and Maria Abbasi for the HydroHome project.
 ************************************************/
#include "TouchScreen.h"

/************************************************
 * Sensor / Motor Controller Definitions
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
 * LCD Color Definitions
 ************************************************/
#define BLACK       0x0000      /*   0,   0,   0 */
#define RED         0xF800
#define NAVY        0x000F      /*   0,   0, 128 */
#define DARKGREEN   0x03E0      /*   0, 128,   0 */
#define DARKCYAN    0x03EF      /*   0, 128, 128 */
#define MAROON      0x7800      /* 128,   0,   0 */
#define PURPLE      0x780F      /* 128,   0, 128 */
#define OLIVE       0x7BE0      /* 128, 128,   0 */
#define LIGHTGREY   0xC618      /* 192, 192, 192 */
#define DARKGREY    0x7BEF      /* 128, 128, 128 */
#define BLUE        0x001F      /*   0,   0, 255 */
#define GREEN       0x07E0      /*   0, 255,   0 */
#define CYAN        0x07FF      /*   0, 255, 255 */
#define PINK         0xF800     /* 255,   0,   0 */
#define MAGENTA     0xF81F      /* 255,   0, 255 */
#define YELLOW      0xFFE0      /* 255, 255,   0 */
#define TURQ        0x2593      /*   0, 159, 162 */
#define KELLYGREEN  0X152C
#define TEALBLUE    0x14BF
#define SEAGREEN    0X3F55
#define WHITE       0xFFFF      /* 255, 255, 255 */
#define ORANGE      0xFD20      /* 255, 165,   0 */
#define GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define PINK        0xF81F
#define LSEAGREEN   0X5777
#define LSEAGREEN1  0xB7F7
#define BOISON      0X0BDB
#define FOREST      0x27EF
#define PUR         0x5272
#define AZUL        0x353E
#define AZUL1       0x86DE
#define BLUE2RED    3



/************************************************
 * LCD Color Definitions
 ************************************************/

#define LCD_CS    A3      // Chip Select goes to Analog 3
#define LCD_CD    A2      // Command/Data goes to Analog 2
#define LCD_WR    A1      // LCD Write goes to Analog 1
#define LCD_RD    A0      // LCD Read goes to Analog 0
#define LCD_RESET A4      // Can alternately just connect to Arduino's reset pin
#define YP        A3      // AKA CS Y+ pin
#define XM        A2      // AKA Command Data X- pin
#define YM        9       // AKA Y- pin
#define XP        8       // X+ pin

 
#define TS_MIN_X    120   //ADC value for X=0
#define TS_MAX_X    920   //ADC value for X=240-1
#define TS_MIN_Y    70    //ADC value for Y=0
#define TS_MAX_Y    900   //ADC value for Y=320-1
#define MINPRESSURE 10
#define MAXPRESSURE 1000

/************************************************
 * Types
 ************************************************/
 enum Page_type
 {
  HOME_PAGE   = 1,
  SENSOR_PAGE = 2,
  ROTATE_PAGE = 3,
  METER_PAGE  = 4
 };

/************************************************
 * Global Variables
 ************************************************/
int         TDSbuff[SCOUNT]; //TDS buffer to hold samples of TDS
int         TEMPbuff[SCOUNT]; //Temp buffer to hold samples of TEMPERATURE
float       ECbuff[SCOUNT]; //EC buffer to hold samples of EC
int         PHbuff[SCOUNT]; //pH buffer to hold samples of PH

static int  buffIdx     = 0; 
int         cpyIdx      = 0;
float       avgVolt     = 0; //value for average voltage
float       tdsValue    = 0; //value for tds
float       temperature = 25; //reference temperature
float       ec_voltage   = 5; //ref ec voltage
static int  timer_count = 0;
static bool flag    = false;
float       ph_voltage  = 0;


float       ecvalue;

int         driver_speed  = 700;
boolean     driver_dir    = EEPROM.read( EEPROM_DIR );

const uint16_t t1_load = 0;
const uint16_t t1_comp = 62500;

OneWire     ds(TEMPSENSORPIN);
DFRobot_EC  EC_sensor;
GravityTDS  TDS_sensor;
DFRobot_PH  PH_sensor;

Elegoo_TFTLCD     tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts =  TouchScreen(XP, YP, XM, YM, 300);// 300 ohms across x plate confirm with dmm


/************************************************
 * LCD Screen Variables
 ************************************************/
Elegoo_GFX_Button     buttons;
uint16_t              identifier; 
int                   tempUnit;//Temperature Unit "1" Celsius "2" Farenheit
int                   currentPage=1;//Current Page indicator  "1" Home screen,  "2" sensor
int                   x, y;
int                   reading = 0; // Value to be displayed
