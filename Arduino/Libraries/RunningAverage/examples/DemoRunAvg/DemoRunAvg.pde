#include <RunningAverage.h>

RunningAverage myRA(20); 
int samples = 0;

void setup(void) 
{
	Serial.begin(115200);
	Serial.println("Demo RunningAverage lib");
	myRA.clear(); // explicitly start clean
}

void loop(void) 
{
	long rn = random(0, 100);
	myRA.addValue(rn/100.0);
	samples++;
	Serial.print("Running Average: ");
	Serial.println(myRA.getAverage(), 4);

	if (samples == 50)
	{
		// After 5 seconds, force a high value
		myRA.trimToValue(2); 
	}

	if (samples == 300)
	{
		samples = 0;
		myRA.clear();
	}
	
	delay(100);
}