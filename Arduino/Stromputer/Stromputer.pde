// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   V-Strom Mk1B - An extra display for a Suzuki DL-650 ("V-Strom") that adds the following functionality:
// []	  1. Battery Level display in Volts - e.g. 14.5V
// []	  2. Gear Position Indicator - e.g. 1, 2, 3, 4, 5, 6, N
// []	  3. Ambient Temperature in Farenheight or Celsius - e.g. 62.5F
// []	  4. [Future] Led display the gear position (one led for each gear 1-6, in different colors, N will be either blank or blinking 1)
// []	  5. [Future] Accurate display of the fuel level (in percentage)
// []	  6. [Future] Fix the OEM V-Strom Fuel Gauge to become linear
// []
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
// []     0.7 - Prototype, ADC worksonly with PCF8591
// []     0.8 - Prototype, ADC works with Arudino Analog or PCF8591
// []     0.9 - 12/3/2011, Prototype, add timed actions - Battery and Temperature should not be polled every loop, just once a few seconds
// []     0.10 - Stable Prototype
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

// References/Credits:
// DS1631: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1221926830

#define VERSION "0.10"

#include <Wire.h>
#include <inttypes.h>

// Include 3rd Party library - LCD I2C NHD
// http://www.arduino.cc/playground/Code/LCDi2c
#include <LCDi2cNHD.h>                    

// Include 3rd Party library - Timed Actions
// http://www.arduino.cc/playground/Code/TimedAction
#include <TimedAction.h>

#define SERIAL_SPEED 9600

// ---------------- Timed Actions (Scheduled Events) -------------
// Timed action for processing battery level
TimedAction battLevelTimedAction = TimedAction( 2000, ProcessBatteryLevel );
// Timed action for processing temperature
TimedAction temperatureTimedAction = TimedAction( 5000, ProcessTemperature );

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

// Battery level (4:1 voltage divider) is connected to Analog Pin 0
#define ANALOGPIN_BATT_LEVEL 0
// Gear Position (2:1 voltage divider) is connected to Analog Pin 1
#define ANALOGPIN_GEAR_POSITION 1

#define LED_PIN 13

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
int lastGear = -2;          // Force initial update
float gearVolts[] = { 5, 4.5, 4.8, 4.3 };
byte gearReadError = 0;
float gearPositionVolts = 0;
 
float battLevel;             // Volts
float lastBattLevel = -0.1;  // Force initial update
byte battReadError = 0;

int displayRefreshTime = 125; // msec

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
#define Welcome1_Line1 "V-Strom Mk1B"
#define Welcome1_Line2 "\"White Pearl\""
#define Welcome2_Line1 "Bigman73 Ver="
#define Welcome2_Line2 "Stromtrooper.com"

/// --------------------------------------------------------------------------
/// Arduino one-time setup routine - i.e. Program entry point like main()
/// --------------------------------------------------------------------------
void setup() 
{ 
    // Setup Serial connection
    Serial.begin( SERIAL_SPEED );
    Serial.print("------- V-Strom Mk1B, VERSION: "); Serial.println(VERSION);
    
    pinMode(LED_PIN, OUTPUT);     
    BlinkLed();
    
    lcd.init();                          // Init the display, clears the display

    // Set initial backlight
    // TODO: In future, control automatically and continously with light sensor
    lcd.setBacklight( 2 );

    #ifdef SHOW_WELCOME
    PrintWelcomeScreen(Welcome1_Line1, Welcome1_Line2, 1000, 25, DIRECTION_RIGHT );
    char line1[16] = Welcome2_Line1;
    strcat( line1, VERSION );
    PrintWelcomeScreen(line1, Welcome2_Line2, 1000, 25, DIRECTION_LEFT );
    #endif
    
    SetupDS1631();

    #ifdef PCF8591_READ
        Serial.print( "===>     PCF8591_READ" );
    #endif
}

/// --------------------------------------------------------------------------
/// Arduino loop routine - gets called all the time in an infinite loop
/// --------------------------------------------------------------------------
void loop()
{
    BlinkLed();
    #ifdef PCF8591_READ
         ReadBatteryAndGearPositionPCF8591();
    #else
        #ifdef PCF8591_GEAR_EMULATOR
        // NOTE: DEBUG MODE ONLY - AUTO INCREMENTS EMULATOR OUTPUT VOLTAGE (Used for gear emulation)
        dac_value += 4;
        ControlPCF8591_I2C( dac_value, all_adc_values, PCF8591_MASK_CHANNEL0 ); // Output DAC, ADC from channel 1
        #endif
        
        ReadGearPositionAnalog();          
    #endif

    PrintGearPosition();

    battLevelTimedAction.check();       
    temperatureTimedAction.check();

    delay(displayRefreshTime);
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for battLevelTimedAction - 
//     Processes the battery level
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



/// --------------------------------------------------------------------------
/// Setup the DS 1631 Temperature Sensor (I2C IC)
/// --------------------------------------------------------------------------
void SetupDS1631()
{
   // Stop conversion to be able to modify "Access Config" Register
  Wire.beginTransmission( DS1631_I2C_ADDRESS );
  Wire.send((int)( DS1631_I2C_COMMAND_STOP_CONVERT )); // Stop conversion
  Wire.endTransmission();  
    
  // Read "Access Config" regsiter
  
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
         gear = 0;  // Neutral
     else
         gear = -1;
}

/// ----------------------------------------------------------------------------------------------------
/// Print the current gear position (only if there has been a changed)
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
    if ( !gearReadError && ( lastGear == gear ) )
    {
        return;
    }

    #ifdef SERIAL_DEBUG
    Serial.print( ", lastGear = " ); Serial.print( lastGear );
    Serial.print( ", gear = " ); Serial.println( gear );
    #endif
    
    // Keep the current gear position, to optimize display time
    lastGear = gear;

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

/// ----------------------------------------------------------------------------------------------------
/// Blinks the LED once (on and off)
/// ----------------------------------------------------------------------------------------------------
void BlinkLed()
{
    digitalWrite(LED_PIN, HIGH);   // set the LED on
    delay(10);
    digitalWrite(LED_PIN, LOW);   // set the LED off
    delay(10);
}