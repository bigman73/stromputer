#ifndef Stromputer_H
#define Stromputer_H


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   Stromputer - An Enhanced display for a Suzuki DL-650 ("V-Strom")
// []     License: GPL V3
/*
    Stromputer - Enhanced display for Suzuki V-Strom Motorcycles (DL-650, DL-1000, years 2004-2011)
    Copyright (C) 2011 Yuval Naveh

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 .::::::.:::::::::::::::::::..       ...     .        :::::::::::. ...    :::::::::::::::.,:::::: :::::::..   
;;;`    `;;;;;;;;'''';;;;``;;;;   .;;;;;;;.  ;;,.    ;;;`;;;```.;;;;;     ;;;;;;;;;;;'''';;;;'''' ;;;;``;;;;  
'[==/[[[[,    [[      [[[,/[[['  ,[[     \[[,[[[[, ,[[[[,`]]nnn]]'[['     [[[     [[      [[cccc   [[[,/[[['  
  '''    $    $$      $$$$$$c    $$$,     $$$$$$$$$$$"$$$ $$$""   $$      $$$     $$      $$""""   $$$$$$c    
 88b    dP    88,     888b "88bo,"888,_ _,88P888 Y88" 888o888o    88    .d888     88,     888oo,__ 888b "88bo,
  "YMmMY"     MMM     MMMM   "W"   "YMMMMMP" MMM  M'  "MMMYMMMb    "YmmMMMM""     MMM     """"YUMMMMMMM   "W" 
*/

#define VERSION "0.44"

// Macro for defining PROGMEM (Flash) Strings
#define FS( text ) (const char*)F(text)

// ---------------- Control/Operation mode ------------------------
// Comment in/out to enable/disable showing the welcome screen, when the sketch starts
#define SHOW_WELCOME

// Comment in/out to enable/disable printing the gear volts
#define DEBUG_PRINT_GEARVOLTS

// Temperature mode - F or C
#define DEFAULT_TEMPERATURE_MODE 'F'

#define SERIAL_SPEED 38400

// ------------------------- LCD -------------------------------------------
#define LCD_ROWS 2
#define LCD_COLS 16
#define LCD_I2C_ADDRESS 0x50
#define LCD_I2C_NHD_SCROLL_LEFT 0x55
#define LCD_I2C_NHD_SCROLL_RIGHT 0x56

byte lcdBackLight = 4; // LCD back light (brightness)
byte lastLcdBackLight = 4; // Ranges in NHD LCD from 1..8 (Very Dim..Very Bright)
byte lcdContrast = 50; // 0..50 (no contrast .. high contrast)

bool lcdInitialized = false;

// Create the LCD controller instance, for NHD-0216B3Z-FL-GBW
LCDi2cNHD lcd = LCDi2cNHD( LCD_ROWS, LCD_COLS, LCD_I2C_ADDRESS >> 1,0 );

// --------------------------------------------------------------------------

// ----------------------- DS1631 I2C Thermometer ----------------------------------------
#define DS1631_I2C_ADDRESS 0x90 >> 1
#define DS1631_I2C_COMMAND_START_CONVERT 0x51
#define DS1631_I2C_COMMAND_STOP_CONVERT 0x22
#define DS1631_I2C_COMMAND_READ_TEMP 0xAA
#define DS1631_I2C_COMMAND_ACCESS_CONFIG 0xAC
#define DS1631_I2C_CONTROLBYTE_CONT_12BIT 0x0C
// ----------------------------------------------------------------------------------------


// ----------------------- PCF8591 -------------------------------------------------------
// PCF8591 I2C Thermometer 
#define PCF8591_I2C_ADDRESS 0x92 >> 1
#define PCF8591_DAC_SINGLECHANNEL_MODE 0x40
// Note: Not related to I2C protocol. Used internally for efficient reading of channels. Channel 0 is always read.
#define PCF8591_MASK_CHANNEL0 1
#define PCF8591_MASK_CHANNEL1 2
#define PCF8591_MASK_CHANNEL2 4
#define PCF8591_MASK_CHANNEL3 8

// ----------------------------------------------------------------------------------------

// ^^^^^^^^^^^   Gear mapping voltage values ^^^^^^^^^^^
// Note: DL-650 gear shows 0V on 1st gear, when bike engine is off, but switch is on. Only when engaged to N for first time, then the 1st gear reading becomes 1.33.

#define GEAR1_DEFAULT    1.33f

