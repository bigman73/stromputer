// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   Stromputer - An enhanced display for a Suzuki DL-650 ("V-Strom") that adds the following functions:
// []	  1. Battery Level display in Volts - e.g. 14.5V
// []	  2. Gear Position Indicator on LCD - e.g. 1, 2, 3, 4, 5, 6, N
// []	  3. Ambient Temperature in Farenheight or Celsius - e.g. 62.5F
// []	  4. LED display of gear position (one led for each gear 1-6, in different colors, N will be blinking on 1)
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

/*
 .::::::.:::::::::::::::::::..       ...     .        :::::::::::. ...    :::::::::::::::.,:::::: :::::::..   
;;;`    `;;;;;;;;'''';;;;``;;;;   .;;;;;;;.  ;;,.    ;;;`;;;```.;;;;;     ;;;;;;;;;;;'''';;;;'''' ;;;;``;;;;  
'[==/[[[[,    [[      [[[,/[[['  ,[[     \[[,[[[[, ,[[[[,`]]nnn]]'[['     [[[     [[      [[cccc   [[[,/[[['  
  '''    $    $$      $$$$$$c    $$$,     $$$$$$$$$$$"$$$ $$$""   $$      $$$     $$      $$""""   $$$$$$c    
 88b    dP    88,     888b "88bo,"888,_ _,88P888 Y88" 888o888o    88    .d888     88,     888oo,__ 888b "88bo,
  "YMmMY"     MMM     MMMM   "W"   "YMMMMMP" MMM  M'  "MMMYMMMb    "YmmMMMM""     MMM     """"YUMMMMMMM   "W" 
*/

#include <Wire.h>

// ================================================================================================
// ==> Select your LCD type here: Only ONE define should be active from the LCD_TYPE_* defines
#define LCD_TYPE_NHD 1
// #define LCD_TYPE_LIQUIDCRYSTAL 1
// ================================================================================================


// -----------------    Library Includes    ------------------------
#ifdef LCD_TYPE_NHD

  // Include 3rd Party library - LCD I2C NHD
  // http://www.arduino.cc/playground/Code/LCDi2c
  #include <LCDi2cNHD.h>                    
#elif LCD_TYPE_LIQUIDCRYSTAL

  // Include 3rd Party library - LCD Crystal I2C 
  #include <LCD.h>
  #include <LiquidCrystal_I2C.h>                    

#endif


// Include 3rd Party library - Timed Actions (Modified by Yuval Naveh)
// http://www.arduino.cc/playground/Code/TimedAction
#include <TimedAction.h>

// Include 3rd Party library - LED (Modified by Yuval Naveh)
// http://arduino.cc/playground/Code/LED
#include <LED.h>

// Include 3rd Party library - TimerOne, used for Timer ISR
// http://arduino.cc/playground/Code/Timer1
#include <TimerOne.h>

// EEPROM support
#include <EEPROM.h>
// EEProm Anything: http://arduino.cc/playground/Code/EEPROMWriteAnything
#include "EEPROMAnything.h"

// 1-Wire Support (Dallas Semiconductor Serial Communication Protocol)
// http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <OneWire.h>
// Dallas Temperature (i.e. DS18B20) support
// https://code.google.com/p/dallas-temperature-control-library/
#include <DallasTemperature.h>

#include <RunningAverage.h>

// -----------------------------------------------------------------

// Other References/Credits:
// DS1631: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1221926830
// ISR: http://letsmakerobots.com/node/28278
// Light Sensors: http://www.ladyada.net/learn/sensors/cds.html

#include "Stromputer.h"


// ---------------- Timed Actions (Scheduled re-occouring Events) -------------

// Timed action for forced LCD update
TimedAction checkForceLCDRefreshTimedAction = TimedAction( 0, LCD_FORCEREFRESH_INTERVAL / 2 , checkForceLCDRefresh );

// Timed action for processing battery level
TimedAction battLevelTimedAction = TimedAction( 0, PROCESS_BATT_LEVEL_TIMED_INTERVAL , processBatteryLevel );
// Timed action for processing temperature
TimedAction temperatureTimedAction = TimedAction( 0, PROCESS_TEMP_TIMED_INTERVAL, processTemperature );
// Timed action for processing photo cell light 
TimedAction photoCellTimedAction = TimedAction( 0, PROCESS_PHOTO_CELL_TIMED_INTERVAL, processPhotoCell );
// Timed action for Sampling & LCD Display
TimedAction lcdDisplayTimedAction = TimedAction( 0, LCD_DISPLAY_LOOP_TIMED_INTERVAL, lcdDisplayLoop );

// Timed action for Serial Input
TimedAction serialInputTimedAction = TimedAction( 0, SERIALINPUT_TIMED_INTERVAL, processSerialInput );

OneWire oneWire( DIGITALPIN_DS18B20 );
DallasTemperature DS18B20Sensor( &oneWire ); 

