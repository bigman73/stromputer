void setup()
{
   // Setup Serial connection
    Serial.begin( 38400 );
    // http://www.openmusiclabs.com/learning/digital/atmega-adc/
    DIDR0 = 0X0F;
    
    digitalWrite(A3, LOW);  // Disable pullup on analog pin 3
}

void loop()
{
  
  int value = analogRead(A3);
  Serial.print( "A3 = " );
  Serial.println(value);

  
 /* 
  Reading VCC is killing the measurement!!!! It maybe turns on an internal pull up resistor, but the values gets completely skewed
 
   float vcc = readVcc();
  Serial.print( "VCC(V) = ");
  Serial.println(vcc);
  digitalWrite(A3, LOW);  // Disable pullup on analog pin 3
  delay( 10 );*/
  
/*  Serial.print( "A3(V) = ");
  float a3Volts = vcc *( value / 1023.0f );
  Serial.println( a3Volts );
*/

  float a3Volts = 5.0f *( value / 1023.0f );
  
  Serial.print( "Actual V = ");
  Serial.println(a3Volts * 2.0); // 2.0 - voltage divider on A3
  Serial.println("-----------");

  delay(250);
}

float readVcc() 
{
    long vccValue;
    // Read 1.1V reference against AVcc
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delay(50); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Convert
    while (bit_is_set(ADCSRA,ADSC));
    vccValue = ADCL;
    vccValue |= ADCH<<8;
    vccValue = 1126400L / vccValue; // Back-calculate AVcc in mV
  
    // Convert mV integer to V (float)
    float result = vccValue / 1000.0f;
    // Constrain VCC between 0 .. 5.5
    result = constrain( result, 0.01f, 5.5f );
    
    return result;
}

