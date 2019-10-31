/*
 * pid.cpp
 *
 *  Created on: 13 aug 2019
 *      Author: Alex
 */

#include "pid.h"
#include "tools.h"
#include <math.h>

PIDparam::PIDparam(int32_t Kp, int32_t Ki, int32_t Kd) {
	this->Kp	= Kp;
	this->Ki	= Ki;
	this->Kd	= Kd;
}

PIDparam::PIDparam(const PIDparam &p) {
	Kp	= p.Kp;
	Ki	= p.Ki;
	Kd	= p.Kd;
}

void PID::load(const PIDparam &p) {
	Kp	= p.Kp;
	Ki	= p.Ki;
	Kd	= p.Kd;
}

void PID::init(uint8_t denominator_p) {							// PID parameters are initialized from EEPROM by  call
	Kp	= 10;
	Ki	= 10;
	Kd  = 0;
	this->denominator_p = denominator_p;
}

void PID::resetPID(void) {
	temp_h0 		= 0;
	temp_h1 		= 0;
	power  			= 0;
	i_summ 			= 0;
}

int32_t PID::changePID(uint8_t p, int32_t k) {
	switch(p) {
    	case 1:
    		if (k >= 0) Kp = k;
    		return Kp;
    	case 2:
    		if (k >= 0) Ki = k;
    		return Ki;
    	case 3:
    		if (k >= 0) Kd = k;
    		return Kd;
    	default:
    		break;
	}
	return 0;
}
/*
 * Ku = 4 * delta_power / (PI * SQRT(alpha^2-epsion^2), where
 * diff = alpha^2-epsion^2,
 * alpha - amplitude of temperature oscillation
 * epsilon - hysteresis (delta_temp)
 *
 * Pu = period - the oscillation period, ms
 * Kp = 0.6*Ku; Ti = 0.5*Pu; Td = 0.125*Pu;
 * Ki = Kp*T/Ti;
 * Kd = Kp*Td/T;
 */
void PID::newPIDparams(uint16_t delta_power, uint32_t diff, uint32_t period) {
	uint32_t T = 50;										// Check IRON period, ms
	double Ku  = 4 * delta_power;
	Ku /= M_PI * sqrt(diff);
	uint32_t denominator = 1 << denominator_p;
	Kp = round(Ku * 0.6 * denominator);						// Translate Kp to the numerator of implemented PID
	Ki = (Kp * T * 2 + period/2) / period;
	Kd = (Kp * period) >> 3;								// 1/8
	Kd += T/2;
	Kd /= T;
	/*
	 *  The algorithm gives very big values for Kd (about 39 -> 39*2048=79892)
	 *  The big values of Kd gives us the big power dispersion
	 *  That is why it is better to limit the Kd value.
	 */
	Kd = constrain(Kd, 0, 10000);
}

int32_t PID::reqPower(int16_t temp_set, int16_t temp_curr) {
	if (temp_h0 == 0) {										// Use direct formulae because do not know previous temperature
		power 		= 0;
		i_summ 		= 0;
		i_summ += temp_set - temp_curr;
		power = Kp*(temp_set - temp_curr) + Ki * i_summ;
	} else {
		int32_t kp = Kp * (temp_h1 	- temp_curr);
		int32_t ki = Ki * (temp_set	- temp_curr);
		int32_t kd = Kd * (temp_h0 	+ temp_curr - 2 * temp_h1);
		int32_t delta_p = kp + ki + kd;
		power += delta_p;									// Power is stored multiplied by denominator!
	}
	temp_h0 = temp_h1;
	temp_h1 = temp_curr;
	int32_t pwr = power + (1 << (denominator_p-1));			// prepare the power to divide by denominator, round the result
	pwr >>= denominator_p;									// divide by the denominator
	return pwr;
}

void PIDTUNE::start(uint16_t base_pwr, uint16_t delta_power, uint16_t base_temp, uint16_t delta_temp) {
	if (base_pwr && delta_power) {
		this->base_power	= base_pwr;						// The power required to keep the preset temperature
		this->delta_power	= delta_power;					// Apply +- delta power in relay method
		this->base_temp		= base_temp;
		this->delta_temp	= delta_temp;
		app_delta_power		= false;
		pwr_change			= 0;
		loops				= 0;
		period.reset();
		temp_min.reset();
		temp_max.reset();
	}
}

uint16_t PIDTUNE::run(uint32_t t) {
	if (app_delta_power) {									// Applying extra power
		if (check_min && (int16_t)t > base_temp) {			// Finish looking for minimum temperature
			check_min = false;
			temp_min.update(t_min);
		}
		if ((int16_t)t > base_temp + delta_temp) {			// Crossed high temperature limit, decrease the power
			app_delta_power = false;
			if (pwr_change > 0) {
				period.update(HAL_GetTick() - pwr_change);
				pwr_change = HAL_GetTick();
				++loops;
			} else {
				pwr_change = HAL_GetTick();
			}
			check_min	= false;							// Be paranoid
			check_max	= true;
			t_max		= t;
		}
	} else {												// Returning to preset temperature
		if (check_max && (int16_t)t < base_temp) {			// Finish looking for maximum temperature
			check_max = false;
			temp_max.update(t_max);
		}
		if ((int16_t)t < base_temp - delta_temp) {			// Crossed low temperature limit, increase the power
			app_delta_power = true;
			check_max	= false;							// Be paranoid
			check_min	= true;
			t_min		= t;
		}
	}
	if (check_max && t > t_max)	t_max = t;					// Update maximum temperature of this cycle
	if (check_min && t < t_min) t_min = t;					// Update minimum temperature of this cycle
	uint16_t p = base_power;
	if (app_delta_power) p += delta_power; else	p -= delta_power;
	return p;
}
