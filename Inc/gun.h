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

class HOTGUN_HW {
	public:
		HOTGUN_HW(void)										{ }
		void		init(void);
		bool 		isGunReedOpen(void)						{ return sw_gun.status();					}	// TRUE if switch is open
		bool 		isGunConnected(void) 					{ return c_fan.status();					}
		int32_t		fanCurrent(void)						{ return c_fan.read();						}
		void		updateFanCurrent(uint16_t value)		{ c_fan.update(value);						}
		void		checkSWStatus(void);
	protected:
		void		safetyRelay(bool activate);
		void		pwrKeepRelay(bool activate);
		volatile 	uint8_t		relay_ready_cnt	= 0;		// The relay ready counter, see HOTHUN::power()
		volatile 	bool		keep_power		= false;	// Status of keep power relay
	private:
		uint32_t	check_sw					= 0;		// Time when check reed switch status (ms)
		SWITCH 		sw_gun;									// Hot Air Gun reed switch
		SWITCH 		c_fan;									// Hot Air Gun is connected switch
		const		uint16_t	fan_off_value	= 500;
		const 		uint16_t	fan_on_value	= 1000;
		const 		uint8_t		sw_off_value	= 30;
		const 		uint8_t		sw_on_value		= 60;
		const 		uint8_t		sw_avg_len		= 10;
        const		uint32_t	relay_activate	= 1;		// The relay activation delay (loops of TIM1, 1 time per second)
};

class HOTGUN : public HOTGUN_HW, public PID {
    public:
		typedef enum { POWER_OFF, POWER_ON, POWER_FIXED, POWER_COOLING } PowerMode;
        HOTGUN(void) : h_power(hot_gun_hist_length), h_temp(hot_gun_hist_length) { }
        void        init(void);
		bool		isOn(void)								{ return (mode == POWER_ON || mode == POWER_FIXED); }
		uint16_t	presetTemp(void)						{ return temp_set; 								}
		uint16_t	presetFan(void)							{ return fan_speed;								}
		uint16_t 	averageTemp(void)                  		{ return h_temp.read(); 						}
        uint8_t     getMaxFixedPower(void)					{ return max_fix_power; 						}
        bool		isCold(void)							{ return h_temp.read() < temp_gun_cold;			}
        bool		isFanWorking(void)						{ return (fanSpeed() >= min_fan_speed);			}
        uint16_t	maxFanSpeed(void)						{ return max_fan_speed;							}
        uint16_t	pwrDispersion(void)              		{ return d_power.read(); 						}
		void		setTemp(uint16_t temp)					{ temp_set	= constrain(temp, 0, int_temp_max);	}
		void		updateTemp(uint16_t value)				{ if (isGunConnected()) h_temp.update(value);	}
		void		setFan(uint16_t fan)					{ fan_speed = constrain(fan, min_working_fan, max_fan_speed);	}
		void		fanFixed(uint16_t fan)					{ TIM2->CCR2 = constrain(fan, 0, max_fan_speed);}
		uint16_t	alternateTemp(void);					// Current temperature or 0 if cold
        void        switchPower(bool On);
        uint8_t		avgPowerPcnt(void);
		uint16_t	appliedPower(void);
		uint16_t	fanSpeed(void);							// Fan supplied to Fan, PWM duty
        void        fixPower(uint8_t Power);				// Set the specified power to the the hot gun
		uint8_t		presetFanPcnt(void);
		uint16_t    power(void);							// Required Hot Air Gun power to keep the preset temperature
		void		hwPwrOff(void);							// Hardware power switch was off
    private:
		void		shutdown(void);
		PowerMode	mode				= POWER_OFF;
		uint8_t    	fix_power			= 0;				// Fixed power value of the Hot Air Gun (or zero if off)
		bool		chill				= false;			// Chill the Hot Air gun if it is over heating
		uint16_t	temp_set			= 0;				// The preset temperature of the hot air gun (internal units)
		uint16_t	fan_speed			= 0;				// Preset fan speed
		uint32_t	fan_off_time		= 0;				// Time when the fan should be powered off in cooling mode (ms)
		EMP_AVERAGE	h_power;								// Exponential average of applied power
		EMP_AVERAGE	h_temp;									// Exponential average of Hot Air Gun temperature
		EMP_AVERAGE	d_power;								// Exponential average of power dispersion
		EMP_AVERAGE	zero_temp;								// Exponential average of minimum (zero) temperature
        const       uint8_t     max_fix_power 	= 70;
		const		uint8_t		max_power		= 99;
		const		uint16_t	min_fan_speed	= 600;
		const		uint16_t	max_fan_speed	= 1999;
		const		uint16_t	max_cool_fan	= 1600;
		const		uint16_t	min_working_fan	= 800;
        const       uint16_t    temp_gun_cold   = 100;		// The temperature of the cold Hot Air Gun
        const		uint32_t	fan_off_timeout	= 5*60*1000;// The timeout to turn the fan off in cooling mode
};

#endif