/// --------------------------------------------------------------------------
/// Arduino one-time setup routine - i.e. Program entry point like main()
/// --------------------------------------------------------------------------
void setup() 
{ 
    // Setup Serial connection
    Serial.begin( SERIAL_SPEED_BAUD );
    Serial.print( MSG_FIRMWARE ); Serial.println( VERSION );

    onBoardLed.on();    
    
    // Start Temperature interface
    DS18B20Sensor.begin();                   
    
    setupConfiguration();

    if ( !initializeLCD() )
    {
        Serial.println( ERR_LCD_INIT_FAILED );
    }

    showWelcome();
   
#ifdef USE_DS1631   
    if ( !initializeDS1631() )
    {
        Serial.println( ERR_DS1631_INIT_FAILED );
    }
#endif    
    	
    // set Timer1
    Timer1.initialize( TIMER1_RESOLUTION );
    Timer1.attachInterrupt( timerISR ); // attach the service routine here
 
    battLevelTimedAction.force();
    temperatureTimedAction.force();
    photoCellTimedAction.force();
    
    forceLCDRefresh( true );
     
    Serial.println( MSG_STROMPUTER_READY );
}


/// --------------------------------------------------------------------------
/// Arduino loop routine - gets called all the time in an infinite loop
/// --------------------------------------------------------------------------
void loop()
{
    serialInputTimedAction.check();
    lcdDisplayTimedAction.check();
    
    delay(LoopSleepTime);
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for Sampling & LCD Display Loop 
/// --------------------------------------------------------------------------
void lcdDisplayLoop()
{   
    // Timed Actions ("Events")
    checkForceLCDRefreshTimedAction.check();
    battLevelTimedAction.check();       
    temperatureTimedAction.check();
    photoCellTimedAction.check();

    #ifdef PCF8591_DAC_GEAR_EMULATOR
    // NOTE: DEBUG MODE ONLY - AUTO INCREMENTS EMULATOR OUTPUT VOLTAGE (Used for gear emulation)
    dac_value += 3;
    controlPCF8591_I2C( dac_value, all_adc_values, PCF8591_MASK_CHANNEL0 ); // Output DAC, ADC from channel 1
    #endif
    
    printGearPosition();
}

unsigned short int timerDivider = 0;
bool disableTimerISR = false;

/// --------------------------------------------------------------------------
/// Timer compare Interrupt Service Routine (ISR)
/// Note: Setup to run on 1Khz (1msec)
/// --------------------------------------------------------------------------
void timerISR()
{
    if ( disableTimerISR )
        return;
    
    // Handle main board blinking at 1Hz for each toggle
    if ( timerDivider % 1000 == 1 ) 
    {
        /// Toggle the Main Board LED
        onBoardLed.toggle();
    }

    // Handle gear LED display at 20Hz (i.e. check every 50 msec)
    if ( timerDivider % 50 == 1 ) 
    {
        // Read the gear position analog value 
        readGearPositionAnalog();          

        // Update Gear Position LEDs (note: updateGearLEDs() is optimized to only actually refresh on gear change)
        updateGearLEDs();
    }
    
    // Handle neutral gear blinking, at 4Hz for each toggle
    if ( timerDivider % 250 == 1  && 
         gear == GEAR_NEUTRAL )
    {
        // Toggle 1st Gear LED On/Off (using PWM) , signaling the rider that the gear is N
        // Note: LED::Toggle() doesn't work well, because it uses digitalWrite instead of analogWrite, thus doesn't use PWM and causes the LED to show with full brightness always
        ledGears[ 0 ].setValue( ledGears[ 0 ].getState() ? 0 : ledBrightnessGreen ); // If last state was true, toggle it to true, and visa versa
    }    

    timerDivider++;
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for checkForceLCDRefreshTimedAction - 
///     Triggers a full screen refresh of the NHD LCD 
/// --------------------------------------------------------------------------
void checkForceLCDRefresh()
{
    forceLCDRefresh( false );
}

/// --------------------------------------------------------------------------
/// Force LCD Refresh
/// --------------------------------------------------------------------------
void forceLCDRefresh( bool forceNow )
{
    // Force full screen LCD refresh every X seconds (some unknown bug with NHD LCD causes it to clear the screen from time to time)
    if ( forceNow || ( millis() - lastForceLCDRefreshMillis > LCD_FORCEREFRESH_INTERVAL ) )
    {
        isForceRefreshTemp = true;
        isForceRefreshBatt = true;
        isForceRefreshGear = true;
        lastForceLCDRefreshMillis = millis();
        
        // Force the timed actions to run
        battLevelTimedAction.force(); 
        temperatureTimedAction.force();
        photoCellTimedAction.force();
    }
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for battLevelTimedAction - 
///     Processes the battery level
/// --------------------------------------------------------------------------
void processBatteryLevel()
{   
    readBatteryLevelAnalog();
    
    // When 'live' on the motorcycle, there is a difference between what Arduino samples and what a volt meter samples.
    // TODO: Figure out the difference, probably due to a diode somewhere
    // As a temporary workaround, a constant is added
    if ( battLevel > 1.5f )
        battLevel += 0.8f;
    
    printBatteryLevel();
}

/// ----------------------------------------------------------------------------------------------------
/// Reads the current battery level from the voltage divider circuit (4:1, 20V -> 5V), using Arduino ADC
/// ----------------------------------------------------------------------------------------------------
void readBatteryLevelAnalog()
{
    battReadError = true; // Assume read had errors, by default

    int adcValue = analogRead( ANALOGPIN_BATT_LEVEL );    // read the input pin for Battery Level
    
    float currentBattLevel = 5.0f * BATT_VOLT_DIVIDER * ( adcValue / 1024.0f );
    currentBattLevel = constrain( currentBattLevel, 0.01f, 20.0f );

    // Trim the battery level average when the switch is turned on for the first time
    if ( currentBattLevel > 8.f && battLevel < 2.0f )   
    {
        battRunAvg.trimToValue( currentBattLevel );
    }
    // Regular operation
    else
    {
        // Update the moving average window
        battRunAvg.addValue( currentBattLevel );    
    }

    battLevel = battRunAvg.getAverage();
        
    battReadError = false; // Clear read error - we made it here
    
}

/// ----------------------------------------------------------------------------------------------------
/// Print the current battery level (only if there has been a changed)
/// ----------------------------------------------------------------------------------------------------
void printBatteryLevel()
{
    if ( isForceRefreshBatt )
    {
           isForceRefreshBatt = false;
    }
    else
    {
        // Optimization: Don't print the battery level if nothing has changed since the last printing
        if ( !battReadError && ( CompareFloats( lastBattLevel, battLevel )  ) )
        {
            return;
        }
    }
    
    // Keep the current battery level, to optimize display time
    lastBattLevel = battLevel;

    // Print Battery Label
    lcd_print_at(LCD_ROW_BATT_LABEL, LCD_COL_BATT_LABEL, BATTERY_LABEL );  
    
    // Print Battery Value
    String battLevelValue = NOTAVAIL;
    
    if ( battReadError )
    {
        battLevelValue = "ERR  ";
    }
    else
    {
        // Format battery float value
        char formattedBattLevel[5]; // DD.D + NULL => Maximum 5 characters
        dtostrf(battLevel, 4, 1, formattedBattLevel );
        battLevelValue = formattedBattLevel;

        // Add volts unit
        battLevelValue += UNIT_VOLT;
    }
      
    lcd_print_at(LCD_ROW_BATT_VALUE, LCD_COL_BATT_VALUE, battLevelValue );  
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for photoCellTimedAction - 
///     Processes the photo cell level and adjust LCD Back Light accordingly
/// --------------------------------------------------------------------------
void processPhotoCell()
{   
    readPhotoCellAnalog();
   
    if ( photoCellLevel == 0 ) // Error - CdS is 0 only when sensor is not working
        lightLevel = DEFAULT_LIGHT_LEVEL; // Default
    // Determine new LCD Back Light level (1..8, 1 is very dim .. 8 is very bright)
    else if ( photoCellLevel < PHOTOCELL_LEVEL1 )
        lightLevel = 1; // Very dim
    else if ( photoCellLevel < PHOTOCELL_LEVEL2 )
        lightLevel = 2;
    else if ( photoCellLevel < PHOTOCELL_LEVEL3 )
        lightLevel = 3;
    else if ( photoCellLevel < PHOTOCELL_LEVEL4 )
        lightLevel = 4;
    else if ( photoCellLevel < PHOTOCELL_LEVEL5 )
        lightLevel = 5;
    else if ( photoCellLevel < PHOTOCELL_LEVEL6 )
        lightLevel = 6;
    else if ( photoCellLevel < PHOTOCELL_LEVEL7 )
        lightLevel = 7;
    else
        lightLevel = 8; // Very bright

    // Only update the LCD backlight if there is actually a change (to reduce costly I2C traffic)
    if ( lightLevel != lastLightLevel )
    {       
        // if a large change, use a smoothing function (average) to reduce sharp/large changes
        if ( abs( lightLevel - lastLightLevel ) > 1 )
            lightLevel = ( lightLevel + lastLightLevel ) / 2;
            
        lastLightLevel = lightLevel;
                  
        forceLedUpdate = true; // Force update of LEDs
         
        lcd.setBacklight( lightLevel );
    }    
  
    lcd_print_at(LCD_ROW_BACK_LIGHT, LCD_COL_BACK_LIGHT, lightLevel );  
}

/// ----------------------------------------------------------------------------------------------------
/// Reads the current photo cell level from the voltage divider circuit (3K-11K : 10K), using Arduino ADC
/// ----------------------------------------------------------------------------------------------------
void readPhotoCellAnalog()
{
    photoCellLevel = ( short ) analogRead( ANALOGPIN_PHOTCELL );    // read the input pin for Photo Cell Level
}

// ----------------------------------------------------------------------------------------------------
/// Reads the current gear position level from the voltage divider circuit (2:1, 10V -> 5V), using Arduino ADC
/// ----------------------------------------------------------------------------------------------------
void readGearPositionAnalog()
{
    gearReadError = true; // Assume read had errors, by default
   
    int value = analogRead( ANALOGPIN_GEAR_POSITION );
        
    float currentGearPositionVolts = 5.0f * GEAR_VOLT_DIVIDER * ( value / 1024.0f );    // read the input pin for Gear Position
    // Constrain the input reading, to reduce errors
    currentGearPositionVolts = constrain( currentGearPositionVolts, 0.01f, 5.0f );
    
    if ( abs( currentGearPositionVolts - gearLevelRunAvg.getAverage() ) > 0.5f ) // big change, probably a real gear shift
    {
        gearLevelRunAvg.trimToValue( currentGearPositionVolts );
    }
    else // small change
    {    
        // Recalc average volts
        gearLevelRunAvg.addValue( currentGearPositionVolts );
    }

    // Use the average gear volts
    gearPositionVolts = gearLevelRunAvg.getAverage();
    
    determineCurrentGear();
    
    gearReadError = false; // Clear read error - we made it here
}
// []     0.44 -   4/21/2012 + Restored VREF self learning mechanism

// ----------------------------------------------------------------------------------------------------
/// Determins the current gear from the gear position volts
/// ----------------------------------------------------------------------------------------------------
void determineCurrentGear()
{
     short lastTransientGear = transientGear;

     // Calculate fixed gear position volts
     float fixedGearPositionVolts = gearPositionVolts;
     
     if ( IsBetween( fixedGearPositionVolts,  GEAR1_FROM_VOLTS, GEAR2_FROM_VOLTS ) )
         transientGear = 1;
     else if ( IsBetween( fixedGearPositionVolts, GEAR2_FROM_VOLTS, GEAR3_FROM_VOLTS) )
         transientGear = 2;
     else if ( IsBetween( fixedGearPositionVolts , GEAR3_FROM_VOLTS, GEAR4_FROM_VOLTS ) )
         transientGear = 3;
     else if ( IsBetween( fixedGearPositionVolts, GEAR4_FROM_VOLTS, GEAR5_FROM_VOLTS ) )
         transientGear = 4;
     else if ( IsBetween( fixedGearPositionVolts, GEAR5_FROM_VOLTS, GEAR6_FROM_VOLTS ) )
         transientGear = 5;
     else if ( IsBetween( fixedGearPositionVolts, GEAR6_FROM_VOLTS, GEARN_FROM_VOLTS ) )
         transientGear = 6;
     else if ( IsBetween( fixedGearPositionVolts, GEARN_FROM_VOLTS, GEARN_TO_VOLTS ) )
         transientGear = GEAR_NEUTRAL;
     else
         transientGear = GEAR_ERROR; // Default: Error
         
     // If transient gear has changed, do not allow it to stabilize
     if ( lastTransientGear != transientGear )
     {
         // Reset the transient start counter
         transientGearStartMillis = millis();
         lastTransientGear = transientGear;
     }
     else // Transient gear is the same since last time
     {
         int deltaTransientMillis = millis() - transientGearStartMillis;
         // check if it was stable long enough
         if ( deltaTransientMillis > MIN_TRANSIENTGEAR_INTERVAL )
         {
             // Make transient gear change a stable gear change
             gear = transientGear;

             transientGear = GEAR_ERROR;
         }
     }
}

/// ----------------------------------------------------------------------------------------------------
/// Print the current gear position (only if there has been a change)
/// ----------------------------------------------------------------------------------------------------
void printGearPosition()
{    
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    #ifdef DEBUG_PRINT_GEARVOLTS

    char buffer[6];
    dtostrf(gearPositionVolts, 4, 2, buffer );
    lcd_print_at(LCD_ROW_GEARVOLTS, LCD_COL_GEARVOLTS, buffer );  
        
    #endif
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    if ( isForceRefreshGear )
    {
        isForceRefreshGear = false;
    }
    else
    {
        // Optimization: Don't print the gear position if nothing has changed since the last printing
        if ( !gearReadError && ( lastGearLCD == gear ) )
        {
            return;
        }
    }
    
    // Keep the current gear position, to optimize LCD display time
    lastGearLCD = gear;
        
    // Print Gear Position Label
    #ifndef DEBUG_PRINT_GEARVOLTS

    lcd_print_at(LCD_ROW_GEARLABEL, LCD_COL_GEARLABEL, GEAR_LABEL );  
    
    #endif
    
    // Print Gear Position Value
    String gearValue;
    if ( gearReadError )
    {
        gearValue = "ERR ";
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
    
        gearValue.concat( "= " );
    }

    lcd_print_at(LCD_ROW_GEAR, LCD_COL_GEAR, gearValue );  
}

/// ----------------------------------------------------------------------------------------------------
/// Updates the gear LEDs with current gear position (only if there has been a change)
/// Note: Netural is blinking and is therefore being handled with a timer ISR, by NeutralGearLedBlink() 
/// ----------------------------------------------------------------------------------------------------
void updateGearLEDs()
{
    // Optimization: Don't print the gear position if nothing has changed since the last printing
    if ( !gearReadError && !forceLedUpdate && ( lastGearLED == gear ) )
    {
        return;
    }
    
    forceLedUpdate = false;
    lastGearLED = gear;
    
    // Update each gear led, only if not in error mode
    if ( gear != GEAR_ERROR )
    {
        byte greenFactor;
        byte yellowFactor;
        byte whiteFactor;
        byte blueFactor;
        if ( lightLevel < 4 ) {
            // Night time - dimmer
            greenFactor = 1;
            yellowFactor = 2;
            whiteFactor = 2;
            blueFactor = 2;
        }
        else {
            // Day time - brighter
            greenFactor = 3;
            yellowFactor = 12;
            whiteFactor = 21;
            blueFactor = 18;
        }
        
        ledBrightnessGreen = 1 + lightLevel * greenFactor; // Note: Green LED (1st Gear LED) is extremely bright even with very small currents/PWM duty cycle
        ledBrightnessYellow = 1 + lightLevel * yellowFactor; // PWM 0..255 : 0%-100%
        ledBrightnessWhite = 75 + lightLevel * whiteFactor; // PWM 0..255 : 0%-100%
        ledBrightnessBlue = 1 + lightLevel * blueFactor; // PWM 0..255 : 0%-100%
      
         // Do not handle N gear, the ISR will take care of it
        if ( gear != GEAR_NEUTRAL )
        {
            ledGears[1-1].setValue( ( gear >= 1 && gear < 6 ) ? ledBrightnessGreen    : 0 );
        }
        ledGears[2-1].setValue( ( gear >= 2 && gear < 6 ) ? ledBrightnessYellow   : 0 );
        ledGears[3-1].setValue( ( gear >= 3 && gear < 6 ) ? ledBrightnessYellow   : 0 );
        ledGears[4-1].setValue( ( gear >= 4 && gear < 6 ) ? ledBrightnessWhite    : 0 );
        ledGears[5-1].setValue(   gear == 5               ? ledBrightnessWhite    : 0 );
        ledGears[6-1].setValue(   gear == 6               ? ledBrightnessBlue     : 0 );  // Overdrive - show only the 6th gear
        
        /// LED Layout: Gr  Yl   Yl  Wh  Wh  Bl
        ///         LED: 1   2   3   4   5   6
        ///  GEAR
        ///  N:          B   -   -   -   -   -
        ///  1:          X   -   -   -   -   -
        ///  2:          X   X   -   -   -   -
        ///  3:          X   X   X   -   -   -
        ///  4:          X   X   X   X   -   -
        ///  5:          X   X   X   X   X   -
        ///  6:          -   -   -   -   -   X     Over Drive Mode: The last LED light on only when 6th gear is engaged
        ///  B = Blink,   X = On,  - = Off
    }
}


/// --------------------------------------------------------------------------
/// Timed Action Event handler for temperatureTimedAction - 
//     Processes the ambient temperature
/// --------------------------------------------------------------------------
void processTemperature()
{
    readTemperatureDS18B20();

#ifdef USE_DS1631
    readTemperatureDS1631();
       
    // Workaround to a problem with DS1631: From time to time, the IC goes nuts and starts returning odd readings (TODO: Check if related to pull up resistor values, or breadboard, or the generic IC socket
    // Check if DS1631 is returning bad temperature vlaues (but not on boot last temperature is set to -99), if yes, re-establish communication with it
    if ( !onBoardTemperatureReadError && lastOnBoardTemperature > TEMPERATURE_MIN_VALID && abs( lastOnBoardTemperature - onBoardTemperature ) > TEMPERATURE_ERROR_DIFF )
    {
        // Re-Initialize DS1631 - Stop temperature conversion, and start it again
        initializeDS1631();
        
        // Now read temperature again
        readTemperatureDS1631();
        
        // If still there are odd readings then declare temperature error mode
        if ( abs( lastOnBoardTemperature - onBoardTemperature ) > TEMPERATURE_ERROR_DIFF )
        {
            onBoardTemperatureReadError = true;
        }
    }
#endif    

    printTemperature(); 
  
    // ******** Handle AUTO STAT Mode **************
    if ( autoStat )
    {
      printStat();
    }
    // *********************************************
}

/// ----------------------------------------------------------------------------------------------------
// Read the current temperature from OneWire DS18B20 Thermometer
/// ----------------------------------------------------------------------------------------------------
void readTemperatureDS18B20( )
{
    temperatureReadError = true;
    
    DS18B20Sensor.requestTemperatures( );
    ds18b20Temperature = DS18B20Sensor.getTempCByIndex( 0 );
    if ( ds18b20Temperature == DEVICE_DISCONNECTED )
        return;
    
    if ( configuration.temperatureMode == 'F' )
        // Convert celsius to fahrenheight
        ds18b20Temperature = DallasTemperature::toFahrenheit( ds18b20Temperature );
  
    temperatureReadError = false;
}

/// ----------------------------------------------------------------------------------------------------
/// Reads the current temperature from the I2C DS1631 Thermometer 
/// ----------------------------------------------------------------------------------------------------
#ifdef USE_DS1631
void readTemperatureDS1631()
{   
    onBoardTemperatureReadError = true; // Assume read had errors, by default
    
    // READ T° from DS1631
    Wire.beginTransmission( DS1631_I2C_ADDRESS );
    Wire.write((int)( DS1631_I2C_COMMAND_READ_TEMP ));
    Wire.endTransmission();
    Wire.requestFrom( DS1631_I2C_ADDRESS, 2 ); // READ 2 bytes
    int bytes;
    bytes = Wire.available(); // 1st byte
    if (bytes != 2)
        return;
    int _TH = Wire.read(); // receive a byte (TH - High Register)
    
    bytes = Wire.available(); // 2nd byte
    if (bytes != 1)
      return;
    int _TL = Wire.read(); // receive a byte (TL - Low Register)
    
    // T° processing
    if ( _TH >= 0x80 ) //if sign bit is set, then temp is negative
        _TH = _TH - 256; 
    onBoardTemperature = _TH + _TL / 256.0;
      
    if ( configuration.temperatureMode == 'F' )
    {
        // Convert celsius to fahrenheight
        onBoardTemperature = DallasTemperature::toFahrenheit( onBoardTemperature );
    }
    
    onBoardTemperatureReadError = false; // Clear read error - we made it here
}
#endif

/// ----------------------------------------------------------------------------------------------------
/// Print the current temperature (only if it has been a changed)
/// ----------------------------------------------------------------------------------------------------
void printTemperature()
{
    if ( isForceRefreshTemp )
    {
       isForceRefreshTemp = false;
    }
    else
    {
        // Optimization: Don't print the temperature if nothing has changed since the last printing
        if ( !temperatureReadError && ( CompareFloats( lastTemperature, ds18b20Temperature ) ) )
        {
            return;
        }
    }

    // Keep the current gear position, to optimize display time
    lastTemperature = ds18b20Temperature;

    // Print temperature label
#ifndef USE_DS1631
    lcd_print_at(LCD_ROW_TEMP_LABEL, LCD_COL_TEMP_LABEL, TEMPERATURE_LABEL );       
#endif    

    // Print temperature value
    String temperatureValue;

    if ( temperatureReadError )
    {
        temperatureValue = " ERR  ";
    }
    else if ( ds18b20Temperature < -55 )
    {
        // Workaround when there are odd readings on first seconds when temperature sensor is starting up
        temperatureValue = " ----";
    }
    else
    {      
        // format temperature into a fixed .1 format (e.g. 62.5 or 114.4 [too hot to ride! :) ] or -10.7 [too cold to ride! :) ])
        char formattedTemperature[7]; // [-]DDD.D + NULL => Maximum 7 characters (NULL included)
        dtostrf(ds18b20Temperature, 6, 1, formattedTemperature );
        temperatureValue = formattedTemperature;
        
        // Add temperature mode unit
        if ( configuration.temperatureMode == 'F' )
        {
            temperatureValue += "F";
        }
        else
        {
            temperatureValue += "C";
        }
    }
    
    lcd_print_at(LCD_ROW_TEMP_VALUE, LCD_COL_TEMP_VALUE, temperatureValue );  

#ifdef USE_DS1631
    // -- DEBUG OnBoard temperature
    char formattedTemperature[7]; // [-]DDD.D + NULL => Maximum 7 characters (NULL included)
    dtostrf(onBoardTemperature, 6, 1, formattedTemperature );
    String onboardTemperatureValue = formattedTemperature;
    onboardTemperatureValue += "*";
    lcd_print_at(LCD_ROW_OBTEMP_VALUE, LCD_COL_OBTEMP_VALUE, onboardTemperatureValue );  
#endif    
}
/// ------------------------------------------------------------
/// Show all gear LEDs
/// ------------------------------------------------------------
void showAllGearLEDs()
{
    Serial.println( MSG_SHOW_ALL_GEAR_LEDS );
    
    for ( int i=0; i < 2; i++ )
    {
        // Light left most and right most, then 'move' towards the center with two leds at a time
        ledGears[ 0 ].on();
        ledGears[ 5 ].on();
        
        delay( 100 );
        ledGears[ 0 ].off();
        ledGears[ 5 ].off();
        ledGears[ 1 ].on();
        ledGears[ 4 ].on();
        delay( 100 );
        ledGears[ 1 ].off();
        ledGears[ 4 ].off();
        ledGears[ 2 ].on();
        ledGears[ 3 ].on();
        delay( 100 );
        ledGears[ 2 ].off();
        ledGears[ 3 ].off();
    }
}

/// ------------------------------------------------------------
/// Tests each gear LED
/// ------------------------------------------------------------
void testEachGearLED()
{
    Serial.println( MSG_TEST_EACH_GEAR_LED );
        
    for ( int i=0; i < 1; i++ )
    {
        for ( int j=0; j <6 ; j++ )
        {
            for ( int k=255; k>= 0; k-=6 )            
            {
                ledGears[ j ].setValue(k);
                delay( 100 );
            }
            ledGears[ j ].off();
            delay( 200 );            
        }
    }
}

/// --------------------------------------------------------------------------
/// Initialize the LCD (I2C IC)
/// --------------------------------------------------------------------------
bool initializeLCD()
{   
    Serial.println( MSG_LCD_INIT_BEGIN );

#ifdef LCD_TYPE_NHD
    // Initialize the display, clears the display
    if ( !lcd.init() )
    {
        return false;
    }

    // Set initial LCD backlight & contrast
    lcd.setBacklight( lightLevel );
    lcd.setContrast( lcdContrast );
    
#elif LCD_TYPE_LIQUIDCRYSTAL
    
    // Turn backlight on
    lcd.setBacklight();
    
    // initialize the lcd 
    lcd.begin(LCD_ROWS, LCD_COLS);
    lcd.clear();    
    
#endif

    Serial.println( MSG_LCD_INIT_END );
    lcdInitialized = true;
    
    return true;
}

/// --------------------------------------------------------------------------
/// Initialize the DS 1631 Temperature Sensor (I2C IC)
/// --------------------------------------------------------------------------
bool initializeDS1631()
{
  Serial.println( MSG_DS1631_INIT_BEGIN );
  
   // Stop conversion to be able to modify "Access Config" Register
  Wire.beginTransmission( DS1631_I2C_ADDRESS );
  Wire.write((int)( DS1631_I2C_COMMAND_STOP_CONVERT )); // Stop conversion
  if ( Wire.endTransmission() )
  {
    Serial.println( F( ">> Error: Stop Convert command failed" ) );
    return false; // Error
  }
    
  // READ "Access Config" register
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.write((int)( DS1631_I2C_COMMAND_ACCESS_CONFIG ));
  if ( Wire.endTransmission() )
  {
      Serial.println( F( ">> Error: Access Config command failed" ) );
      return false; // Error
  }

  Wire.requestFrom( DS1631_I2C_ADDRESS,1 ); // Read 1 byte
  Wire.available();
  Wire.read(); // receive a byte (AC), but ignore it, we have no use for it
    
  // WRITE into "Access Config" Register
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.write( DS1631_I2C_COMMAND_ACCESS_CONFIG );
  Wire.write( DS1631_I2C_CONTROLBYTE_CONT_12BIT ); // Continuous conversion & 12 bits resolution
  if ( Wire.endTransmission() )
  {
      Serial.println( F( ">> Error: Access Continous Control Byte command failed" ) );
      return false; // Error
  }

  // START conversion to get T°
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.write((int) DS1631_I2C_COMMAND_START_CONVERT); // Start Conversion
  if ( Wire.endTransmission() )
  {
      Serial.println( F( ">> Error: Start Convert command failed" ) );
      return false; // Error
  }
  
  Serial.println( MSG_DS1631_INIT_END );
  
  return true; // Success
}

/// ----------------------------------------------------------------------------------------------------
/// Show the welcome sequence
/// ----------------------------------------------------------------------------------------------------
void showWelcome()
{
    #ifdef SHOW_WELCOME    
    
    showAllGearLEDs();

    String line2 = String( Welcome1_Line2);
    line2 += VERSION;
    printWelcomeScreen(String(Welcome1_Line1), line2, 800, 25, DIRECTION_RIGHT );
   
    #endif    
}

/// ----------------------------------------------------------------------------------------------------
/// Prints the welcome screen using the given two lines arguments
/// ----------------------------------------------------------------------------------------------------
void printWelcomeScreen( String line1, String line2, int showDelay, int scrollDelay, char scrollDirection )
{
    Serial.println( MSG_WELCOME_BEGIN );     

    lcd.clear();
    
    // Print welcome lines
    lcd_print_at( 0, 0, line1 );  
    lcd_print_at( 1, 0, line2 );  

    delay( showDelay );

#ifdef LCD_TYPE_NHD
    // Scroll Left or Right, based on the scrollDirection argument
    int i2cScrollCommand = ( scrollDirection == DIRECTION_LEFT ) ? LCD_I2C_NHD_SCROLL_LEFT : LCD_I2C_NHD_SCROLL_RIGHT;
    
    // Shift screen right
    for ( int i = 0; i < LCD_COLS; i++ )
    {          
        lcd.command( i2cScrollCommand );
        delay( scrollDelay );
    }
#elif LCD_TYPE_LIQUIDCRYSTAL

    // TODO: Use LCD::scrollDisplayLeft()

#endif    
    
    lcd.clear();
    
    Serial.println( MSG_WELCOME_END );
}

String serialCommandLine = "";
const char *commandArgDelimiter = " ";

/// ----------------------------------------------------------------------------------------------------
/// Processes Serial Input - commands given to Stromputer through the Serial port
/// ----------------------------------------------------------------------------------------------------
void processSerialInput()
{ 
  while (Serial.available() > 0) 
  {
    byte ch = ( byte ) Serial.read();
    
    if ( ch == ( byte ) ';' )
    {      
        disableTimerISR = true;
   
        // handle command line
        serialCommandLine.toUpperCase();
        Serial.println();
        Serial.print( F( "Recieved: " ) ); Serial.println( serialCommandLine );
        
        // Parse command line into tokens
        serialCommandLine.concat( commandArgDelimiter ); // To ensure that the last token is read stopping properly
        char commandLine[30];
        serialCommandLine.toCharArray( commandLine, 30 );
        serialCommandLine = String( "" );
        char *tkn_it;
        String cmd, arg1, arg2;
       
        char *p = strtok_r( commandLine, commandArgDelimiter, &tkn_it );        
        if ( p ) 
        {
            cmd = String( p );
            p = strtok_r( NULL, commandArgDelimiter, &tkn_it );
            if ( p ) 
            {
                arg1 = String( p );
                arg1.toUpperCase();
                p = strtok_r( NULL, commandArgDelimiter, &tkn_it );
                if ( p )
                {
                    arg2 = String( p );
                    arg2.toUpperCase();
                }                    
            }
        }
            
        handleCommand( cmd, arg1, arg2 );
        
        Serial.println();

        Serial.flush();
        delay( 100 );
        disableTimerISR = false;
    }
    else
    {
        // A regular character - add it to the command line
        
        // Protect against commands which are too long
        if ( serialCommandLine.length() >= 30 )
        {
             serialCommandLine = String( "" );
         }
 
         serialCommandLine += ( char ) ch;
    }
  }
}

/// ----------------------------------------------------------------------------------------------------
/// Prints the current device status into the Serial output
/// ----------------------------------------------------------------------------------------------------
void printStat()
{
#ifdef USE_DS1631
    Serial.print( F( "Tob=" ) );
    if ( onBoardTemperatureReadError )
      Serial.println( F( "ERROR" ) );
    else
    {
      Serial.print( onBoardTemperature, 2 );
      Serial.println( configuration.temperatureMode  );
    }
#endif    

    Serial.print( F( "T=" ) );
    if ( temperatureReadError )
      Serial.println( F( "ERROR" ) );
    else
    {
      Serial.print( ds18b20Temperature, 2 );
      Serial.println( configuration.temperatureMode  );
    }    

    Serial.print( F( "Gear=" ) );
    if ( gear == 0 )
        Serial.println( "N" );
    else
        Serial.println( gear );
    Serial.print( F( "GearV=" ) ); 
    Serial.println( gearPositionVolts );
    Serial.print( F( "Batt=" ) ); Serial.println( battLevel );    
    Serial.print( F( "CdS=" ) ); Serial.println( photoCellLevel );
    Serial.print( F( "Light Level=" ) ); Serial.println( lightLevel );
    Serial.print( F( "LCD=" ) ); Serial.println( lcdInitialized ? "Y" : "N" ) ;
    Serial.print( F( "LCD CT=" ) ); Serial.println( lcdContrast );
    Serial.println( F( "------------------" ) ); 
    // Important Note: The serial buffer is limited to 128 bytes. 
    //       Keep it short, or an overflow will occur (currupted memory/hang up)
}

/// ----------------------------------------------------------------------------------------------------
/// Handles setting a persistent configration parameter
/// ----------------------------------------------------------------------------------------------------
void handleSetCfg( String arg1, String arg2 )
{
    if ( arg1.length() == 0 )
        Serial.println( F( "Syntax error: arg1" ) );
    else if ( arg2.length() == 0 )
        Serial.println( F( "Syntax error: arg2" ) );
    else            
    {
        if ( arg1 == CMD_ARG_TEMP )
        {
             char tempMode = arg2[0];
             if ( configuration.temperatureMode != tempMode )
             {
                configuration.temperatureMode = tempMode;
                // Recalc last temperatures in new mode
                if ( tempMode == 'F' )
                {
                    lastTemperature = DallasTemperature::toFahrenheit( ds18b20Temperature ); 
#ifdef USE_DS1631
                    lastOnBoardTemperature = DallasTemperature::toFahrenheit( onBoardTemperature ); 
#endif                    
                }
                else
                {
                    lastTemperature = DallasTemperature::toCelsius( ds18b20Temperature ); 
#ifdef USE_DS1631
                    lastOnBoardTemperature = DallasTemperature::toCelsius( onBoardTemperature );                    
#endif                    
                }
                
                writeConfiguration();
                
                isForceRefreshTemp = true;
                temperatureTimedAction.force();

                Serial.print( F( "Temp mode set to: " ) ); Serial.println( tempMode );
                
            }
            else
            {
                Serial.print( F( "Nothing done" ) );
            }           
        }
        else
        {
            Serial.print( F( "Unkonwn token2: " ) ); Serial.println( arg1 );
        }
    }
}

/// ----------------------------------------------------------------------------------------------------
/// Handle a single command 
/// ----------------------------------------------------------------------------------------------------
void handleCommand( String cmd, String arg1, String arg2 )
{
      if ( cmd == CMD_ALIVE )
      {
          Serial.println( MSG_ISALIVE );
      }
      else if ( cmd == CMD_STAT )
      {
         printStat();
      }
      else if ( cmd == CMD_AUTOSTAT )
      {
         autoStat = !autoStat;
      }
      else if ( cmd == CMD_CONFIG )
      {
         printConfiguration(); 
      }
      else if ( cmd == CMD_TEST )
      {
         showWelcome(); 
         // Force immediate refresh
         forceLCDRefresh( true );
      }
      else if ( cmd == CMD_TESTLEDS )
      {
          testEachGearLED();
      }
      else if ( cmd == CMD_SETCFG )
      {
          handleSetCfg( arg1, arg2 );           
      }
      else
      {
          Serial.println( MSG_SYNTAX_1 );
          Serial.println( MSG_SYNTAX_2 );
      }
}

/// ----------------------------------------------------------------------------------------------------
/// Setups the EEPROM configuration values - read, initialize/upgrade if needed
/// ----------------------------------------------------------------------------------------------------
void setupConfiguration()
{
    // Read configuration from EEPROM
    EEPROM_readAnything( EEPROM_CONFIG_MEMOFFSET, configuration );
    printConfiguration();
    
    if ( configuration.isValidConfig < EEPROM_STRUCT_VER || configuration.isValidConfig  > EEPROM_STRUCT_VER + 1000 )
    {       
        // First time configuration - set & write defaults to EEPROM
        configuration.isValidConfig = EEPROM_STRUCT_VER;
        configuration.temperatureMode = DEFAULT_TEMPERATURE_MODE;
     
        writeConfiguration();
    }
    else
    {
        // TODO: Check if upgrade to structure is needed
    }
}

/// ----------------------------------------------------------------------------------------------------
/// Writes the the current configuration values into the EEPROM storage
/// ----------------------------------------------------------------------------------------------------
void writeConfiguration()
{
    EEPROM_writeAnything( EEPROM_CONFIG_MEMOFFSET, configuration );        
}
    
/// ----------------------------------------------------------------------------------------------------
/// Prints the current configuration values
/// ----------------------------------------------------------------------------------------------------
void printConfiguration()
{
    Serial.println( "-- CONFIG -- " );
    Serial.print( "IsValidConfig (Raw Value): " ); Serial.println( configuration.isValidConfig );
    Serial.print( "IsValidConfig: " ); Serial.println( ( configuration.isValidConfig == 12345 ) ? "Y" : "N" );
    Serial.print( "temperatureMode Mode: " ); Serial.println( configuration.temperatureMode );
}

