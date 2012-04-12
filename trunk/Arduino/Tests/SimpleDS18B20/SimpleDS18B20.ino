#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 4 on the Arduino
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup(void)
{
  // start serial port
  Serial.begin(38400);
  Serial.println("Dallas Temperature IC Control Library Demo");

 // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(13, OUTPUT);     
  
  // Start up the library
  sensors.begin();
}

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
  Serial.print("Device count = " );
  Serial.println( sensors.getDeviceCount() );
  
  Serial.print("Temperature for the device 1 (index 0) is: ");
  float tempValue = sensors.getTempCByIndex(0);
  if ( tempValue == DEVICE_DISCONNECTED )
  {
    Serial.println( "ERROR: DEVICE DISCONNECTED" );
  }
  else
  {
    Serial.print( tempValue ); 
    Serial.println("C");
  }

 
  digitalWrite(13, LOW);    // set the LED off
 
  digitalWrite(13, HIGH);   // set the LED on  
  delay( 500 );
}
