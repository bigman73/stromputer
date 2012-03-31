// 
//    FILE: RunningAverage.cpp
//  AUTHOR: Rob Tillaart
// VERSION: 0.1.01
// PURPOSE: RunningAverage library for Arduino
//
// The library does store the last N individual values, to 
// calculate the running average. 
//
// HISTORY: 
// 0.1.00 - 2011-01-30 initial version
// 0.1.01 - 2011-02-28 fixed missing destructor in .h
// 0.2.00 - 2012-23-20 Yuval Naveh
//						Modified to become a proper library (examples, keywords)
//						Changed method names to be clearer
//						Added new Method: trimToValue
//
// Released to the public domain
//

#include "RunningAverage.h"
#include <stdlib.h>

// Constructor
RunningAverage::RunningAverage(int windowSize)
{
	_size = windowSize;
	_ar = (float*) malloc(_size * sizeof(float));
	clear();
}
RunningAverage::~RunningAverage()
{
	free(_ar);
}

// resets all counters
void RunningAverage::clear() 
{ 
	_cnt = 0;
	_idx = 0;
	_sum = 0.0;
	for (int i=0; i< _size; i++) _ar[i] = 0.0;
}

// adds a new value to the data-set
void RunningAverage::addValue(float value)
{
	_sum -= _ar[_idx];
	_ar[_idx] = value;
	_sum += _ar[_idx];
	_idx = (_idx + 1) % _size;
	if (_cnt < _size) _cnt++;
}

// returns the average of the data-set added so far
float RunningAverage::getAverage()
{
	if (_cnt == 0) return 0;
		return _sum / _cnt;
}

void RunningAverage::trimToValue(float value)
{
	clear();
	addValue(value);
}
