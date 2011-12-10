#ifndef Stromputer_H
#define Stromputer_H

// ---------------- Control/Operation mode ------------------------
// Comment in/out to enable/disable serial debugging info (tracing)
// #define SERIAL_DEBUG

// Comment in/out to enable/disable showing the welcome screen, when the sketch starts
#define SHOW_WELCOME

// Comment in/out to enable/disable printing the gear volts
#define DEBUG_PRINT_GEARVOLTS

// Comment in/out to enable/disable PCF8591 ADC Read/Arduino Analog Read
/// #define PCF8591_READ

// Comment in/out to enable/disable PCF8591 DAC Gear Emulation
// #define PCF8591_GEAR_EMULATOR

// Comment in/out to enable/disable manual gear emulation (using two tactile buttons)
#define MANUAL_GEAR_EMULATION

// Temperature mode - F or C
#define TEMPERATURE_MODE 'F'

#define SERIAL_SPEED 9600

// ------------------------- LCD -------------------------------------------
#define LCD_ROWS 2
#define LCD_COLS 16
#define LCD_I2C_ADDRESS 0x50
#define LCD_I2C_NHD_SCROLL_LEFT 0x55
#define LCD_I2C_NHD_SCROLL_RIGHT 0x56

int lcdBackLight = 2; // Default initial LCD back light

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
#define GEAR1_FROM_VOLTS 1.33f - 0.30f
#define GEAR1_TO_VOLTS   1.33f + 0.17f
#define GEAR2_FROM_VOLTS 1.77f - 0.17f
#define GEAR2_TO_VOLTS   1.77f + 0.30f
#define GEAR3_FROM_VOLTS 2.50f - 0.30f
#define GEAR3_TO_VOLTS   2.50f + 0.30f
#define GEAR4_FROM_VOLTS 3.23f - 0.30f
#define GEAR4_TO_VOLTS   3.23f + 0.35f
#define GEAR5_FROM_VOLTS 4.10f - 0.35f
#define GEAR5_TO_VOLTS   4.10f + 0.20f
#define GEAR6_FROM_VOLTS 4.55f - 0.20f
#define GEAR6_TO_VOLTS   4.55f + 0.20f
#define GEARN_FROM_VOLTS 5.00f - 0.20f
#define GEARN_TO_VOLTS   5.00f + 0.50f
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#define GEAR_NEUTRAL 0
#define GEAR_ERROR -1

#define MANUAL_GEAR_DOWN_PIN 11
#define MANUAL_GEAR_UP_PIN 12

// Buttons are pulled up, so pressed is zero voltage or logical zero, released is VCC or logical on
#define BUTTON_DOWN 0

// Battery level (4:1 voltage divider) is connected to Analog Pin 0
#define ANALOGPIN_BATT_LEVEL 0
// Gear Position (2:1 voltage divider) is connected to Analog Pin 1
#define ANALOGPIN_GEAR_POSITION 1

// Note: 6 Digital outputs will be used for Gear LEDs - from GEAR1_LED_PIN .. GEAR1_LED_PIN + 5 (inclusive), for a total of 6 pins, each pin dedicated to a gear respectively.
#define GEAR_BASE_LED_PIN 2

LED ledGears[6] = { LED( GEAR_BASE_LED_PIN ), LED( GEAR_BASE_LED_PIN + 1 ), LED( GEAR_BASE_LED_PIN + 2 ), \
                    LED( GEAR_BASE_LED_PIN + 3 ), LED( GEAR_BASE_LED_PIN + 4 ), LED( GEAR_BASE_LED_PIN + 5 ) };

// create a LED object at with the default on-board LED
LED onBoardLed = LED();


#define IsBetween( x, a, b ) ( x >= a && x <= b ? 1 : 0 )

// --------------------------------------------------------------------------
// Variables

// Variables used for PCF8591 IC IO
byte dac_value=0;     // output 0..255 -> 0-5V (or VSS..VREF)
byte all_adc_values[4];   // Input 0..255 -> 0-5V  (or VSS..VREF)

float temperature;  // Farenheit
float lastTemperature = -99; // Force initial update
byte temperatureReadError = 0;

int gear = 0;               // 0 = Neutral, or 1-6
int lastGearLCD = -2;          // Force initial update
int lastGearLED = -2;       // Force initial update
float gearVolts[] = { 5, 4.5, 4.8, 4.3 };
byte gearReadError = 0;
float gearPositionVolts = 0;
int gearButtonTriggered = true; // Used to ensure that a tactile button has to be released up, before the system handles the next button down event ("Click")
 
float battLevel;             // Volts
float lastBattLevel = -0.1;  // Force initial update
byte battReadError = 0;

int LoopSleepTime = 5; // msec

long lastForceLCDRefreshMillis = 0;

// --------------------------------------------------------------------------

// Utility for comparing floats
#define EPSILON 0.01
#define CompareFloats( a, b ) abs( a - b ) < EPSILON ? 1 : 0

#define DIRECTION_LEFT 'L'
#define DIRECTION_RIGHT 'R'

// UI Labels
#define GEAR_LABEL "Gear " 
#define TEMPERATURE_LABEL "Temp "
#define BATTERY_LABEL "Batt "
#define Welcome1_Line1 "Stromputer-DL650"
#define Welcome1_Line2 "F/W Ver="

#endif
