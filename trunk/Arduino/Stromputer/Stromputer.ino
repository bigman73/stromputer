// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   V-Strom Mk1B - An extra display for a Suzuki DL-650 ("V-Strom") that adds the following functionality:
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
// []     0.14 - 12/9/2011, + All LED control was changed to use the LED Library,
// []                       + Add force LCD update every 15 seconds (workaround to LCD clearing screen from time to time)
// []                       + Fixed TimedAction to trigger immediately when sketch starts
// []     0.15 - 12/10/2011, + Changed custom Timer ISR to TimerOne library, Refactored all constants and variables to Stromputer.h header file
// []     0.16 - 12/10/2011, + Code refactoring, naming conventions
// []     0.17 - 12/10/2011 + Moved to Arduino 1.0 (.ino), Main loop slowed down to refersh on 4Hz, removed obsolete PCF8591 gear read logic
// []     0.18 - 12/13/2011 + Add Photo Cell read/Automatic LCD Backlight Adjustment
// []     0.19 - 12/24/2011 + Fixed forced refresh, lcd back light value now using average (for smoothing), Fixed temperature error handling
// []     0.20 - 12/25/2011 + LED Dimming, All LED pins changed to PWM
// []     0.21 - 12/26/2011 + Fixed minor bug - Neutral light was 'jumping' while light has been dimming.
// []     0.22 - 12/26/2011 + Adjusted real resistor values
// []     0.23 -   1/8/2012 + Added time action for checking forced LCD refresh, added serial messages when booting, Added initial Serial Input
// []     0.24 -   1/14/2012 + Added Serial Commands, removed serial debug
// []     0.25 -   1/14/2012 + Added Non-Volatile EEPROM configuration (remains after power is shut off, read when rebooting)
// []     0.26 -   1/21/2012 + Fixed I2C Error Handling, more EEPROM configurations
// []     0.27 -   1/22/2012 + Fixed some bugs - ISR was disabled on each serial processing, MILETONE: Successful 2ND deployment to V-Strom
// []     0.28 -   1/23/2012 + Fixed transient/fake Neutral
// []     **** Compatible with ARDUINO: 1.00 ****
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

#include <Wire.h>
#include <inttypes.h>

// -----------------    Library Includes    ------------------------
// Include 3rd Party library - LCD I2C NHD
// http://www.arduino.cc/playground/Code/LCDi2c
#include <LCDi2cNHD.h>                    

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
TimedAction serialInputTimedAction = TimedAction( 0, 100, processSerialInput );

