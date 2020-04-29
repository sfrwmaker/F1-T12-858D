/*
 * gun.cpp
 *
 *  Created on: 14 aug 2019
 *      Author: Alex
 */

#include "gun.h"

void HOTGUN_HW::init(void) {
	c_fan.init(sw_avg_len,		fan_off_value,	fan_on_value);
	sw_gun.init(sw_avg_len,		sw_off_value, 	sw_on_value);
}

void HOTGUN_HW::checkSWStatus(void) {
	if (HAL_GetTick() > check_sw) {
		uint16_t on = 0;
		if (GPIO_PIN_SET == HAL_GPIO_ReadPin(GUN_REED_GPIO_Port, GUN_REED_Pin))	on = 100;
		sw_gun.update(on);									// If reed switch open, write 100;
	}
}

void HOTGUN::init(void) {
	mode		= POWER_OFF;								// Completely stopped, no power on fan also
	fan_speed	= 0;
	fix_power	= 0;
	chill		= false;
	HOTGUN_HW::init();
    h_power.reset();
	h_temp.reset();
	d_power.length(ec);
	PID::init(13);											// Initialize PID for Hot Air Gun
    resetPID();
}

uint8_t HOTGUN::avgPowerPcnt(void) {
	uint8_t pcnt = 0;
	if (mode == POWER_FIXED) {
//		pcnt = map(h_power.read(), 0, max_fix_power, 0, 100);
		pcnt = map(fix_power, 0, max_fix_power, 0, 100);
	} else {
		pcnt = map(h_power.read(), 0, max_power, 0, 100);
	}
	if (pcnt > 100) pcnt = 100;
	return pcnt;
}

uint16_t HOTGUN::appliedPower(void) {
	return TIM1->CCR4;
}

uint16_t HOTGUN::fanSpeed(void) {
	return constrain(TIM2->CCR2, 0, 1999);
}

uint16_t HOTGUN::alternateTemp(void) {
	uint16_t t = h_temp.read();
	if (mode == POWER_OFF)
		t = 0;
	return t;
}

void HOTGUN::switchPower(bool On) {
	fan_off_time = 0;										// Disable fan offline by timeout
	switch (mode) {
		case POWER_OFF:
			if (fanSpeed() == 0) {							// No power supplied to the Fan
				if (On)										// !FAN && On
					mode = POWER_ON;
			} else {
				if (On) {
					if (isGunConnected()) {					// FAN && On && connected
						mode = POWER_ON;
					} else {								// FAN && On && !connected
						shutdown();
					}
				} else {
					if (isGunConnected()) {					// FAN && !On && connected
						if (isCold()) {						// FAN && !On && connected && cold
							shutdown();
						} else {							// FAN && !On && connected && !cold
							mode = POWER_COOLING;
							fan_off_time = HAL_GetTick() + fan_off_timeout;
						}
					}
				}
			}
			break;
		case POWER_ON:
			if (!On) {
				mode = POWER_COOLING;
				fan_off_time = HAL_GetTick() + fan_off_timeout;
			}
			break;
		case POWER_FIXED:
			if (fanSpeed()) {
				if (On) {									// FAN && On
					mode = POWER_ON;
				} else {									// FAN && !On
					if (isGunConnected()) {					// FAN && !On && connected
						if (isCold()) {						// FAN && !On && connected && cold
							shutdown();
						} else {							// FAN && !On && connected && !cold
							mode = POWER_COOLING;
							fan_off_time = HAL_GetTick() + fan_off_timeout;
						}
					}
				}
			} else {										// !FAN
				if (!On) {									// !FAN && !On
					shutdown();
				}
			}
			break;
		case POWER_COOLING:
			if (fanSpeed()) {
				if (On) {									// FAN && On
					if (isGunConnected()) {					// FAN && On && connected
						mode = POWER_ON;
					} else {								// FAN && On && !connected
						shutdown();
					}
				} else {									// FAN && !On
					if (isGunConnected()) {
						if (isCold()) {						// FAN && !On && connected && cold
							shutdown();
						}
					} else {								// FAN && !On && !connected
						shutdown();
					}
				}
			} else {
				if (On) {									// !FAN && On
					mode = POWER_ON;

				}
			}
	}
	h_power.reset();
	d_power.reset();
	GPIO_PinState relay_state = (mode == POWER_ON || mode == POWER_FIXED)?GPIO_PIN_SET:GPIO_PIN_RESET;
	HAL_GPIO_WritePin(AC_RELAY_GPIO_Port, AC_RELAY_Pin, relay_state);
}

void HOTGUN::fixPower(uint8_t Power) {
    if (Power == 0) {										// To switch off the hot gun, set the Power to 0
        switchPower(false);
        return;
    }

    if (Power > max_power) Power = max_power;
    mode = POWER_FIXED;
    HAL_GPIO_WritePin(AC_RELAY_GPIO_Port, AC_RELAY_Pin, GPIO_PIN_SET);
    fix_power	= Power;
}

uint16_t HOTGUN::power(void) {
	uint16_t t = h_temp.read();								// Actual Hot Air Gun temperature

	if ((t >= int_temp_max + 100) || (t > (temp_set + 400))) {	// Prevent global over heating
		if (mode == POWER_ON) chill = true;					// Turn off the power in main working mode only;
	}

	int32_t		p = 0;
	switch (mode) {
		case POWER_OFF:
			break;
		case POWER_ON:
			TIM2->CCR2	= fan_speed;
			if (chill) {
				if (t < (temp_set - 2)) {
					chill = false;
					resetPID();
				} else {
					break;
				}
			}
			p = PID::reqPower(temp_set, t);
			p = constrain(p, 0, max_power);
			break;
		case POWER_FIXED:
			p			= fix_power;
			TIM2->CCR2	= fan_speed;
			break;
		case POWER_COOLING:
			if (TIM2->CCR2 < min_fan_speed) {
				shutdown();
			} else {
				if (isGunConnected()) {
					if (isCold()) {							// FAN && connected && cold
						shutdown();
					} else {								// FAN && connected && !cold
						uint16_t fan = map(h_temp.read(), temp_gun_cold, temp_set, max_cool_fan, min_fan_speed);
						fan = constrain(fan, min_fan_speed, max_fan_speed);
						TIM2->CCR2 = fan;
					}
				} else {									// FAN && !connected
					if (fan_off_time) {						// The fan should be turned off in specific time
						if (HAL_GetTick() < fan_off_time)	// It is not time to shutdown the fan
							break;
						fan_off_time = 0;
					}
					shutdown();
				}
			}
			break;
		default:
			break;
	}

	// Only supply the power to the heater if the Hot Air Gun is connected
	if (TIM2->CCR2 < min_fan_speed || !isGunConnected()) p = 0;
	h_power.update(p);
	int32_t	ap	= h_power.average(p);
	int32_t	diff 	= ap - p;
	d_power.update(diff*diff);
	return p;
}

uint8_t	HOTGUN::presetFanPcnt(void) {
	uint16_t pcnt = map(fan_speed, 0, max_fan_speed, 0, 100);
	if (pcnt > 100) pcnt = 100;
	return pcnt;

}

void HOTGUN::shutdown(void)	{
	mode = POWER_OFF;
	TIM2->CCR2 = 0;
	HAL_GPIO_WritePin(AC_RELAY_GPIO_Port, AC_RELAY_Pin, GPIO_PIN_RESET);
}
