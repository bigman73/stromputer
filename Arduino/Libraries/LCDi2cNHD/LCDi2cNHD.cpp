
#include <Wire.h>

#include <inttypes.h>

// **************************************************************************
// 12/11/2011, YUVAL NAVEH - Patched to compile with Arduino 1.0, 
//                            as well as older Arudino (e.g. 0023)
// **************************************************************************
#if (ARDUINO >= 100)
	#include <Arduino.h>
	#define __Wire_write Wire.write
#else
	#include "WConstants.h"  //all things wiring / arduino
	#define __Wire_write Wire.send
#endif
// **************************************************************************
// **************************************************************************
  
#include "LCDi2cNHD.h"


#define _LCDEXPANDED				// If defined turn on advanced functions

// Global Vars 

uint8_t g_num_lines = 4;
uint8_t g_num_col = 20;
int g_i2caddress = 0x50 >> 1; // NHD documents address in 8-bit but arduino uses 7
uint8_t g_display = 0;
int g_cmdDelay = 50;
int g_charDelay = 0;



// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	LCDi2c Class
// []
// []	num_lines = 1-4
// []   num_col = 1-80
// []   i2c_address = 7 bit address of the device
// []   display = Not used at this time
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
//--------------------------------------------------------



LCDi2cNHD::LCDi2cNHD (uint8_t num_lines,uint8_t num_col,int i2c_address,uint8_t display){
	
	g_num_lines = num_lines;
	g_num_col = num_col;
	g_i2caddress = i2c_address;
	g_display = display;	
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	initiatize lcd after a short pause
// []
// []	Init the i2c buss
// []   Put the display in some kind of known mode
// []   Put the cursor at 0,0
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


bool LCDi2cNHD::init () {
	
	Wire.begin();
	// TODO: Check that LCD was initialized properly. on() should return a value, command() should return a value and check endTranmission value, a global flag (initializeError) should be added
	if ( !on() )
	{
		return false;  // error
	}
	
	clear();
	blink_off();
	cursor_off(); 
	home();
	
	return true; // success
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Over ride the default delays used to send commands to the display
// []
// []	The default values are set by the library
// []   this allows the programer to take into account code delays
// []   and speed things up.
// []   
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDi2cNHD::setDelay (int cmdDelay,int charDelay) {
	
	g_cmdDelay = cmdDelay;
	g_charDelay = charDelay;
	
}




// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   Send a command to the display. 
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


bool LCDi2cNHD::command(uint8_t value) {

  Wire.beginTransmission(g_i2caddress);
  __Wire_write(0xFE);
  __Wire_write(value);
  if ( Wire.endTransmission() ) 
	return false; // error
  delay(g_cmdDelay);
  
  return true; // success
}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   Send a command to the display. 
// []
// []	This is also used by the print, and println
// []	
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


// **************************************************************************
// 12/12/2011, YUVAL NAVEH - Patched to compile with Arduino 1.0, 
//                            as well as older Arudino (e.g. 0023)
// **************************************************************************
#if (ARDUINO >= 100)

size_t LCDi2cNHD::write(uint8_t value) {

  Wire.beginTransmission(g_i2caddress);
  __Wire_write(value);
  Wire.endTransmission();
  delay(g_charDelay);

  return 0;
}
#else

void LCDi2cNHD::write(uint8_t value) {

  Wire.beginTransmission(g_i2caddress);
  __Wire_write(value);
  Wire.endTransmission();
  delay(g_charDelay);
}

#endif
// **************************************************************************
// **************************************************************************




// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Clear the display, and put cursor at 0,0 
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDi2cNHD::clear(){
	
      command(0x51);
 
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Home to custor to 0,0
// []
// []	Do not disturb data on the displayClear the display
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDi2cNHD::home(){

	setCursor(0,0);					// The command to home the cursor does not work on the version 1.3 of the display
}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn on the display
// []
// []	Depending on the display, might just turn backlighting on
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

bool LCDi2cNHD::on(){
	if ( !command(0x41) )
		return false;
		
	return true;

}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn off the display
// []
// []	Depending on the display, might just turn backlighting off
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDi2cNHD::off(){

      command(0x42);

        
}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn on the underline cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDi2cNHD::cursor_on(){


      command(0x47);

        
}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn off the underline cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDi2cNHD::cursor_off(){

      command(0x48);
}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn on the blinking block cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDi2cNHD::blink_on(){

      command(0x4B);

}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn off the blinking block cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDi2cNHD::blink_off(){

      command(0x4C);
 
}


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Position the cursor to position line,column
// []
// []	line is 0 - Max Display lines
// []	column 0 - Max Display Width
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDi2cNHD::setCursor(uint8_t line_num, uint8_t x){

      uint8_t base = 0x00;
      if (line_num == 1)
          base = 0x40;
      if (line_num == 2)
          base = 0x14;
      if (line_num == 3)
          base = 0x54;
      Wire.beginTransmission(g_i2caddress);
      __Wire_write(0xFE);
      __Wire_write(0x45);
      __Wire_write(base + x);
      Wire.endTransmission();
      delay(g_cmdDelay*2);

}

#ifdef _LCDEXPANDED


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Return the status of the display
// []
// []	Does nothing on this display
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]	

uint8_t LCDi2cNHD::status(){
	
	return 0;
}





// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Load data for a custom character
// []
// []	Char = custom character number 0-7
// []	Row is array of chars containing bytes 0-7
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDi2cNHD::load_custom_character(uint8_t char_num, uint8_t *rows)
{


	Wire.beginTransmission(g_i2caddress);
	__Wire_write(0xFE);
	__Wire_write(0x54);
	__Wire_write(char_num);
	for (uint8_t i = 0 ; i < 8 ; i++)
		__Wire_write(rows[i]);
	Wire.endTransmission();
	delay(g_cmdDelay);
}




void LCDi2cNHD::setBacklight(uint8_t new_val)
{
	
	Wire.beginTransmission(g_i2caddress);
	__Wire_write(0xFE);
	__Wire_write(0x53);
	__Wire_write(new_val);
	Wire.endTransmission();
	delay(g_cmdDelay);

}


void LCDi2cNHD::setContrast(uint8_t new_val)
{
	
	Wire.beginTransmission(g_i2caddress);
	__Wire_write(0xFE);
	__Wire_write(0x52);
	__Wire_write(new_val);
	Wire.endTransmission();
	delay(g_cmdDelay);
}


#endif