/// --------------------------------------------------------------------------
/// Arduino one-time setup routine - i.e. Program entry point like main()
/// --------------------------------------------------------------------------
void setup() 
{ 
    // Setup Serial connection
    Serial.begin( SERIAL_SPEED );
    Serial.print( "------- Stromputer, Firmware version: " ); Serial.println( VERSION );

    onBoardLed.on();    
    
    setupConfiguration();

    if ( !initializeLCD() )
    {
        Serial.println( ">> ERROR: LCD failed to initialize" );
    }

    showWelcome();
   
    if ( !initializeDS1631() )
    {
        Serial.println( ">> ERROR: DS1631 failed to initialize" );
    }
    	
    // sets the digital pin of gear tacktile button as input
    #ifdef MANUAL_GEAR_EMULATION
    pinMode( MANUAL_GEAR_DOWN_PIN, INPUT );
    pinMode( MANUAL_GEAR_UP_PIN, INPUT );
    #endif
    
    // set a timer to 62.5 milliseconds (or 16Hz)
    Timer1.initialize( 1000 );  // 1000 microseconds = 1 msec = 1000hz
    Timer1.attachInterrupt( timerISR ); // attach the service routine here
 
    forceLCDRefresh( true );
     
    Serial.println( ">> Stromputer ON. Ready to Rock! <<" );
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

    // Handle gears at 20Hz
    if ( timerDivider % 50 == 1 ) 
    {
        // Handle gear position read
        handleGearPositionRead();

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
/// Handle reading the gear position, using one of the modes:
///   Emulation - Use breadboard tactile buttons
///   Analog    - Use analog-to-digital read from Motorcycles's Gear Position Sensor voltage
/// --------------------------------------------------------------------------
void handleGearPositionRead()
{
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
        readGearPositionAnalog();          
    #endif
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for battLevelTimedAction - 
///     Processes the battery level
/// --------------------------------------------------------------------------
void processBatteryLevel()
{   
    readBatteryLevelAnalog();
    
    // When 'live' on the motorcycle, there is a 0.9V difference between what Arduino samples and what a volt meter samples.
    // TODO: Figure out the difference, probably due to a diode somewhere
    // As a temporary workaround, 0.9V are added as a constant
    if ( battLevel > 1.5f )
        battLevel += 0.9f;
    
    printBatteryLevel();
}


float battLevel0 = 0; // last value
float battLevel1 = 0; // value before last value (1st oldest value)

/// ----------------------------------------------------------------------------------------------------
/// Reads the current battery level from the voltage divider circuit (4:1, 20V -> 5V), using Arduino ADC
/// ----------------------------------------------------------------------------------------------------
void readBatteryLevelAnalog()
{
    battReadError = true; // Assume read had errors, by default

    int value = analogRead( ANALOGPIN_BATT_LEVEL );    // read the input pin for Battery Level
    
    // Keep a moving time window of 3 readings
    float battLevel2 = battLevel1; // 2nd oldest value
    battLevel1 = battLevel0;

    battLevel0 = BATT_VOLT_DIVIDER * ARDUINO_VIN_VOLTS * ( value / 1024.0f );
    
    if ( battLevel1 == 0 || battLevel2 == 0 )
    {
        // Disable moving window, for initial readings until they stabilize    
        battLevel = battLevel0; 
    }
    else
    {
        // Calculate average voltage, using time window
        battLevel = ( battLevel0 + battLevel1 + battLevel2 ) / 3.0f;
    }
        
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
        char formattedBattLevel[5]; // DD.D + NULL => Maximum 5 characters
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
      
    lcd.setCursor(1,0);   
    lcd.print( battLevelValue );
}

/// --------------------------------------------------------------------------
/// Timed Action Event handler for photoCellTimedAction - 
///     Processes the photo cell level and adjust LCD Back Light accordingly
/// --------------------------------------------------------------------------
void processPhotoCell()
{   
    readPhotoCellAnalog();
   
    // Determine new LCD Back Light level (1..8, 1 is very dim .. 8 is very bright)
    if ( photoCellLevel < 50 )
        lcdBackLight = 1; // Very dim
    else if ( photoCellLevel < 250 )
        lcdBackLight = 2;
    else if ( photoCellLevel < 500 )
        lcdBackLight = 3;
    else if ( photoCellLevel < 700 )
        lcdBackLight = 4;
    else if ( photoCellLevel < 800 )
        lcdBackLight = 5;
    else if ( photoCellLevel < 900 )
        lcdBackLight = 6;
    else if ( photoCellLevel < 1000 )
        lcdBackLight = 7;
    else
        lcdBackLight = 8; // Very bright

    // Only update the LCD backlight if there is actually a change (to reduce costly I2C traffic)
    if ( lcdBackLight != lastLcdBackLight )
    {       
        // if a large change, use a smoothing function (average) to reduce sharp/large changes
        if ( abs( lcdBackLight - lastLcdBackLight ) > 1 )
            lcdBackLight = ( lcdBackLight + lastLcdBackLight ) / 2;
            
        lastLcdBackLight = lcdBackLight;
                  
        forceLedUpdate = true; // Force update of LEDs
         
        lcd.setCursor( 0, 4 );
        lcd.print( lcdBackLight );   

        lcd.setBacklight( lcdBackLight );
    }      
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
        
    gearPositionVolts = GEAR_VOLT_DIVIDER * GEAR_POSITION_VOLTS * ( value / 1024.0f );    // read the input pin for Gear Position
    determineCurrentGear();
    
    gearReadError = false; // Clear read error - we made it here
}

long neutralStartMillis = 0;

// ----------------------------------------------------------------------------------------------------
/// Determins the current gear from the gear position volts
/// ----------------------------------------------------------------------------------------------------
void determineCurrentGear()
{
     bool isNeutral = false;
     
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
     {
         isNeutral = true;
         
         // Start the neutral time count, but do NOT shift to N
         // Only after Neutral has been persistent long enough, let it go through
         //  This should eliminate transient 'Fake' Neutral readings, that occur between shifting of gears.
         if ( neutralStartMillis == 0 )
         {
             neutralStartMillis = millis();
         }
         // Now that Neutral has shown it is persistent, set the gear as N
         else if ( millis() - neutralStartMillis > 250)
         {
             gear = GEAR_NEUTRAL;  // "Real"/Stable Neutral
         }
     }
     else
         gear = GEAR_ERROR; // Default: Error
         
     // Reset the neutral start count for all *actual* gear shifts other than Neutral
     if ( !isNeutral && neutralStartMillis > 0 )
     {
         neutralStartMillis = 0; // Reset Neutral start time stamp
     }
}

/// ----------------------------------------------------------------------------------------------------
/// Print the current gear position (only if there has been a change)
/// ----------------------------------------------------------------------------------------------------
void printGearPosition()
{    
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    #ifdef DEBUG_PRINT_GEARVOLTS
    lcd.setCursor( 0, 6 );
    lcd.print( gearPositionVolts );
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
    lcd.setCursor( 0, 6 );
    lcd.print( GEAR_LABEL );      
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

    lcd.setCursor(1,6);
    lcd.print( gearValue );   
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
        byte factor;
        if ( lcdBackLight <=4 )
            // Night
            factor = 1;
        else
            // Day
            factor = 3;
        
        ledBrightnessGreen = 1 + lcdBackLight * factor; // Note: Green LED (1st Gear LED) is extremely bright even with very small currents/PWM duty cycle
        ledBrightnessYellow = 1 + lcdBackLight * factor * 4; // PWM 0..255 : 0%-100%
        ledBrightnessWhite = 1 + lcdBackLight * factor * 12; // PWM 0..255 : 0%-100%
        ledBrightnessBlue = 1 + lcdBackLight * factor * 6; // PWM 0..255 : 0%-100%
      
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
    readTemperature();
   
    // Workaround to a problem with DS1631: From time to time, the IC goes nuts and starts returning odd readings (TODO: Check if related to pull up resistor values, or breadboard, or the generic IC socket
    // Check if DS1631 is returning bad temperature vlaues (but not on boot last temperature is set to -99), if yes, re-establish communication with it
    if ( !temperatureReadError && lastTemperature > TEMPERATURE_MIN_VALID && abs( lastTemperature - temperature ) > TEMPERATURE_ERROR_DIFF )
    {
        // Re-Initialize DS1631 - Stop temperature conversion, and start it again
        initializeDS1631();
        
        // Now read temperature again
        readTemperature();
        
        // If still there are odd readings then declare temperature error mode
        if ( abs( lastTemperature - temperature ) > 30 )
        {
            temperatureReadError = true;
        }
    }

    printTemperature();   
}

/// ----------------------------------------------------------------------------------------------------
/// Reads the current temperature from the I2C DS1631 Thermometer 
/// ----------------------------------------------------------------------------------------------------
void readTemperature()
{   
    temperatureReadError = true; // Assume read had errors, by default
    
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
    temperature = _TH + _TL / 256.0;
      
    if ( configuration.temperatureMode == 'F' )
    {
        // Convert celsius to fahrenheight
        temperature = toFahrenheight( temperature );
    }
    
    temperatureReadError = false; // Clear read error - we made it here
}

float toFahrenheight( float celsius )
{
      return temperature * 9.0 / 5.0 + 32;
}

float toCelsius( float fahrenheight )
{
      return ( temperature - 32 ) * 5.0 / 9.0;
}


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
        if ( !temperatureReadError && ( CompareFloats( lastTemperature, temperature ) ) )
        {
            return;
        }
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
    else if ( temperature < -55 )
    {
        // Workaround when there are odd readings on first seconds when temperature sensor is starting up
        temperatureValue = " ----";
    }
    else
    {   
        // Pad one space, align to right if temperature is two digits (left of dot)
        if ( ( temperature > 10 && temperature < 100 ) || ( temperature < 0 && temperature > -10 ) )
            temperatureValue += " " ;
        // Pad two spaces, align to right if temperature is one digit (left of dot)        
        else if ( temperature > 0 && temperature < 10 )
            temperatureValue += "  " ;
    
        // format temperature into a fixed .1 format (e.g. 62.5 or 114.4 [too hot to ride! :) ] or -10.7 [too cold to ride! :) ])
        char formattedTemperature[7]; // DDD.DD + NULL => Maximum 7 characters
        formatFloat( formattedTemperature, temperature, 1 );
        temperatureValue += formattedTemperature;
        
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
    
    lcd.setCursor( 1, 10 );
    lcd.print( temperatureValue );
}


/// ----------------------------------------------------------------------------------------------------
/// Formats a float to a string. A workaround to sprintf not working on arduino (produces "?")
/// Based on source: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1164927646
/// ----------------------------------------------------------------------------------------------------
char *formatFloat( char *buffer, double floatValue, int precision )
{
  static long precisionLongs[] = {0,10,100,1000,10000};
  
  // Keep the start of the output buffer
  char *result = buffer;
  
  // write the integer part
  long intPart = (long) floatValue;
  itoa(intPart, buffer, 10);
  while (*buffer != '\0') buffer++;

  // Append dot
  *buffer++ = '.';

  // Append the fractional part with the requesed preceision
  long decimals = abs((long)( (floatValue - intPart) * precisionLongs[ precision ] ));
  itoa(decimals, buffer, 10);
  
  // Return the result buffer (pointer)
  return result;
}
 
 
#ifdef PCF8591_DAC_GEAR_EMULATOR
 
/// ------------------------------------------------------------
/// Controls the PCF8591 IC through I2C protocol
/// Arguments:
///    dac_value - A byte value (0..255) that would be converted to analog voltage level (VSS..VREF, typically 0..5V)
///    adc_values - An output array of bytes where the results of ADC conversion will be stored, per channel
///    adcChannelMask - A mask for the ADC Input channel mask
/// ------------------------------------------------------------
int controlPCF8591_I2C(byte dac_value, byte adc_values[], byte adcChannelMask )
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
                  Wire.write( PCF8591_DAC_SINGLECHANNEL_MODE | (byte) inputChannel );
                      
                  // Only send the dac value once (AOUT)
                  if ( dacValueSent == 0 )
                  {
                      Wire.write(dac_value);
                      dacValueSent = 1;
                  }
                  Wire.endTransmission();
                  
                  delay(1);
                  
                  Wire.requestFrom( (int) PCF8591_I2C_ADDRESS, 2 );
                  if (Wire.available()) 
                  {
                    Wire.read();  // last value -> just read but ignore
                  }
                  else
                      return 0;
                  
                  if (Wire.available())
                  {
                    adc_values[inputChannel] = Wire.read();
                  }
                  else
                      return 0;
                  

                  delay(1);
            }
    } // for
    
    return 1;
}    
#endif

