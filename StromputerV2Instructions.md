# **Building Instructions for Stromputer V2** #

# Introduction #

Stromputer V2 will be an improved design (i.e. evolution, not re-design) of Stromputer V1.
It focuses on the following factors:
1. Reduced cost
2. Simpler design, less components
3. Smaller form factor

# Install Arduino #
  1. Download Arduino IDE (1.0+, recommended 1.0.3)
http://arduino.cc/en/Main/Software
  1. Unzip the zip into a local folder - e.g. C:\Tools\arduino-1.0.3)

# Source Code Compiling #

  1. Checkout the code from Google Code: http://code.google.com/p/stromputer/source/checkout, into a local folder (e.g. C:\Development\Stromputer)
  1. Delete the Arduino library LiquidCrystal (e.g. C:\Tools\arduino-1.0.3\libraries\LiquidCrystal)
  1. Copy all Stromputer 3rd Party libraries (C:\Development\stromputer\Arduino\Libraries) into Arduino folder (C:\Tools\arduino-1.0.3\libraries)
  1. Run Arduino IDE
  1. Open Stromputer sketch file (C:\Development\stromputer\Arduino\Stromputer\Stromputer.ino)
  1. Compile all code by clicking on the Verify button

## Expected result ##
Code should compile without errors ("Done Compiling."), the last line in the output should be similar to this one, numbers may vary:<br />
**Binary sketch size: 23,688 bytes (of a 30,720 byte maximum)**