#define GEAR1_FROM_VOLTS 0.00f
#define GEAR1_TO_VOLTS   1.55f
#define GEAR2_FROM_VOLTS 1.55f
#define GEAR2_TO_VOLTS   2.20f
#define GEAR3_FROM_VOLTS 2.20f
#define GEAR3_TO_VOLTS   2.90f
#define GEAR4_FROM_VOLTS 2.90f
#define GEAR4_TO_VOLTS   3.65f
#define GEAR5_FROM_VOLTS 3.65f
#define GEAR5_TO_VOLTS   4.30f
#define GEAR6_FROM_VOLTS 4.30f
#define GEAR6_TO_VOLTS   4.75f
#define GEARN_FROM_VOLTS 4.75f
#define GEARN_TO_VOLTS   5.50f


// Gear Voltage Level Measurements
//  DATE        ARDUINO_TEMP  ADJ_FACTOR   F/W    1       N      2             3       4      5      6
//  3/18/2012    82.5F        N/A          0.32   1.35    4.9    ?1.90-2.00?   2.50    3.04   4.05   4.5
//  3/18/2012    71.8F        N/A          0.32   1.35    4.95   1.7           2.50    3.20   4.05   4.45

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#define GEAR_NEUTRAL 0
#define GEAR_ERROR -1

// Buttons are pulled up, so pressed is zero voltage or logical zero, released is VCC or logical on
#define BUTTON_DOWN 0

// Battery level (4:1 voltage divider) is connected to Analog Pin 0
#define ANALOGPIN_BATT_LEVEL A0
// Gear Position (2:1 voltage divider) is connected to Analog Pin 1
#define ANALOGPIN_GEAR_POSITION A3
// Photocell level (3K-11K:10K voltage divider) is connected to Analog Pin 2
#define ANALOGPIN_PHOTCELL A2

// --- Ethernet Cable #1 ( 'Blue' sheath, right ethernet port )
//
// Note: 6 Digital outputs will be used for Gear LEDs - a total of 6 pins, each pin dedicated to a gear respectively. 
//       All LEDs pins are PWM (to allow LED Dimming)
//
// Gear    |    PIN      |    Wire Color   | LED Color
// --------------------------------------------------
// 1/N     |    D3       |    Green        |  Green
//  2      |    D5       |    White/Green  |  Yellow
//  3      |    D6       |    Orange       |  Yellow
//  4      |    D9       |    White/Orange |  White
//  5      |    D10      |    White/Blue   |  White
//  6      |    D11      |    Blue         |  Blue
//
//  Brown & White/Brown Wires = GND (Below TX/RX, bottom right ports on ProtoScrewShield)
//


LED ledGears[6] = { LED( 3 ), LED( 5 ), LED( 6 ), \
                    LED( 9 ), LED( 10 ), LED( 11 ) };

byte ledBrightnessGreen = 5;  
byte ledBrightnessYellow = 127; 
byte ledBrightnessWhite = 127; 
byte ledBrightnessBlue = 127; 

bool forceLedUpdate = false;
 
// create a LED object at with the default on-board LED
LED onBoardLed = LED();

// --------------------------------------------------------------------------

// --- Ethernet Cable #2 ( 'Gray' sheath, left ethernet )
//
//   Wire Color   |   Description
// ---------------------------------------------------
//   Blue         |   LCD Ground (GND)
//   Green        |   LCD +5 (VDD)
//   White/Blue   |   LCD I2C Clock (SCL)
//   White/Green  |   LCD I2C Data (SDA)
//   Orange       |   Photo cell leg 1
//   White/Orange |   Photo cell leg 2
//   Brown        |   Not Used (Future: Rocker Switch)
//   White/Brown  |   Not Used (Future: Push Button)
//

// --------------------------------------------------------------------------

#define IsBetween( x, a, b ) ( x >= a && x <= b ? 1 : 0 )

#define EEPROM_CONFIG_MEMOFFSET 16
#define EEPROM_STRUCT_VER 10000

struct config_t
{
    int isValidConfig; // Should be >= 10000 || <= 11000 when valid (1000 structure versions allowed)
    char temperatureMode; // F or C
} configuration;

// Variables

// Variables used for PCF8591 IC IO
byte dac_value=0;     // output 0..255 -> 0-5V (or VSS..VREF)
byte all_adc_values[4];   // Input 0..255 -> 0-5V  (or VSS..VREF)

float ds18b20Temperature = 0;  // Farenheit
float onBoardTemperature = 0;  // Farenheit
float lastTemperature = -99; // Force initial update
float lastOnBoardTemperature = -99;
bool temperatureReadError = false;
bool onBoardTemperatureReadError = false;

short gear = 0;               // 0 = Neutral, or 1-6
long transientGearStartMillis = 0;
short transientGear = GEAR_ERROR;
short lastGearLCD = -2;          // Force initial update
short lastGearLED = -2;       // Force initial update
float gearVolts[] = { 5, 4.5, 4.8, 4.3 };
bool gearReadError = false;
float gearPositionVolts = 0;

