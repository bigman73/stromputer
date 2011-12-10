// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   V-Strom Mk1B - An extra display for a Suzuki DL-650 ("V-Strom") that adds the following functionality:
// []	  1. Battery Level display in Volts - e.g. 14.5V
// []	  2. Gear Position Indicator on LCD - e.g. 1, 2, 3, 4, 5, 6, N
// []	  3. Ambient Temperature in Farenheight or Celsius - e.g. 62.5F
// []	  4. [Future] LED display of gear position (one led for each gear 1-6, in different colors, N will be blinking on 1)
// []	  5. [Future] Accurate display of the fuel level (in percentage)
// []     6. [Future] Show Fuel consumption - MPG or KM/L, TBD: need to tap into motorcycle's speed sensor (PWM)
// []	  7. [Future] Fix the OEM V-Strom Fuel Gauge to become linear
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
// []
// []
// []   Versions:
// []     0.07 - Prototype, ADC worksonly with PCF8591
// []     0.08 - Prototype, ADC works with Arudino Analog or PCF8591
// []     0.09 - Prototype, add timed actions - Battery and Temperature should not be polled every loop, just once a few seconds
// []     0.10 - 12/3/2011, Stable Prototype, first version in Google Code/SVN
// []     0.11 - 12/4/2011, Added gear led logic in software, refactored code to use ISR for main board blink and gear neutral blinking led, changed welcome screen
// []     0.12 - 12/5/2011, Added direct gear emulation (tactile button)
// []     0.13 - 12/9/2011, Added gear led boot test. Last version compatible with Arduino 0023 before moving to Arduino 1.0
// []
// []
// []     **** Compatible with ARDUINO: 0023 ****
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

// References/Credits:
// DS1631: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1221926830
// ISR: http://letsmakerobots.com/node/28278
// Light Sensors: http://www.ladyada.net/learn/sensors/cds.html


#define VERSION "0.14"

#include <Wire.h>
#include <inttypes.h>

// Include 3rd Party library - LCD I2C NHD
// http://www.arduino.cc/playground/Code/LCDi2c
#include <LCDi2cNHD.h>                    

// Include 3rd Party library - Timed Actions
// http://www.arduino.cc/playground/Code/TimedAction
#include <TimedAction.h>

// Include 3rd Party library - LED (Modified by Yuval Naveh)
#include <LED.h>


#define SERIAL_SPEED 9600

// ---------------- Timed Actions (Scheduled Events) -------------
// Timed action for processing battery level
TimedAction battLevelTimedAction = TimedAction( -999999, 2000, ProcessBatteryLevel ); // Prev = -999999 == > Force  first time update without waiting
// Timed action for processing temperature
TimedAction temperatureTimedAction = TimedAction( -999999, 5000, ProcessTemperature );
// Timed action for Main Sampling/Display Loop
TimedAction mainLoopTimedAction = TimedAction( -999999, 50, MainLoopTimedAction );

// ---------------- Control/Operation mode ------------------------
// Comment in/out to enable/disable serial debugging info (tracing)
// #define SERIAL_DEBUG

// Comment in/out to enable/disable showing the welcome screen, when the sketch starts
// #define SHOW_WELCOME

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

// ------------------------- LCD -------------------------------------------
#define LCD_ROWS 2
#define LCD_COLS 16
#define LCD_I2C_ADDRESS 0x50
#define LCD_I2C_NHD_SCROLL_LEFT 0x55
#define LCD_I2C_NHD_SCROLL_RIGHT 0x56

// Create the LCD controller instance, for NHD-0216B3Z-FL-GBW
LCDi2cNHD lcd = LCDi2cNHD( LCD_ROWS, LCD_COLS, LCD_I2C_ADDRESS >> 1,0 );

