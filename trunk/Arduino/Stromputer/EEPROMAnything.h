#ifndef EEPROMAnything_H
#define EEPROMAnything_H

#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

// Source: http://arduino.cc/playground/Code/EEPROMWriteAnything


// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   Stromputer
/*
    Stromputer - Enhanced display for Suzuki V-Strom Motorcycles (DL-650, DL-1000, years 2004-2011)
    Copyright (C) 2011 Yuval Naveh

     >> This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License: by-nc-sa <<   
     http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode

    You are free:

      to Share — to copy, distribute and transmit the work
      to Remix — to adapt the work

    Under the following conditions:

      Attribution — You must attribute the work in the manner specified by the author or licensor (but not in any way that suggests that they endorse you or your use of the work).
      Noncommercial — You may not use this work for commercial purposes.
      Share Alike — If you alter, transform, or build upon this work, you may distribute the resulting work only under the same or similar license to this one.
*/

/*
 .::::::.:::::::::::::::::::..       ...     .        :::::::::::. ...    :::::::::::::::.,:::::: :::::::..   
;;;`    `;;;;;;;;'''';;;;``;;;;   .;;;;;;;.  ;;,.    ;;;`;;;```.;;;;;     ;;;;;;;;;;;'''';;;;'''' ;;;;``;;;;  
'[==/[[[[,    [[      [[[,/[[['  ,[[     \[[,[[[[, ,[[[[,`]]nnn]]'[['     [[[     [[      [[cccc   [[[,/[[['  
  '''    $    $$      $$$$$$c    $$$,     $$$$$$$$$$$"$$$ $$$""   $$      $$$     $$      $$""""   $$$$$$c    
 88b    dP    88,     888b "88bo,"888,_ _,88P888 Y88" 888o888o    88    .d888     88,     888oo,__ 888b "88bo,
  "YMmMY"     MMM     MMMM   "W"   "YMMMMMP" MMM  M'  "MMMYMMMb    "YmmMMMM""     MMM     """"YUMMMMMMM   "W" 
*/


// Write a template value into EEPROM address [ee]
template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}

// Read a template value from EEPROM address [ee]
template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}

#endif

