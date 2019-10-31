/*
 * tools.cpp
 *
 *  Created on: 13 рту. 2019 у.
 *      Author: Alex
 */

#include "tools.h"

// Arduino IDE map() function: maps the value from v_ interval to r_ interval
int32_t map(int32_t value, int32_t v_min, int32_t v_max, int32_t r_min, int32_t r_max) {
	if (v_min == v_max) return r_min;
	int32_t round = (v_max - v_min) >> 1;
	return ((value - v_min) * (r_max - r_min) + round) / (v_max - v_min) + r_min;
}

// Calculate non-linear power gauge. Translate p_middle percentage to gauge middle interval
uint8_t gauge(uint8_t percent, uint8_t p_middle, uint8_t g_max) {
	if (percent > 100)	percent = 100;
	if (p_middle > 70)	p_middle = 70;
	if (g_max < 10)		g_max = 10;
	if (percent <= p_middle) {
		return map(percent, 0, p_middle, 0, g_max >> 1);
	} else {
		return map(percent, p_middle+1, 100, (g_max >> 1)+1, g_max);
	}
}

// Arduino constrain() function: limits the value inside the required interval
int32_t constrain(int32_t value, int32_t min, int32_t max) {
	if (value < min)	return min;
	if (value > max)	return max;
	return value;
}

//Convert integer Celsius temperature to the Fahrenheit
int16_t celsiusToFahrenheit(int16_t cels) {
	return (cels *9 + 32*5 + 2)/5;
}

// Convert integer Fahrenheit temperature to the Celsius
int16_t fahrenheitToCelsius(int16_t fahr) {
	return (fahr - 32*5 + 5) / 9;
}