int lcdBackLight = 2; // Default initial LCD back light
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
#define GEAR1_FROM_VOLTS 1.33f - 0.3f
#define GEAR1_TO_VOLTS   1.33f + 0.2f
#define GEAR2_FROM_VOLTS 1.77f - 0.2f
#define GEAR2_TO_VOLTS   1.77f + 0.35f
#define GEAR3_FROM_VOLTS 2.50f - 0.40f
#define GEAR3_TO_VOLTS   2.50f + 0.4f
#define GEAR4_FROM_VOLTS 3.23f - 0.3f
#define GEAR4_TO_VOLTS   3.23f + 0.4f
#define GEAR5_FROM_VOLTS 4.10f - 0.4f
#define GEAR5_TO_VOLTS   4.10f + 0.25f
#define GEAR6_FROM_VOLTS 4.55f - 0.2f
#define GEAR6_TO_VOLTS   4.55f + 0.25f
#define GEARN_FROM_VOLTS 5.00f - 0.24f
#define GEARN_TO_VOLTS   5.00f + 0.5f
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
#define Welcome1_Line2 "\"White Pearl\""
#define Welcome2_Line1 "F/W Ver="
#define Welcome2_Line2 "Stromtrooper.com"

/// --------------------------------------------------------------------------
/// Arduino one-time setup routine - i.e. Program entry point like main()
/// --------------------------------------------------------------------------
void setup() 
{ 
    // Setup Serial connection
    Serial.begin( SERIAL_SPEED );
    Serial.print("------- V-Strom Mk1B, VERSION: "); Serial.println(VERSION);

    onBoardLed.on();    
     
    lcd.init();                          // Init the display, clears the display

    // Set initial backlight
    // TODO: In future, control automatically and continously with light sensor
    lcd.setBacklight( lcdBackLight );

    #ifdef SHOW_WELCOME
    PrintWelcomeScreen(Welcome1_Line1, Welcome1_Line2, 600, 25, DIRECTION_RIGHT );
    char line1[16] = Welcome2_Line1;
    strcat( line1, VERSION );
    PrintWelcomeScreen(line1, Welcome2_Line2, 600, 25, DIRECTION_LEFT );
    #endif
    
    setupDS1631();

    #ifdef PCF8591_READ
        Serial.print( "===>     PCF8591_READ" );
    #endif  

    testGearLEDs();
    	
    // sets the digital pin of gear tacktile button as input
    #ifdef MANUAL_GEAR_EMULATION
    pinMode(MANUAL_GEAR_DOWN_PIN, INPUT);      
    pinMode(MANUAL_GEAR_UP_PIN, INPUT);      
    #endif
    
    setupTimerInterrupt();  
}


