/*
 * gun.h
 *
 *  Created on: 14 рту. 2019 у.
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
		bool 		isGunReedSwitch(void)					{ return sw_gun.status();					}	// TRUE if switch is short
		bool		isHotAirGunMode(void)					{ return sw_mode.status();					}	// TRUE if switch is short
		bool 		isGunConnected(void) 					{ return c_fan.status();					}
		void		updateFanCurrent(uint16_t value)		{ c_fan.update(value);						}
		void		checkSWStatus(void);
	private:
		uint32_t	check_sw					= 0;		// Time when check tilt switch status (ms)
		SWITCH 		sw_gun;									// Hot Air Gun reed switch
		SWITCH		sw_mode;								// Hardware mode switch (iron/hot air gun)
		SWITCH 		c_fan;									// Hot Air Gun is connected switch
		const		uint16_t	fan_off_value	= 800;
		const 		uint16_t	fan_on_value	= 10000;
		const 		uint8_t		sw_off_value	= 30;
		const 		uint8_t		sw_on_value		= 60;
		const 		uint8_t		sw_avg_len		= 10;
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
		void		updateCurrentTemp(uint16_t value)		{ if (isGunConnected()) h_temp.update(value);	}
		void		setFan(uint16_t fan)					{ fan_speed = constrain(fan, min_working_fan, max_fan_speed);	}
        void        switchPower(bool On);
        uint8_t		avgPowerPcnt(void);
		uint16_t	appliedPower(void);
		uint16_t	fanSpeed(void);
        void        fixPower(uint8_t Power);				// Set the specified power to the the hot gun
		uint8_t		presetFanPcnt(void);
		uint16_t    power(void);							// Required Hot Air Gun power to keep the preset temperature
    private:
		void		shutdown(void)							{ mode = POWER_OFF; TIM2->CCR2 = 0;				}
		PowerMode	mode				= POWER_OFF;
		uint8_t    	fix_power			= 0;				// Fixed power value of the Hot Air Gun (or zero if off)
		bool		chill				= false;			// Chill the Hot Air gun if it is overheats
		uint16_t	temp_set			= 0;				// The preset temperature of the hot air gun (internal units)
		uint16_t	fan_speed			= 0;				// Preset fan speed
		EMP_AVERAGE	h_power;								// Exponential average of applied power
		EMP_AVERAGE	h_temp;									// Exponential average of Hot Air Gun temperature
		EMP_AVERAGE	d_power;
        const       uint8_t     max_fix_power 	= 70;
		const		uint8_t		max_power		= 99;
		const		uint16_t	min_fan_speed	= 600;
		const		uint16_t	max_fan_speed	= 1999;
		const		uint16_t	max_cool_fan	= 1600;
		const		uint16_t	min_working_fan	= 800;
        const       uint16_t    temp_gun_cold   = 40;		// The temperature of the cold Hot Air Gun
};

#endif
