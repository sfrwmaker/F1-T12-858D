/*
 * gun.cpp
 *
 *  Created on: 14 aug 2019
 *      Author: Alex
 */

#include "gun.h"

void HOTGUN::init(void) {
	mode		= POWER_OFF;								// Completely stopped, no power on fan also
	fan_speed	= 0;
	fix_power	= 0;
	chill		= false;
	UNIT::init(sw_avg_len, fan_off_value, fan_on_value,   sw_avg_len, sw_off_value, sw_on_value);
	safetyRelay(false);										// Completely turn-off the power of Hot Air Gun
    h_power.reset();
	h_temp.reset();
	d_power.length(ec);
	d_temp.length(ec);
	PID::init(1000, 13, false);								// Initialize PID for Hot Air Gun, 1Hz. Do not forcible heat!
    resetPID();
}

uint8_t HOTGUN::avgPowerPcnt(void) {
	uint8_t pcnt = 0;
	if (mode == POWER_FIXED) {
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

void HOTGUN::updateTemp(uint16_t value) {
	if (isConnected()) {
		int32_t at = h_temp.average(value);
		int32_t diff	= at - value;
		d_temp.update(diff*diff);
	}
}

void HOTGUN::switchPower(bool On) {
	fan_off_time = 0;										// Disable fan offline by timeout
	switch (mode) {
		case POWER_OFF:
			if (fanSpeed() == 0) {							// No power supplied to the Fan
				if (On)	{									// !FAN && On
					safetyRelay(true);						// Supply AC power to the hot air gun socket
					mode = POWER_HEATING;
				}
			} else {
				if (On) {
					if (isConnected()) {					// FAN && On && connected
						safetyRelay(true);
						uint16_t t = h_temp.read();
						if (t < temp_set && t + 200 < temp_set) {
							mode = POWER_HEATING;
						} else {
							mode = POWER_ON;
						}
					} else {								// FAN && On && !connected
						shutdown();
					}
				} else {
					if (isConnected()) {					// FAN && !On && connected
						if (avg_sync_temp < temp_gun_cold) { // FAN && !On && connected && cold
							shutdown();
						} else {							// FAN && !On && connected && !cold
							mode = POWER_COOLING;
							fan_off_time = HAL_GetTick() + fan_off_timeout;
							reach_cold_temp	= false;
						}
					}
				}
			}
			break;
		case POWER_ON:
		case POWER_HEATING:
		case POWER_PID_TUNE:
			if (!On) {										// Start cooling the hot air gun
				mode = POWER_COOLING;
				fan_off_time = HAL_GetTick() + fan_off_timeout;
				reach_cold_temp = false;
			}
			break;
		case POWER_FIXED:
			if (fanSpeed()) {
				if (On) {									// FAN && On
					mode = POWER_ON;
				} else {									// FAN && !On
					if (isConnected()) {					// FAN && !On && connected
						if (avg_sync_temp < temp_gun_cold) { // FAN && !On && connected && cold
							shutdown();
						} else {							// FAN && !On && connected && !cold
							mode = POWER_COOLING;
							fan_off_time = HAL_GetTick() + fan_off_timeout;
							reach_cold_temp = false;
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
					if (isConnected()) {					// FAN && On && connected
						safetyRelay(true);					// Supply AC power to the hot air gun socket
						uint16_t t = h_temp.read();
						if (t < temp_set && t + 200 < temp_set) {
							mode = POWER_HEATING;
						} else {
							mode = POWER_ON;
						}
					} else {								// FAN && On && !connected
						shutdown();
					}
				} else {									// FAN && !On
					if (isConnected()) {
						if (avg_sync_temp < temp_gun_cold) { // FAN && !On && connected && cold
							fan_off_time = HAL_GetTick() + fan_extra_time;
							reach_cold_temp = true;
						}
					} else {								// FAN && !On && !connected
						shutdown();
					}
				}
			} else {
				if (On) {									// !FAN && On
					safetyRelay(true);						// Supply AC power to the hot air gun socket
					mode = POWER_HEATING;
				}
			}
			break;
		default:
			break;
	}
	h_power.reset();
	d_power.reset();
}

void HOTGUN::fixPower(uint16_t Power) {
    if (Power == 0) {										// To switch off the hot gun, set the Power to 0
        switchPower(false);
        return;
    }

    if (Power > max_power) Power = max_power;
    mode = POWER_FIXED;
    safetyRelay(true);										// Supply AC power to the hot air gun socket
    fix_power	= Power;
}

// Called from HAL_TIM_OC_DelayElapsedCallback() event handler 1 time per second (see core.cpp)
uint16_t HOTGUN::power(void) {
	uint16_t t = h_temp.read();								// Actual Hot Air Gun temperature
	avg_sync_temp = t;										// Save average temperature to be read as average value

	if ((t >= int_temp_max + 100) || (t > (temp_set + 400))) {	// Prevent global over heating
		if (mode == POWER_ON) chill = true;					// Turn off the power in main working mode only;
	}

	int32_t	p = 0;											// The Hot Air Gun power value
	switch (mode) {
		case POWER_OFF:
			break;
		case POWER_HEATING:
		case POWER_ON:
			FAN_TIM.Instance->CCR2	= fan_speed;
			if (chill) {
				if (t < (temp_set - 2)) {
					chill = false;
					resetPID();
				} else {
					break;
				}
			}
			if (mode == POWER_HEATING && t >= temp_set + 20) {
				mode = POWER_ON;
				PID::pidStable(stable);
			}
			if (relay_ready_cnt > 0) {						// Relay is not ready yet
				--relay_ready_cnt;							// Do not apply power to the HOT GUN till AC relay is ready
				relay_ready_cnt &= 7;
			} else {
				p = PID::reqPower(temp_set, t);
				p = constrain(p, 0, max_power);
			}
			break;
		case POWER_FIXED:
			if (relay_ready_cnt > 0) {						// Relay is not ready yet
				--relay_ready_cnt;							// Do not apply power to the HOT GUN till AC relay is ready
			} else {
				p = fix_power;
			}
			FAN_TIM.Instance->CCR2	= fan_speed;
			break;
		case POWER_COOLING:
			if (fanSpeed() < min_fan_speed) {
				shutdown();
			} else {
				if (isConnected()) {
					if (avg_sync_temp < temp_gun_cold) {	// FAN && connected && cold
						if (!reach_cold_temp) {
							reach_cold_temp = true;
							fan_off_time = HAL_GetTick() + fan_extra_time;
						}
					} else {								// FAN && connected && !cold
						uint16_t fan = map(avg_sync_temp, temp_gun_cold, temp_set, max_cool_fan, min_fan_speed);
						fan = constrain(fan, min_fan_speed, max_fan_speed);
						FAN_TIM.Instance->CCR2 = fan;
					}
				}
				// Here the FAN is working but the Hot Air Gun can be disconnected
				if (fan_off_time && HAL_GetTick() >= fan_off_time) { // The fan should be turned off in specific time
					shutdown();
				}
			}
			break;
		case POWER_PID_TUNE:
			p = PIDTUNE::run(t);
			break;
		default:
			break;
	}

	// Only supply the power to the heater if the Hot Air Gun is connected
	if (fanSpeed() < min_fan_speed || !isConnected()) p = 0;
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

// Can be called from the event handler.
void HOTGUN::shutdown(void)	{
	mode = POWER_OFF;
	TIM2->CCR2 = 0;
	safetyRelay(false);										// Stop supplying AC power to the hot air gun
	reach_cold_temp = true;
}

// We need some time to activate the relay, so we initialize the relay_ready_cnt variable.
void HOTGUN::safetyRelay(bool activate) {
	if (activate) {
		HAL_GPIO_WritePin(AC_RELAY_GPIO_Port, AC_RELAY_Pin, GPIO_PIN_SET);
		relay_ready_cnt = relay_activate;
	} else {
		HAL_GPIO_WritePin(AC_RELAY_GPIO_Port, AC_RELAY_Pin, GPIO_PIN_RESET);
		relay_ready_cnt = 0;
	}
}
