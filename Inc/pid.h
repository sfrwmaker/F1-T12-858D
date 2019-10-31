/*
 * pid.h
 *
 *  Created on: 13 рту. 2019 у.
 *      Author: Alex
 */

#ifndef _PID_H
#define _PID_H

#include "main.h"
#include "stat.h"
#include "vars.h"

class PIDparam {
	public:
		PIDparam(int32_t Kp = 0, int32_t Ki = 0, int32_t Kd = 0);
		PIDparam(const PIDparam &p);
		int32_t	Kp					= 0;
		int32_t	Ki					= 0;
		int32_t	Kd					= 0;
};

/*  The PID algorithm 
 *  Un = Kp*(Xs - Xn) + Ki*summ{j=0; j<=n}(Xs - Xj) + Kd(Xn - Xn-1),
 *  Where Xs - is the setup temperature, Xn - the temperature on n-iteration step
 *  In this program the interactive formula is used:
 *    Un = Un-1 + Kp*(Xn-1 - Xn) + Ki*(Xs - Xn) + Kd*(Xn-2 + Xn - 2*Xn-1)
 *  With the first step:
 *  U0 = Kp*(Xs - X0) + Ki*(Xs - X0); Xn-1 = Xn;
 *  
 *  The default values of PID coefficients can be found in config.cpp
 */
class PID {
	public:
		PID(void) 											{ }
		void		load(const PIDparam &p);
		PIDparam	dump(void)								{ return PIDparam(Kp, Ki, Kd);	}
		void		init(uint8_t denominator_p = 11);
		void 		resetPID(void);        					// reset PID algorithm history parameters
		int32_t 	reqPower(int16_t temp_set, int16_t temp_curr);
		int32_t  	changePID(uint8_t p, int32_t k);    	// set or get (if parameter < 0) PID parameter
		void		newPIDparams(uint16_t delta_power, uint32_t diff, uint32_t period);
	private:
		void  		debugPID(int t_set, int t_curr, long kp, long ki, long kd, long delta_p);
		int16_t   	temp_h0			= 0;					// previously measured temperatures
		int16_t	  	temp_h1			= 0;
		int32_t  	i_summ			= 0;					// Ki summary multiplied by denominator
		int32_t  	power			= 0;					// The power iterative multiplied by denominator
		int32_t  	Kp 				= 10;					// The PID coefficients multiplied by denominator.
		int32_t     Ki 				= 10;
		int32_t		Kd				= 0;
		int16_t  	denominator_p	= 11;              		// The common coefficient denominator power of 2 (11 means 2048)
};

class PIDTUNE {
	public:
		PIDTUNE(void) : period(auto_pid_hist_length), temp_max(auto_pid_hist_length), temp_min(auto_pid_hist_length)		{ 	}
		void		start(uint16_t base_pwr, uint16_t delta_power, uint16_t base_temp, uint16_t delta_temp);
		uint16_t	run(uint32_t t);
		uint16_t	autoTuneLoops(void)						{ return loops; 						}
		uint32_t	autoTunePeriod(void)					{ return period.read();					}
		uint16_t	tempMin(void)							{ return temp_min.read();   			}
		uint16_t	tempMax(void)							{ return temp_max.read();   			}
	private:
		HIST		period;									// Average value of relay method oscillations period
		HIST		temp_max;								// Average value of maximum temperature
		HIST		temp_min;								// Average value of minimum temperature
		volatile	uint16_t	base_power		= 0;		// Base power value
		volatile 	uint16_t 	delta_power		= 0;		// PLUS delta power applied
		volatile	uint16_t	base_temp		= 0;		// Base temperature value
		volatile	uint16_t	delta_temp		= 0;		// The temperature limit (base_temp - delta_temp <= t <= base_temp + delta_temp)
		volatile	bool		app_delta_power	= false;	// Do apply delta power
		volatile	uint32_t	pwr_change		= 0;		// The time (ms) when tune extra power changed
		volatile	bool		check_max		= false;
		volatile	bool		check_min		= false;
		volatile 	uint16_t	t_max			= 0;
		volatile    uint16_t	t_min			= 0;
		volatile	uint16_t	loops			= 0;		// Whole tune oscillation loop count
};

#endif