/// --------------------------------------------------------------------------
/// Arduino loop routine - gets called all the time in an infinite loop
/// --------------------------------------------------------------------------
void loop()
{
    mainLoopTimedAction.check();
    
    delay(LoopSleepTime);
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for Main Loop 
/// Note: Used instead of the stock main loop, to ensure better more accurate
///         timed scheduling, due to use of the TimedAction mechanism. In stock main
///          loop a typical sleep is needed, which is avoided here.
/// --------------------------------------------------------------------------
void MainLoopTimedAction()
{
    // TODO: Force full screen refresh every 15 seconds (some unknown bug with NHD LCD causes it to clear the screen from time to time)
    if ( millis() - lastForceLCDRefreshMillis > 15000 )
    {
        #ifdef SERIAL_DEBUG 
        Serial.println( "** FORCE REFRESH LCD DISPLAY **" );
        #endif
        lastTemperature = -99;    
        lastBattLevel  = -1;    
        lastGearLCD = -2;    
        lastForceLCDRefreshMillis = millis();
    }
    
    // Timed Actions ("Events")
    battLevelTimedAction.check();       
    temperatureTimedAction.check();

    #ifdef PCF8591_READ
         ReadBatteryAndGearPositionPCF8591();
    #else
        #ifdef PCF8591_GEAR_EMULATOR
        // NOTE: DEBUG MODE ONLY - AUTO INCREMENTS EMULATOR OUTPUT VOLTAGE (Used for gear emulation)
        dac_value += 4;
        ControlPCF8591_I2C( dac_value, all_adc_values, PCF8591_MASK_CHANNEL0 ); // Output DAC, ADC from channel 1
        #endif
    #endif
    
    PrintGearPosition();
    
    
}

int timerDivider = 0;

/// --------------------------------------------------------------------------
/// Timer compare Interrupt Service Routine (ISR)
/// Note: Setup to run on 16Hz (62.5msec)
/// --------------------------------------------------------------------------
ISR( TIMER1_COMPA_vect )          
{
    // Handle main board blinking at 1Hz for each toggle
    if ( timerDivider % 16 == 1 ) 
    {
        /// Toggle the Main Board LED
        onBoardLed.toggle();
    }

    // Handle gear position read
    HandleGearPositionRead();

    // Update Gear Position LEDs (note: updateGearLEDs() is optimized to only actually refresh on gear change)
    updateGearLEDs();
    
    // Handle neutral gear blinking, at 2Hz for each toggle
    if ( timerDivider % 8 == 1  && 
         gear == GEAR_NEUTRAL )
    {
        // Toggle 1st led Gear, for signaling rider the gear is N
        ledGears[ 0 ].toggle();
    }    

    timerDivider++;
}


void HandleGearPositionRead()
{
    #ifndef PCF8591_READ    
        #ifdef MANUAL_GEAR_EMULATION
            // read the input pin for Gear Up button
            int manualButtonValue = digitalRead(MANUAL_GEAR_UP_PIN);
            if ( manualButtonValue == BUTTON_DOWN )
            {
                if ( gearButtonTriggered )
                {
                    gear++;
                    gearButtonTriggered = false;
                
                    if ( gear > 6 )
                        gear = GEAR_NEUTRAL;
                }
            }
            else
            {
                // read the input pin for Gear Down button
                manualButtonValue = digitalRead(MANUAL_GEAR_DOWN_PIN);
                if ( manualButtonValue == BUTTON_DOWN )
                {
                    if ( gearButtonTriggered )
                    {
                        gear--;
                        gearButtonTriggered = false;
                
                        if ( gear < 0)
                            gear = 6;
                    }
                }
                // Both buttons are UP, therefore are considered triggered
                else
                {
                    gearButtonTriggered = true;
                }
            }
        #else    
            ReadGearPositionAnalog();          
        #endif
    #endif
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for battLevelTimedAction - 
///     Processes the battery level
/// --------------------------------------------------------------------------
void ProcessBatteryLevel()
{   
    #ifndef PCF8591_READ
    ReadBatteryLevelAnalog();
    #endif
    
    PrintBatteryLevel();
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for temperatureTimedAction - 
//     Processes the ambient temperature
/// --------------------------------------------------------------------------
void ProcessTemperature()
{
    ReadTemperature();
    PrintTemperature();   
}





/// ----------------------------------------------------------------------------------------------------
/// Prints the welcome screen using the given two lines arguments
/// ----------------------------------------------------------------------------------------------------
void PrintWelcomeScreen( char *line1, char *line2, int showDelay, int scrollDelay, char scrollDirection )
{
    // Print line 1
    lcd.print( line1 );       
    // Print line 2
    lcd.setCursor( 1,0 );
    lcd.print(line2 );       

    delay( showDelay );

    // Scroll Left or Right, based on the scrollDirection argument
    int i2cScrollCommand = ( scrollDirection == DIRECTION_LEFT ) ? LCD_I2C_NHD_SCROLL_LEFT : LCD_I2C_NHD_SCROLL_RIGHT;
    
    // Shift screen right
    for ( int i = 0; i < LCD_COLS; i++ )
    {    
        lcd.command( i2cScrollCommand );
        delay( scrollDelay );
    }
    
    lcd.clear();
}


/// ----------------------------------------------------------------------------------------------------
/// Reads the battery and gear position from the PCF8591 DAC/ADC I2C IC
//  Note: Was used for debug. Not in use.
/// ----------------------------------------------------------------------------------------------------
void ReadBatteryAndGearPositionPCF8591()
{
    battReadError = true; // Assume read had errors, by default
    gearReadError = true; // Assume read had errors, by default
    
    if ( !ControlPCF8591_I2C( dac_value, all_adc_values, PCF8591_MASK_CHANNEL0 | PCF8591_MASK_CHANNEL1 | PCF8591_MASK_CHANNEL2 | PCF8591_MASK_CHANNEL3 ) )
    {
        return;
    }
     
    battLevel = 4.0f * 5.0f * ( float ) all_adc_values[ 3 ] / 255;  // 4.0f = Volt Divider of 4:1 (20V -> 5V, using a 120K/39Kohm volt divider), 5.0f = VREF
    gearPositionVolts  = 2.0f * 5.0f * ( float ) all_adc_values[ 0 ] / 255; // 2.0f = Volt Divider of 2:1 (10V -> 5V, using a 47K/47K volt divider), 5.0f = VREF

    DetermineCurrentGear();
             
    battReadError = false; // Clear read error - we made it here
    gearReadError = false; // Clear read error - we made it here
}



/// ----------------------------------------------------------------------------------------------------
/// Reads the current battery level from the voltage divider circuit (4:1, 20V -> 5V), using Arduino ADC
/// ----------------------------------------------------------------------------------------------------
void ReadBatteryLevelAnalog()
{
    battReadError = true; // Assume read had errors, by default

    int value = analogRead( ANALOGPIN_BATT_LEVEL );    // read the input pin for Battery Level
    battLevel = 4.0f * 5.0f * ( value / 1024.0f );
        
    battReadError = false; // Clear read error - we made it here
    
}

/// ----------------------------------------------------------------------------------------------------
/// Print the current battery level (only if there has been a changed)
/// ----------------------------------------------------------------------------------------------------
void PrintBatteryLevel()
{
    // Optimization: Don't print the battery level if nothing has changed since the last printing
    if ( !battReadError && ( CompareFloats( lastBattLevel, battLevel )  ) )
    {
        return;
    }
    
    // Keep the current battery level, to optimize display time
    lastBattLevel = battLevel;

    // Print Battery Label
    lcd.setCursor( 0, 0 );
    lcd.print( BATTERY_LABEL );     

    // Print Battery Value
    String battLevelValue = "X";
    
    if ( battReadError )
    {
        battLevelValue = "ERR  ";
    }
    else
    {
        // Format battery float value
        char formattedBattLevel[6];
        formatFloat( formattedBattLevel, battLevel, 1 );
        String battFormattedString = formattedBattLevel;

        // Pad to right
        if ( battLevel >= 0 && battLevel < 10 )
        {
           battLevelValue = " ";
           battLevelValue += battFormattedString;
        }
        else
        {
           battLevelValue = battFormattedString;
        };
    
        // Add volts unit
        battLevelValue += "V";
    }
  
    #ifdef SERIAL_DEBUG
    Serial.print( "BattLevel = " ); Serial.print( battLevel );
    Serial.print( "\t battLevelValue = " ); Serial.println( battLevelValue );
    #endif
    
    lcd.setCursor(1,0);   
    lcd.print( battLevelValue );
}


// ----------------------------------------------------------------------------------------------------
/// Reads the current gear position level from the voltage divider circuit (2:1, 10V -> 5V), using Arduino ADC
/// ----------------------------------------------------------------------------------------------------
void ReadGearPositionAnalog()
{
    gearReadError = true; // Assume read had errors, by default
   
    int value = analogRead( ANALOGPIN_GEAR_POSITION );

    #ifdef SERIAL_DEBUG
    Serial.print( "*** gear value: " ); Serial.println( value ); 
    #endif
    
    gearPositionVolts = 2.0f * 5.0f * ( value / 1024.0f );    // read the input pin for Gear Position
    DetermineCurrentGear();
    
    gearReadError = false; // Clear read error - we made it here
}

// ----------------------------------------------------------------------------------------------------
/// Determins the current gear from the gear position volts
/// ----------------------------------------------------------------------------------------------------
void DetermineCurrentGear()
{
     if ( IsBetween( gearPositionVolts, GEAR1_FROM_VOLTS, GEAR1_TO_VOLTS ) )
         gear = 1;
     else if ( IsBetween( gearPositionVolts, GEAR2_FROM_VOLTS, GEAR2_TO_VOLTS ) )
         gear = 2;
     else if ( IsBetween( gearPositionVolts, GEAR3_FROM_VOLTS, GEAR3_TO_VOLTS ) )
         gear = 3;
     else if ( IsBetween( gearPositionVolts, GEAR4_FROM_VOLTS, GEAR4_TO_VOLTS ) )
         gear = 4;
     else if ( IsBetween( gearPositionVolts, GEAR5_FROM_VOLTS, GEAR5_TO_VOLTS ) )
         gear = 5;
     else if ( IsBetween( gearPositionVolts, GEAR6_FROM_VOLTS, GEAR6_TO_VOLTS ) )
         gear = 6;
     else if ( IsBetween( gearPositionVolts, GEARN_FROM_VOLTS, GEARN_TO_VOLTS ) )
         gear = GEAR_NEUTRAL;  // Neutral
     else
         gear = GEAR_ERROR; // Default: Error
         
    // gear = 2;
}

/// ----------------------------------------------------------------------------------------------------
/// Print the current gear position (only if there has been a change)
/// ----------------------------------------------------------------------------------------------------
void PrintGearPosition()
{
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    #ifdef DEBUG_PRINT_GEARVOLTS
    lcd.setCursor( 0, 6 );
    lcd.print( gearPositionVolts );
    #endif
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    // Optimization: Don't print the gear position if nothing has changed since the last printing
    if ( !gearReadError && ( lastGearLCD == gear ) )
    {
        return;
    }

    #ifdef SERIAL_DEBUG
    Serial.print( ", lastGearLCD = " ); Serial.print( lastGearLCD );
    Serial.print( ", gear = " ); Serial.println( gear );
    #endif 
    
    // Keep the current gear position, to optimize LCD display time
    lastGearLCD = gear;
        
    // Print Gear Position Label
    lcd.setCursor( 0, 6 );
    // TODO: RESTORE ONCE GEAR IS STABLE
    //lcd.print( GEAR_LABEL );      
    
    // Print Gear Position Value
    String gearValue;
    if ( gearReadError )
    {
        gearValue = "ERR";
    }
    else
    {
        gearValue = "=";
        if ( gear == 0 )
            gearValue.concat( "N" );
        else if ( gear == -1 )
            gearValue.concat( "E" );
        else
            gearValue.concat( gear );
    
        gearValue.concat( "=" );
    }

    lcd.setCursor(1,6);
    lcd.print( gearValue );   
      
    #ifdef SERIAL_DEBUG
    Serial.print( "gearPositionVolts = " ); Serial.print( gearPositionVolts );
    Serial.print( "\t gearValue = " ); Serial.println( gearValue );
    #endif
}

/// ----------------------------------------------------------------------------------------------------
/// Updates the gear LEDs with current gear position (only if there has been a change)
/// Note: Netural is blinking and is therefore being handled with a timer ISR, by NeutralGearLedBlink() 
/// ----------------------------------------------------------------------------------------------------
void updateGearLEDs()
{
    // Optimization: Don't print the gear position if nothing has changed since the last printing
    if ( !gearReadError && ( lastGearLED == gear ) )
    {
        return;
    }
    
    lastGearLED = gear;

    // TODO: Different algorithms are possible - e.g. each gear has its own led, or incremental, or incremental with top gear (6th) lighting alone
    // Currently option 3 is implemented. 
    
    // Update each gear led
    ledGears[1-1].setState( gear < 6 );
    ledGears[2-1].setState( gear >= 2 && gear < 6 );
    ledGears[3-1].setState( gear >= 3 && gear < 6 );
    ledGears[4-1].setState( gear >= 4 && gear < 6 );
    ledGears[5-1].setState( gear >= 5 && gear < 6 );
    ledGears[6-1].setState( gear == 6 );
}

/// ----------------------------------------------------------------------------------------------------
/// Reads the current temperature from the I2C DS1631 Thermometer 
/// ----------------------------------------------------------------------------------------------------
void ReadTemperature()
{   
    temperatureReadError = true; // Assume read had errors, by default
    
    // READ T° from DS1631
    Wire.beginTransmission( DS1631_I2C_ADDRESS );
    Wire.send((int)( DS1631_I2C_COMMAND_READ_TEMP ));
    Wire.endTransmission();
    Wire.requestFrom( DS1631_I2C_ADDRESS, 2 ); // READ 2 bytes
    int bytes;
    bytes = Wire.available(); // 1st byte
    if (bytes != 2)
      return;
    int _TH = Wire.receive(); // receive a byte (TH - High Register)
    
    bytes = Wire.available(); // 2nd byte
    if (bytes != 1)
      return;
    int _TL = Wire.receive(); // receive a byte (TL - Low Register)
    
    // T° processing
    if ( _TH >= 0x80 ) //if sign bit is set, then temp is negative
        _TH = _TH - 256; 
    temperature = _TH + _TL / 256.0;
      
    #if TEMPERATURE_MODE == 'F'
    
    // Convert celsius to fahrenheight
    temperature = temperature * 9.0 / 5.0 + 32;
    
    #endif
    
    temperatureReadError = false; // Clear read error - we made it here
}

/// ----------------------------------------------------------------------------------------------------
/// Print the current temperature (only if it has been a changed)
/// ----------------------------------------------------------------------------------------------------
void PrintTemperature()
{
    // Optimization: Don't print the temperature if nothing has changed since the last printing
    if ( !temperatureReadError && ( lastTemperature == temperature ) )
    {
        return;
    }

    // Keep the current gear position, to optimize display time
    lastTemperature = temperature;

    // Print temperature label
    lcd.setCursor( 0, 11 );
    lcd.print( TEMPERATURE_LABEL );     

    // Print temperature value
    String temperatureValue;

    if ( temperatureReadError )
    {
        temperatureValue = " ERR  ";
    }
    else
    {
        // Pad to right if temperature is two digits (left of dot)
        if ( ( temperature > 10 && temperature < 100 ) || ( temperature < 0 && temperature > -10 ) )
            temperatureValue += " " ;
        // Pad to right if temperature is one digits (left of dot)        
        else if ( temperature > 0 && temperature < 10 )
            temperatureValue += "  " ;
    
        // format temperature into a fixed .1 format (e.g. 62.5 or 114.4 [too hot to ride! :) ] or -10.7 [too cold to ride! :) ])
        char formattedTemperature[6];
        formatFloat( formattedTemperature, temperature, 1 );
        temperatureValue += formattedTemperature;
        
        // Add temperature mode unit
        #if TEMPERATURE_MODE == 'F'
        temperatureValue += "F";
        #else
        temperatureValue += "C";
        #endif
    }
    
    lcd.setCursor( 1, 10 );
    lcd.print( temperatureValue );
}



/// ----------------------------------------------------------------------------------------------------
/// Formats a float to a string. A workaround to sprintf not working on arduino (produces "?")
/// Source: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1164927646
/// ----------------------------------------------------------------------------------------------------
char *formatFloat(char *buffer, double floatValue, int precision)
{
  long precisionLongs[] = {0,10,100,1000,10000};
  
  char *ret = buffer;
  long heiltal = (long) floatValue;
  itoa(heiltal, buffer, 10);
  while (*buffer != '\0') buffer++;
  *buffer++ = '.';
  long decimals = abs((long)((floatValue - heiltal) * precisionLongs[precision]));
  itoa(decimals, buffer, 10);
  return ret;
}
 
 
 
/// ------------------------------------------------------------
/// Controls the PCF8591 IC through I2C protocol
/// Arguments:
///    dac_value - A byte value (0..255) that would be converted to analog voltage level (VSS..VREF, typically 0..5V)
///    adc_values - An output array of bytes where the results of ADC conversion will be stored, per channel
///    adcChannelMask - A mask for the ADC Input channel mask
/// ------------------------------------------------------------
int ControlPCF8591_I2C(byte dac_value, byte adc_values[], byte adcChannelMask )
{
    int dacValueSent = 0;
    
    // Serial.print( "inputChannelMask= " );     Serial.println( (int) inputChannelMask );
    // Iterate on all input channels, and get their ADC Value if the input mask includes them
    for ( int inputChannel = 0; inputChannel <=3; inputChannel++ )
    {
            if ( ( adcChannelMask & ( 1 << inputChannel ) ) != 0 )
            {
                  Wire.beginTransmission(PCF8591_I2C_ADDRESS);
                  // Select ADC channel (One of CH0..CH3)
                  Wire.send( PCF8591_DAC_SINGLECHANNEL_MODE | (byte) inputChannel );
                  
                  #ifdef SERIAL_DEBUG
                  Serial.print( "InputChannel = " ); Serial.println( inputChannel );
                  #endif
    
                  // Only send the dac value once (AOUT)
                  if ( dacValueSent == 0 )
                  {
                      Wire.send(dac_value);
                      dacValueSent = 1;
                  }
                  Wire.endTransmission();
                  
                  delay(1);
                  
                  Wire.requestFrom( (int) PCF8591_I2C_ADDRESS, 2 );
                  if (Wire.available()) 
                  {
                    Wire.receive();  // last value -> just read but ignore
                  }
                  else
                      return 0;
                  
                  if (Wire.available())
                  {
                    adc_values[inputChannel] = Wire.receive();
                  }
                  else
                      return 0;
                  

                  delay(1);
            }
    } // for
    
    return 1;
}    

/// ------------------------------------------------------------
/// Tests the gear LEDs
/// ------------------------------------------------------------
void testGearLEDs()
{
    for ( int i=0; i < 3; i++ )
    {
        // Light left most and right most, then 'move' towards the center with two leds at a time
        ledGears[ 0 ].on();
        ledGears[ 5 ].on();
        
        delay( 200 );
        ledGears[ 0 ].off();
        ledGears[ 5 ].off();
        ledGears[ 1 ].on();
        ledGears[ 4 ].on();
        delay( 200 );
        ledGears[ 1 ].off();
        ledGears[ 4 ].off();
        ledGears[ 2 ].on();
        ledGears[ 3 ].on();
        delay( 200 );
        ledGears[ 2 ].off();
        ledGears[ 3 ].off();
    }
}

/// --------------------------------------------------------------------------
/// Setup the Timer Interrupt
/// --------------------------------------------------------------------------
void setupTimerInterrupt()
{
    // initialize timer1 in CTC model for 16Hz
    noInterrupts();           // disable all interrupts
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    
    OCR1A = 15625 / 16;            // compare match register 16MHz/256/ ( 1Hz / 16 ), i.e. 16Hz or 62.5msec
    TCCR1B |= (1 << WGM12);   // CTC mode
    TCCR1B |= (1 << CS12);    // 256 prescaler 
    TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
    interrupts();             // enable all interrupts
}


/// --------------------------------------------------------------------------
/// Setup the DS 1631 Temperature Sensor (I2C IC)
/// --------------------------------------------------------------------------
void setupDS1631()
{
   // Stop conversion to be able to modify "Access Config" Register
  Wire.beginTransmission( DS1631_I2C_ADDRESS );
  Wire.send((int)( DS1631_I2C_COMMAND_STOP_CONVERT )); // Stop conversion
  Wire.endTransmission();  
    
  // READ "Access Config" regsiter
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.send((int)( DS1631_I2C_COMMAND_ACCESS_CONFIG ));
  Wire.endTransmission();
  Wire.requestFrom( DS1631_I2C_ADDRESS,1 ); // Read 1 byte
  Wire.available();
  int AC = Wire.receive(); // receive a byte
    
  // WRITE into "Access Config" Register
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.send( DS1631_I2C_COMMAND_ACCESS_CONFIG );
  Wire.send( DS1631_I2C_CONTROLBYTE_CONT_12BIT ); // Continuous conversion & 12 bits resolution
  Wire.endTransmission();

  // START conversion to get T°
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.send((int)(0x51)); // Start Conversion
  Wire.endTransmission();
}