/// ------------------------------------------------------------
/// Tests the gear LEDs
/// ------------------------------------------------------------
void testGearLEDs()
{
    Serial.println( ">> Test Gear LEDs" );
    
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

/// --------------------------------------------------------------------------
/// Initialize the NHD LCD (I2C IC)
/// --------------------------------------------------------------------------
bool initializeLCD()
{   
    Serial.println( ">> LCD Initializing.." );
    // Initialize the display, clears the display
    if ( !lcd.init() )
    {
        return false;
    }

    // Set initial LCD backlight & contrast
    lcd.setBacklight( lcdBackLight );
    lcd.setContrast( lcdContrast );

    Serial.println( ">> LCD Initialized" );
    lcdInitialized = true;
    
    return true;
}

/// --------------------------------------------------------------------------
/// Initialize the DS 1631 Temperature Sensor (I2C IC)
/// --------------------------------------------------------------------------
bool initializeDS1631()
{
  Serial.println( ">> DS1631 Initializing.." );
  
   // Stop conversion to be able to modify "Access Config" Register
  Wire.beginTransmission( DS1631_I2C_ADDRESS );
  Wire.write((int)( DS1631_I2C_COMMAND_STOP_CONVERT )); // Stop conversion
  if ( Wire.endTransmission() )
  {
    Serial.println( ">> Error: Stop Convert command failed" );
    return false; // Error
  }
    
  // READ "Access Config" register
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.write((int)( DS1631_I2C_COMMAND_ACCESS_CONFIG ));
  if ( Wire.endTransmission() )
  {
      Serial.println( ">> Error: Access Config command failed" );
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
      Serial.println( ">> Error: Access Continous Control Byte command failed" );
      return false; // Error
  }

  // START conversion to get T°
  Wire.beginTransmission(DS1631_I2C_ADDRESS);
  Wire.write((int) DS1631_I2C_COMMAND_START_CONVERT); // Start Conversion
  if ( Wire.endTransmission() )
  {
      Serial.println( ">> Error: Start Convert command failed" );
      return false; // Error
  }
  
  Serial.println( ">> DS1631 Initialized" );
  
  return true; // Success
}

/// ----------------------------------------------------------------------------------------------------
/// Show the welcome sequence
/// ----------------------------------------------------------------------------------------------------
void showWelcome()
{
    #ifdef SHOW_WELCOME    
    
    testGearLEDs();

    String line2 = String(Welcome1_Line2);
    line2 += VERSION;
    printWelcomeScreen(String(Welcome1_Line1), line2, 800, 25, DIRECTION_RIGHT );
   
    #endif    
}

/// ----------------------------------------------------------------------------------------------------
/// Prints the welcome screen using the given two lines arguments
/// ----------------------------------------------------------------------------------------------------
void printWelcomeScreen( String line1, String line2, int showDelay, int scrollDelay, char scrollDirection )
{
    Serial.println( ">> Show Welcome - BEGIN.." );     

    lcd.clear();
    
    // Print line 1
    lcd.setCursor( 0, 0 );
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
    
    Serial.println( ">> Show Welcome - END" );
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
        Serial.print( "Recieved: " ); Serial.println( serialCommandLine );
        
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
/// Handle a single command 
/// ----------------------------------------------------------------------------------------------------
void handleCommand( String cmd, String arg1, String arg2 )
{
        if ( cmd == "ALIVE" )
        {
            Serial.println( "Yes, I'm here" );
        }
        else if ( cmd == "STAT" )
        {
            Serial.print( "Temp. = " );
            Serial.print( temperature );
            Serial.println( configuration.temperatureMode  );

            Serial.print( "Gear = " );
            if ( gear == 0 )
                Serial.println( "N" );
            else
                Serial.println( gear );
            Serial.print( "GearPos = " ); Serial.print( gearPositionVolts ); Serial.println( "V" );

            Serial.print( "Batt. Level = " );
            Serial.print( battLevel ); Serial.println( "V" );

            Serial.print( "LCD Initialized: " ); Serial.println( lcdInitialized ) ;
            Serial.print( "LCD Back Light = " ); Serial.println( lcdBackLight );
            Serial.print( "LCD Contrast = " ); Serial.println( lcdContrast );
            Serial.print( "Photo Cell = " ); Serial.println( photoCellLevel ); 
            // Important Note: for some unknown reason, the serial printing cannot be too large. It seems that the Serial buffer size is limited. Flush() doesn't do what its supposed to do.
            //       Keep it short, or maybe add some delay to let the stream clear itself.
        }
        else if ( cmd == "CONFIG" )
        {
           printConfiguration(); 
        }
       
        else if ( cmd == "TEST" )
        {
           showWelcome(); 
           // Force immediate refresh
           forceLCDRefresh( true );
        }
       
        else if ( cmd == "SETCFG" )
        {
            if ( arg1.length() == 0 )
                Serial.println( "Syntax error: arg1" );
            else if ( arg2.length() == 0 )
                Serial.println( "Syntax error: arg2" );
            else            
            {
                if ( arg1 == "TEMP" )
                {
                     char tempMode = arg2[0];
                     if ( configuration.temperatureMode != tempMode )
                     {
                        configuration.temperatureMode = tempMode;
                        // Recalc last temperature to avoid restarting DS1631
                        if ( tempMode == 'F' )
                            lastTemperature = toFahrenheight( temperature ); 
                        else
                           lastTemperature = toCelsius( temperature );
                        
                        writeConfiguration();
                        
                        isForceRefreshTemp = true;
                        temperatureTimedAction.force();

                        Serial.print( "Temp mode set to: " ); Serial.println( tempMode );
                        
                    }
                    else
                    {
                        Serial.print( "Nothing done" );
                    }           
                }
                else
                {
                    Serial.print( "Unkonwn token2: " ); Serial.println( arg1 );
                }
            }
        }
        else
        {
            Serial.println( "SYNTAX: CMD [ARG1] [ARG2];" );
            Serial.println( "   CMD = {ALIVE | TEST | STAT | SETCFG}" );
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

