/*
 * gun.h
 *
 *  Created on: 14 aug 2019
 *      Author: Alex
 */

#ifndef GUN_H_
#define GUN_H_

#include "stat.h"
#include "pid.h"
#include "tools.h"
#include "unit.h"

#define FAN_TIM		htim2
extern TIM_HandleTypeDef FAN_TIM;

class HOTGUN : public UNIT {
    public:
		typedef enum { POWER_OFF, POWER_HEATING, POWER_ON, POWER_FIXED, POWER_COOLING, POWER_PID_TUNE } PowerMode;
        HOTGUN(void) : h_power(hot_gun_hist_length), h_temp(hot_gun_hist_length) { }
        void        		init(void);
		virtual bool		isOn(void)						{ return (mode == POWER_ON || mode == POWER_FIXED); }
		virtual uint16_t	presetTemp(void)				{ return temp_set; 								}
		virtual uint16_t 	averageTemp(void)               { return avg_sync_temp; 						}
        virtual	uint16_t    getMaxFixedPower(void)			{ return max_fix_power; 						}
        virtual bool		isCold(void)					{ return h_temp.read() < temp_gun_cold;			}
        virtual uint16_t	pwrDispersion(void)            	{ return d_power.read(); 						}
		virtual void		setTemp(uint16_t temp)			{ temp_set	= constrain(temp, 0, int_temp_max);	}
		virtual uint16_t 	tmpDispersion(void)				{ return d_temp.read(); 						}
		virtual uint16_t	avgPower(void)					{ return avgPowerPcnt();						}
		uint16_t			presetFan(void)					{ return fan_speed;								}
		bool				isFanWorking(void)				{ return (fanSpeed() >= min_fan_speed);			}
        uint16_t			maxFanSpeed(void)				{ return max_fan_speed;							}
		void				setFan(uint16_t fan)			{ fan_speed = constrain(fan, min_working_fan, max_fan_speed);	}
		void				fanFixed(uint16_t fan)			{ TIM2->CCR2 = constrain(fan, 0, max_fan_speed);}
        void				updateTemp(uint16_t value);
		uint16_t			alternateTemp(void);			// Current temperature or 0 if cold
        virtual void        switchPower(bool On);
        virtual uint8_t		avgPowerPcnt(void);
		uint16_t			appliedPower(void);
		uint16_t			fanSpeed(void);					// Fan supplied to Fan, PWM duty
        virtual void        fixPower(uint16_t Power);		// Set the specified power to the the hot gun
		uint8_t				presetFanPcnt(void);
		uint16_t    		power(void);					// Required Hot Air Gun power to keep the preset temperature
	protected:
		void				safetyRelay(bool activate);
		volatile 	uint8_t		relay_ready_cnt	= 0;		// The relay ready counter, see HOTHUN::power()
    private:
		void		shutdown(void);
		PowerMode	mode				= POWER_OFF;
		uint8_t    	fix_power			= 0;				// Fixed power value of the Hot Air Gun (or zero if off)
		bool		chill				= false;			// Chill the Hot Air gun if it is over heating
		bool		reach_cold_temp		= true;				// Flag indicating the Hot Air Gun has reached the 'temp_gun_cold' temperature
		uint16_t	temp_set			= 0;				// The preset temperature of the hot air gun (internal units)
		uint16_t	fan_speed			= 0;				// Preset fan speed
		uint32_t	fan_off_time		= 0;				// Time when the fan should be powered off in cooling mode (ms)
		EMP_AVERAGE	h_power;								// Exponential average of applied power
		EMP_AVERAGE	h_temp;									// Exponential average of Hot Air Gun temperature
		EMP_AVERAGE	d_power;								// Exponential average of power dispersion
		EMP_AVERAGE d_temp;									// Exponential temperature math dispersion
		EMP_AVERAGE	zero_temp;								// Exponential average of minimum (zero) temperature
		volatile    uint16_t	avg_sync_temp	= 0;		// Average temperature synchronized with TIM1 (used to calculate required power, see power() method)
        const       uint8_t     max_fix_power 	= 70;
		const		uint8_t		max_power		= 99;
		const		uint16_t	min_fan_speed	= 600;
		const		uint16_t	max_fan_speed	= 1999;
		const		uint16_t	max_cool_fan	= 1600;
		const		uint16_t	min_working_fan	= 800;
        const       uint16_t    temp_gun_cold   = 100;		// The temperature of the cold Hot Air Gun
        const		uint32_t	fan_off_timeout	= 6*60*1000;// The timeout to turn the fan off in cooling mode
        const		uint32_t	fan_extra_time	= 60000;	// Extra time to wait after the Hot Air Gun reaches the 'temp_gun_cold' temperature
		const		uint16_t	fan_off_value	= 500;
		const 		uint16_t	fan_on_value	= 1000;
		const 		uint8_t		sw_off_value	= 30;
		const 		uint8_t		sw_on_value		= 60;
		const 		uint8_t		sw_avg_len		= 10;
        const		uint32_t	relay_activate	= 1;		// The relay activation delay (loops of TIM1, 1 time per second)
		const		int32_t		stable			= 300000;	// The power value when the Hot Gun reaches the preset temperature. Used in PID::pidStable()
};

#endif