float battLevel;             // Volts
float lastBattLevel = -0.1;  // Force initial update
byte battReadError = 0;

short photoCellLevel;

short LoopSleepTime = 5; // msec

bool autoStat = false;

long lastForceLCDRefreshMillis = 0;
bool isForceRefreshBatt = true;
bool isForceRefreshGear = true;
bool isForceRefreshTemp = true;

// Intervals for Timed Actions - in msec
#define PROCESS_BATT_LEVEL_TIMED_INTERVAL 1000
#define PROCESS_TEMP_TIMED_INTERVAL 2000
#define PROCESS_PHOTO_CELL_TIMED_INTERVAL 3000
#define LCD_DISPLAY_LOOP_TIMED_INTERVAL 250
#define LCD_FORCEREFRESH_INTERVAL 5000
#define SERIALINPUT_TIMED_INTERVAL 100

 // 1000 microseconds = 1 msec = 1000hz
#define TIMER1_RESOLUTION 1000

// RB1: 39Kohm, Real measured value: 38.2KOhm
// RB2: 120Kohm, Real measured value: 118.5KOhm
// ( 118500 + 38200 ) / 38200 = 4.102f
#define BATT_VOLT_DIVIDER 4.102f
// RG1: 497KOhm
// RG2: 497KOhm
// ( 497K + 497K ) / 994K = 2.0f
#define GEAR_VOLT_DIVIDER 2.0f

#define BATT_WINDOW_SIZE 4

#define GEAR_WINDOW_SIZE 16    

RunningAverage battRunAvg(BATT_WINDOW_SIZE); 

RunningAverage gearLevelRunAvg(GEAR_WINDOW_SIZE); 

RunningAverage vrefRunAvg(GEAR_WINDOW_SIZE); 

// msec
#define MIN_TRANSIENTGEAR_INTERVAL 200

#define TEMPERATURE_ERROR_DIFF 30
#define TEMPERATURE_MIN_VALID -55    

#define PHOTOCELL_LEVEL1 50
#define PHOTOCELL_LEVEL2 250
#define PHOTOCELL_LEVEL3 500
#define PHOTOCELL_LEVEL4 700
#define PHOTOCELL_LEVEL5 800
#define PHOTOCELL_LEVEL6 900
#define PHOTOCELL_LEVEL7 1000    

// --------------------------------------------------------------------------

// Utility macro for comparing floats
#define EPSILON 0.01
#define CompareFloats( a, b ) abs( a - b ) < EPSILON ? 1 : 0

#define DIRECTION_LEFT 'L'
#define DIRECTION_RIGHT 'R'

// UI Labels
#define GEAR_LABEL         "Gear "
#define TEMPERATURE_LABEL  "Temp  "
#define BATTERY_LABEL      "Batt  "
#define Welcome1_Line1     "Stromputer-DL650"
#define Welcome1_Line2     "F/W Ver="

#define CMD_ALIVE      "ALIVE"
#define CMD_STAT       "STAT"
#define CMD_AUTOSTAT   "AUTOSTAT"
#define CMD_CONFIG     "CONFIG"
#define CMD_TEST       "TEST"
#define CMD_SETCFG     "SETCFG"
#define CMD_TESTLEDS   "TESTLEDS"

#define CMD_ARG_TEMP   "TEMP"

#define MSG_STROMPUTER_READY     F( ">> Stromputer ON. Ready to Rock! <<" )
#define MSG_FIRMWARE             F( "------- Stromputer, Firmware version: " )
#define MSG_SHOW_ALL_GEAR_LEDS   F( ">> Show all Gear LEDs" )
#define MSG_TEST_EACH_GEAR_LED   F( ">> Test each Gear LED" )
#define MSG_LCD_INIT_BEGIN       F( ">> LCD Initializing.." )
#define MSG_LCD_INIT_END         F( ">> LCD Initialized" )
#define MSG_DS1631_INIT_BEGIN    F( ">> DS1631 Initializing.." )
#define MSG_DS1631_INIT_END      F( ">> DS1631 Initialized" )
#define MSG_WELCOME_BEGIN        F( ">> Show Welcome - BEGIN.." )
#define MSG_WELCOME_END          F( ">> Show Welcome - END" )
#define MSG_SYNTAX_1             F( "SYNTAX: CMD [ARG1] [ARG2];" )
#define MSG_SYNTAX_2             F( "   CMD = {ALIVE | TEST | STAT | SETCFG}" )
#define MSG_ISALIVE              F( "Yes, I'm here" )

#define ERR_LCD_INIT_FAILED      F( ">> ERROR: LCD failed to initialize" )
#define ERR_DS1631_INIT_FAILED   F( ">> ERROR: DS1631 failed to initialize" )

#endif
