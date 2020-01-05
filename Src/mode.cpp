/*
 * mode.cpp
 *
 *  Created on: 19 sep. 2019
 *      Author: Alex
 */

#include <stdio.h>
#include <math.h>
#include "mode.h"
#include "tools.h"

void SCRSAVER::reset(void) {
	if (to > 0)
		scr_save_ms = HAL_GetTick() + (uint32_t)to * 60000;
	scr_saver = false;
}

bool SCRSAVER::scrSaver(void) {
    if (scr_save_ms && !scr_saver && HAL_GetTick() >= scr_save_ms) {
    	scr_saver = true;
    }
    return scr_saver;
}

//---------------------- The iron standby mode -----------------------------------
void MSTBY_IRON::init(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->encoder;

	pIron->switchPower(false);
	pD->mainInit();
	bool		celsius 	= pCFG->isCelsius();
	int16_t  	ambient		= pIron->ambientTemp();
	uint16_t 	temp_setH	= pCFG->tempPresetHuman();
	uint16_t 	temp_set	= pCFG->humanToTemp(temp_setH, ambient);
	pIron->setTemp(temp_set);
	pD->msgOFF();
	pD->tip(pCFG->tipName());
	uint16_t t_min					= pCFG->tempMinC();		// The minimum preset temperature, defined in main.h
	uint16_t t_max					= pCFG->tempMaxC();		// The maximum preset temperature
	if (!celsius) {											// The preset temperature saved in selected units
		t_min	= celsiusToFahrenheit(t_min);
		t_max	= celsiusToFahrenheit(t_max);
	}
	pEnc->reset(temp_setH, t_min, t_max, 1, 5, false);
	old_temp_set	= 0;
	update_screen	= 0;									// Force to redraw the screen
	clear_used_ms 	= 0;
	used = !pIron->isCold();								// The IRON is in COOLING mode
	SCRSAVER::init(pCFG->getScrTo());
}

MODE* MSTBY_IRON::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	HOTGUN*	pHG		= &pCore->hotgun;
	RENC*	pEnc	= &pCore->encoder;

    uint16_t temp_set 	= pEnc->read();
    uint8_t  button		= pEnc->buttonStatus();

    if (gun_work && pHG->isGunReedOpen())	{				// The Reed switch is open, switch to Hot Air Gun mode
    	gun_work->keepIronWorking(false);					// Do not switch on the IRON
    	return gun_work;
    }
    if (!pIron->isIronConnected() && isACsine())			// IRON is disconnected, Tip selection mode
    	return mode_return;

    // In the Screen saver mode, any rotary encoder change should be ignored
    if ((button || temp_set != old_temp_set) && scrSaver()) {
    	button = 0;
    	pEnc->write(old_temp_set);
		SCRSAVER::reset();
		update_screen = 0;
	}

    if (button == 1) {										// The button pressed shortly
    	if (mode_spress) return mode_spress;
    } else if (button == 2) {								// The button was pressed for a long time
    	BUZZER::shortBeep();
    	if (mode_lpress) return mode_lpress;
    }

    bool scr_saver_reset = (button > 0);
    if (temp_set != old_temp_set) {							// Preset temperature changed
    	old_temp_set = temp_set;
    	scr_saver_reset = true;
    	pCFG->savePresetTempHuman(temp_set);
    	update_screen = 0;									// Force to redraw the screen
    }
    if (scr_saver_reset) SCRSAVER::reset();

    if (HAL_GetTick() < update_screen) return this;
    update_screen = HAL_GetTick() + 1000;

	if (used && pIron->isCold()) {
    	pD->msgCold();
		BUZZER::lowBeep();
		clear_used_ms = HAL_GetTick() + 60000;
		used = false;
	}

	if (clear_used_ms && HAL_GetTick() >= clear_used_ms) {
		clear_used_ms = 0;
		pD->msgOFF();
	}

	int16_t	 	ambient		= pIron->ambientTemp();
    uint16_t	temp  		= pIron->averageTemp();
    uint16_t	tempH 		= pCFG->tempToHuman(temp, ambient);
	uint16_t	temp_setH	= pCFG->tempPresetHuman();
	uint16_t	gun_temp	= pCore->hotgun.alternateTemp();
	if (gun_temp > 0)
		gun_temp = pCFG->tempToHuman(gun_temp, ambient, DEV_GUN);

    if (scrSaver()) {
    	pD->scrSave(tempH, gun_temp);
    } else {
    	pD->mainShow(temp_setH, tempH, ambient, pIron->avgPowerPcnt(), pCFG->isCelsius(), pCFG->isTipCalibrated(), gun_temp, 0, false);
    }
    return this;
}

//-------------------- The iron main working mode, keep the temperature ----------
void MWORK_IRON::init(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->encoder;

	bool 	 celsius	= pCFG->isCelsius();
	int16_t  ambient	= pIron->ambientTemp();
	uint16_t tempH  	= pCFG->tempPresetHuman();
	preset_temp			= pCFG->humanToTemp(tempH, ambient);
	uint16_t t_min		= pCFG->tempMinC();
	uint16_t t_max		= pCFG->tempMaxC();
	if (!celsius) {											// The preset temperature saved in selected units
		t_min	= celsiusToFahrenheit(t_min);
		t_max	= celsiusToFahrenheit(t_max);
	}
	pEnc->reset(tempH, t_min, t_max, 1, 5, false);
	pIron->setTemp(preset_temp);
	pD->mainInit();
	pD->msgON();
	pD->tip(pCFG->tipName());
	idle_pwr.reset();										// Initialize the history for power in idle state
	auto_off_notified 	= false;
	ready 				= false;
	lowpower_mode		= false;
	lowpower_time		= 0;
	time_to_return		= 0;
	old_temp_set 		= 0;
	update_screen		= 0;
	pIron->switchPower(true);
	SCRSAVER::init(pCFG->getScrTo());
}

void MWORK_IRON::adjustPresetTemp(void) {
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;

	uint16_t presetTemp	= pIron->presetTemp();
	uint16_t tempH     	= pCFG->tempPresetHuman();
	int16_t  ambient	= pIron->ambientTemp();
	uint16_t temp  		= pCFG->humanToTemp(tempH, ambient);	// Expected temperature of IRON in internal units
	if (temp != presetTemp) {								// The ambient temperature have changed, we need to adjust preset temperature
		pIron->adjust(temp);
	}
}

void MWORK_IRON::hwTimeout(uint16_t low_temp, bool tilt_active) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;

	uint32_t now_ms = HAL_GetTick();
	if (tilt_active) {										// If the IRON is used, Reset standby time
		lowpower_time = now_ms + pCFG->getLowTO() * 1000;	// Convert timeout to milliseconds
		if (lowpower_mode) {								// If the IRON is in low power mode, return to main working mode
			pIron->setTemp(preset_temp);
			lowpower_time	= 0;
			lowpower_mode	= false;
			ready 			= false;
			time_to_return	= 0;							// Disable to return to the POWER OFF mode
			pD->msgON();
		}
	} else if (!lowpower_mode) {
		if (lowpower_time) {
			if (now_ms >= lowpower_time) {
				preset_temp			= pIron->presetTemp();		// Save the current preset temperature
				int16_t  ambient	= pIron->ambientTemp();
				uint16_t temp_low	= pCFG->getLowTemp();
				uint16_t temp 		= pCFG->humanToTemp(temp_low, ambient);
				pIron->setTemp(temp);
				time_to_return 		= HAL_GetTick() + pCFG->getOffTimeout() * 60000;
				auto_off_notified 	= false;
				lowpower_mode		= true;
				pD->msgStandby();
				return;
			}
		} else {
			lowpower_time = now_ms + pCFG->getLowTO() * 1000;
		}
	}
}

// Use applied power analysis to automatically power-off the IRON
void MWORK_IRON::swTimeout(uint16_t temp, uint16_t temp_set, uint16_t temp_setH, uint32_t td, uint32_t pd, uint16_t ap, int16_t ip) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;

	if ((temp <= temp_set) && (temp_set - temp <= 4) && (td <= 200) && (pd <= 25)) {
		// Evaluate the average power in the idle state
		ip = idle_pwr.average(ap);
	}

	// Check the IRON current status: idle or used
	if ((ap - ip >= 10)) {							// The applied power is greater than idle power. The IRON being used!
		if (time_to_return) {						// The time to switch off the IRON was initialized
			time_to_return 		= 0;
			auto_off_notified 	= false;			// Initialize the idle state power
		}
	} else {										// The IRON is is its idle state
		if (!time_to_return) {
			time_to_return = HAL_GetTick() + pCFG->getOffTimeout() * 60000;
			auto_off_notified 	= false;
		}
		pD->msgIdle();
		return;
	}
	pD->msgON();
}

MODE* MWORK_IRON::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->encoder;

	int temp_setH 		= pEnc->read();						// The preset temperature in human readable units
	uint8_t  button		= pEnc->buttonStatus();

	// Switch to Hot Air Gun mode when the Reed switch is open.
	if (gun_work && pCore->hotgun.isGunReedOpen()) {
		if (lowpower_mode) {
			pIron->setTemp(preset_temp);					// Restore iron working temperature
		}
		gun_work->keepIronWorking(pCFG->isKeepIron());		// Keep IRON working if enabled
    	return gun_work;
	}

    // In the Screen saver mode, any rotary encoder change should be ignored
    if ((button || temp_setH != old_temp_set) && scrSaver()) {
    	button = 0;
    	pEnc->write(old_temp_set);
		SCRSAVER::reset();
		update_screen = 0;
    }

    if (button == 1) {										// The button pressed
        pCFG->saveConfig();
    	if (mode_spress)		return mode_spress;
    } else if (button == 2) {								// The button was pressed for a long time, turn the booster mode
    	if (pCFG->boostTemp()) {
    		if (mode_lpress) 	return mode_lpress;
    	}
    }

    bool scr_saver_reset = (button > 0);
	int16_t ambient	= pIron->ambientTemp();
	if (temp_setH != old_temp_set) {						// Encoder rotated, new preset temperature entered
	   	old_temp_set 		= temp_setH;
		ready 				= false;
		time_to_return 		= 0;
		auto_off_notified 	= false;
		lowpower_mode		= false;
		pD->msgON();
		uint16_t temp = pCFG->humanToTemp(temp_setH, ambient); // Translate human readable temperature into internal value
		pIron->setTemp(temp);
		pCFG->savePresetTempHuman(temp_setH);
		idle_pwr.reset();									// Initialize the history for power in idle state
		update_screen		= 0;
		scr_saver_reset 	= true;
	}
	if (scr_saver_reset) SCRSAVER::reset();

	if (HAL_GetTick() < update_screen) 		return this;
    update_screen = HAL_GetTick() + period;

    int temp			= pIron->averageTemp();
	int temp_set		= pIron->presetTemp();				// Now the preset temperature in internal units!!!
	uint8_t p 			= pIron->avgPowerPcnt();
	uint16_t tempH 		= pCFG->tempToHuman(temp, ambient);

	uint32_t td			= pIron->tmpDispersion();			// The temperature dispersion
	uint32_t pd 		= pIron->pwrDispersion();			// The power dispersion
	int ip      		= idle_pwr.read();					// Applied power in the idle mode
	int ap      		= pIron->avgPower();				// Actually applied power to the IRON
	uint16_t gun_temp	= pCore->hotgun.alternateTemp();
	if (gun_temp > 0)
		gun_temp = pCFG->tempToHuman(gun_temp, ambient, DEV_GUN);

	uint16_t low_temp = pCFG->getLowTemp();					// 'Standby temperature' setup in the main menu
	bool tilt_active = false;
	if (low_temp) tilt_active = pIron->isIronTiltSwitch();	// True if iron is in upright position

	// Check the IRON reaches the preset temperature
	if ((abs(temp_set - temp) < 6) && (td <= 500) && (ap > 0))  {
	    if (!ready) {
	    	ready = true;
	    	ready_clear	= HAL_GetTick() + 2000;
	    	pD->msgReady();
	    	BUZZER::shortBeep();
	    	pD->mainShow(temp_setH, tempH, ambient, p, pCFG->isCelsius(), pCFG->isTipCalibrated(), gun_temp, 0, tilt_active);
	    	return this;
	    }
	}

	// If the automatic power-off feature is enabled, check the IRON status
	if (pCFG->getOffTimeout() && ready && !ready_clear) {	// The IRON has reaches the preset temperature and 'Ready' message is already cleared
		if (low_temp) {										// Use hardware tilt switch to turn low power mode
			hwTimeout(low_temp, tilt_active);
		} else {											// Or use software mode to switch to power OFF mode
			swTimeout(temp, temp_set, temp_setH, td, pd, ap, ip);
		}

		// Show the time remaining to switch off the IRON
		if (time_to_return) {
			uint32_t to = (time_to_return - HAL_GetTick()) / 1000;
			if (to < 100) {
				pD->timeToOff(to);
				if (!auto_off_notified) {
					BUZZER::shortBeep();
					auto_off_notified = true;
				}
			}
		}
	}
	if (!lowpower_mode)
		adjustPresetTemp();

	if (ready && ready_clear && HAL_GetTick() >= ready_clear) {
		ready_clear = 0;
		pD->msgON();
	}

	if (scrSaver()) {
	   	pD->scrSave(tempH, gun_temp);
	} else {
	   	pD->mainShow(temp_setH, tempH, ambient, p, pCFG->isCelsius(), pCFG->isTipCalibrated(), gun_temp, 0, tilt_active);
	}
	return this;
}

//---------------------- The boost mode, shortly increase the temperature --------
void MBOOST::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->encoder;

	uint16_t temp_set 	= pIron->presetTemp();
	bool celsius		= pCFG->isCelsius();
	uint16_t ambient	= pIron->ambientTemp();
	uint16_t tempH  	= pCFG->tempToHuman(temp_set, ambient);
	uint16_t delta		= pCFG->boostTemp();				// The temperature increment in Celsius
	if (!celsius)
		delta = (delta * 9 + 3) / 5;
	tempH			   += delta;
	temp_set 			= pCFG->humanToTemp(tempH, ambient);
	pIron->setTemp(temp_set);
	pIron->fixPower(0xffff);								// Apply maximum value of fixed power
	time_to_return		= HAL_GetTick() + 30000;
	pEnc->reset(0, 0, 1, 1, 1, false);
	old_pos				= 0;
	update_screen		= 0;
	heating				= true;
}

MODE* MBOOST::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->encoder;

	int pos 			= pEnc->read();
	uint8_t  button		= pEnc->buttonStatus();

    if (button || (old_pos != pos)) {						// The button pressed or encoder rotated
    	return mode_return;									// Return to the main mode if button pressed
    }

    if (heating) {
    	uint16_t temp 		= pIron->averageTemp();
    	uint16_t temp_set	= pIron->presetTemp();
    	if (temp_set <= temp) {								// Finish heating
    		pIron->switchPower(true);						// Keep boos temperature
    		heating = false;
    	}
    }

	if (HAL_GetTick() < update_screen) 	return this;
    update_screen = HAL_GetTick() + 500;

    uint16_t ambient= pIron->ambientTemp();
    int temp		= pIron->averageTemp();
	uint8_t p 		= pIron->avgPowerPcnt();
	uint16_t tempH 	= pCFG->tempToHuman(temp, ambient);
	uint16_t tset	= pIron->presetTemp();
	uint16_t tsetH  = pCFG->tempToHuman(tset, ambient);
	pD->msgBoost();
	pD->mainShow(tsetH, tempH, ambient, p, pCFG->isCelsius(), pCFG->isTipCalibrated(), 0, 0, false);
	return this;
}

//---------------------- The tip selection mode ----------------------------------
void MSLCT::init(void) {;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	uint8_t tip_index 	= pCFG->currentTipIndex();
	// Build list of the active tips; The current tip is the second element in the list
	uint8_t list_len 	= pCFG->tipList(tip_index, tip_list, 3, true);

	// The current tip could be inactive, so we should find nearest tip (by ID) in the list
	uint8_t closest		= 0;								// The index of the list of closest tip ID
	uint8_t diff  		= 0xff;
	for (uint8_t i = 0; i < list_len; ++i) {
		uint8_t delta;
		if ((delta = abs(tip_index - tip_list[i].tip_index)) < diff) {
			diff 	= delta;
			closest = i;
		}
	}
	pEnc->reset(closest, 0, list_len-1, 1, 1, false);
	tip_begin_select = HAL_GetTick();						// We stared the tip selection procedure
	old_index		= 3;
	update_screen	= 0;									// Force to redraw the screen
}

MODE* MSLCT::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;
	IRON*	pIron	= &pCore->iron;

	uint8_t	 index 		= pEnc->read();
	if (index != old_index) {
		tip_begin_select 	= 0;
		update_screen 		= 0;
	}
	uint8_t	button = pEnc->buttonStatus();

    if (pIron->isIronConnected() || !isACsine()) {
		// Prevent bouncing event, when the IRON connection restored back too quickly.
		if (tip_begin_select && (HAL_GetTick() - tip_begin_select) < 1000) {
			return 0;
		}
		uint8_t tip_index = tip_list[index].tip_index;
		pCFG->changeTip(tip_index);
		pIron->reset();										// Clear temperature history and switch iron mode to "power off"
		return mode_return;
	}

	if (button == 2) {										// The button was pressed for a long time
	    return mode_lpress;
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 20000;

	for (int8_t i = index; i >= 0; --i) {
		if (tip_list[(uint8_t)i].name[0]) {
			index = i;
			break;
		}
	}
	old_index = index;
	uint8_t tip_index = tip_list[index].tip_index;
	for (uint8_t i = 0; i < 3; ++i)
		tip_list[i].name[0] = '\0';
	uint8_t list_len = pCFG->tipList(tip_index, tip_list, 3, true);
	if (list_len == 0)										// There is no active tip in the list
		return mode_spress;									// Activate tips mode

	for (uint8_t i = 0; i < list_len; ++i) {
		if (tip_index == tip_list[i].tip_index) {
			pEnc->write(i);
		}
	}
	pD->tipListShow("Select tip",  tip_list, 3, tip_index, true);
	return this;
}

//---------------------- The Activate tip mode: select tips to use ---------------
void MTACT::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	uint8_t tip_index = pCFG->currentTipIndex();
	pEnc->reset(tip_index, 1, pCFG->TIPS::loaded()-1, 1, 2, false);	// Start from tip #1, because 0-th 'tip' is a Hot Air Gun
	old_tip_index = 255;
	update_screen = 0;
}

MODE* MTACT::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	uint8_t	 tip_index 	= pEnc->read();
	uint8_t	button		= pEnc->buttonStatus();

	if (button == 1) {										// The button pressed
		if (!pCFG->toggleTipActivation(tip_index)) {
			pD->errorMessage("EEPROM\nwrite\nerror");
			return 0;
		}
		update_screen = 0;									// Force redraw the screen
	} else if (button == 2) {
		return mode_lpress;
	}

	if (tip_index != old_tip_index) {
		old_tip_index = tip_index;
		update_screen = 0;
	}

	if (HAL_GetTick() >= update_screen) {
		TIP_ITEM	tip_list[3];
		uint8_t loaded = pCFG->tipList(tip_index, tip_list, 3, false);
		pD->tipListShow("Activate tip",  tip_list, loaded, tip_index, false);
		update_screen = HAL_GetTick() + 60000;
	}
	return this;
}

//---------------------- The Menu mode -------------------------------------------
void MODE::setup(MODE* return_mode, MODE* short_mode, MODE* long_mode) {
	mode_return	= return_mode;
	mode_spress	= short_mode;
	mode_lpress	= long_mode;
}

MODE* MODE::returnToMain(void) {
	if (mode_return && time_to_return && HAL_GetTick() >= time_to_return)
		return mode_return;
	return this;
}

void MODE::resetTimeout(void) {
	if (timeout_secs) {
		time_to_return = HAL_GetTick() + timeout_secs * 1000;
	}
}
void MODE::setTimeout(uint16_t t) {
	timeout_secs = t;
}

//---------------------- The Menu mode -------------------------------------------
MMENU::MMENU(HW* pCore, MODE* m_boost, MODE* m_calib, MODE* m_act, MODE* m_tune, MODE* m_pid, MODE* m_gun_menu) : MODE(pCore) {
	mode_menu_boost		= m_boost;
	mode_calibrate_menu	= m_calib;
	mode_activate_tips	= m_act;
	mode_tune			= m_tune;
	mode_tune_pid		= m_pid;
	mode_gun_menu		= m_gun_menu;
}

void MMENU::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	off_timeout	= pCFG->getOffTimeout();
	low_temp	= pCFG->getLowTemp();
	low_to		= pCFG->getLowTO();
	buzzer		= pCFG->isBuzzerEnabled();
	celsius		= pCFG->isCelsius();
	keep_iron	= pCFG->isKeepIron();
	scr_saver	= pCFG->getScrTo();
	set_param	= 0;
	if (!pCFG->isTipCalibrated())
		mode_menu_item	= 10;									// Select calibration menu item
	pEnc->reset(mode_menu_item, 0, m_len-1, 1, 1, true);
	update_screen = 0;
}

MODE* MMENU::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	uint8_t item 		= pEnc->read();
	uint8_t  button		= pEnc->buttonStatus();

	// Change the configuration parameters value in place
	if (mode_menu_item != item) {								// The encoder has been rotated
		mode_menu_item = item;
		switch (set_param) {									// Setup new value of the parameter in place
			case 4:												// Setup auto off timeout
				if (item) {
					off_timeout	= item + 2;
				} else {
					off_timeout = 0;
				}
				break;
			case 5:												// Setup low power (standby) temperature
				if (item >= min_standby_C) {
					low_temp = item;
				} else {
					low_temp = 0;
				}
				break;
			case 6:												// Setup low power (standby) timeout
				low_to	= item;
				break;
			case 7:												// Setup Screen saver timeout
				if (item) {
					scr_saver = item + 2;
				} else {
					scr_saver = 0;
				}
				break;
			default:
				break;
		}
		update_screen = 0;										// Force to redraw the screen
	}

	// Going through the main menu
	if (!set_param) {
		if (button > 0) {										// The button was pressed
			switch (item) {
				case 0:											// Boost parameters
					pCFG->setup(off_timeout, buzzer, celsius, keep_iron, low_temp, low_to, scr_saver);
					return mode_menu_boost;
				case 1:											// units C/F
					celsius	= !celsius;
					break;
				case 2:											// buzzer ON/OFF
					buzzer	= !buzzer;
					break;
				case 3:											// Keep iron ON/OFF
					keep_iron =!keep_iron;
					break;
				case 4:											// auto off timeout
					{
					set_param = item;
					uint8_t to = off_timeout;
					if (to > 2) to -=2;
					pEnc->reset(to, 0, 28, 1, 5, false);
					break;
					}
				case 5:											// Standby temperature
					{
					set_param = item;
					pEnc->reset(low_temp, min_standby_C-1, max_standby_C, 1, 5, false);
					break;
					}
				case 6:											// Standby timeout
					set_param = item;
					pEnc->reset(low_to, 5, 240, 1, 5, false);
					break;
				case 7:											// Screen saver timeout
					{
					set_param = item;
					uint8_t to = scr_saver;
					if (to > 2) to -=2;
					pEnc->reset(to, 0, 58, 1, 5, false);
					break;
					}
				case 8:											// save
					pCFG->setup(off_timeout, buzzer, celsius, keep_iron, low_temp, low_to, scr_saver);
					pCFG->saveConfig();
					mode_menu_item = 0;
					return mode_return;
				case 10:										// calibrate IRON tip
					mode_menu_item = 8;
					return mode_calibrate_menu;
				case 11:										// activate tips
					mode_menu_item = 0;							// We will not return from tip activation mode to this menu
					return mode_activate_tips;
				case 12:										// tune the IRON potentiometer
					mode_menu_item = 0;							// We will not return from tune mode to this menu
					mode_tune->ironMode(true);
					return mode_tune;
				case 13:										// Hot Air Gun menu
					mode_menu_item = 11;						// We will return from next level menu here
					return mode_gun_menu;
				case 14:										// Initialize the configuration
					pCFG->initConfigArea();
					mode_menu_item = 0;							// We will not return from tune mode to this menu
					return mode_return;
				case 15:										// Tune PID
					return mode_tune_pid;
				case 16:										// About dialog
					mode_menu_item = 0;
					pD->showVersion();
					HAL_Delay(10000);
					return mode_return;
				default:										// cancel
					pCFG->restoreConfig();
					mode_menu_item = 0;
					return mode_return;
			}
		}
	} else {													// Finish modifying  parameter
		if (button == 1) {
			item 			= set_param;
			mode_menu_item 	= set_param;
			set_param = 0;
			pEnc->reset(mode_menu_item, 0, m_len-1, 1, 1, true);
		}
	}

	// Prepare to modify menu item
	bool modify = false;
	if (set_param >= 4 && set_param <= 7) {
		item = set_param;
		modify 	= true;
	}

	if (button > 0) {											// Either short or long press
		update_screen 	= 0;									// Force to redraw the screen
	}
	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 10000;

	// Build current menu item value
	char item_value[10];
	item_value[1] = '\0';
	switch (item) {
		case 1:													// units: C/F
			item_value[0] = 'F';
			if (celsius)
				item_value[0] = 'C';
			break;
		case 2:													// Buzzer setup
			if (buzzer)
				sprintf(item_value, "ON");
			else
				sprintf(item_value, "OFF");
			break;
		case 3:													// Keep iron working while in Hot Air Gun Mode
			if (keep_iron)
				sprintf(item_value, "KEEP");
			else
				sprintf(item_value, "OFF");
			break;
		case 4:													// auto off timeout
			if (off_timeout) {
				sprintf(item_value, "%2d min", off_timeout);
			} else {
				sprintf(item_value, "OFF");
			}
			break;
		case 5:													// Standby temperature
			if (low_temp) {
				if (celsius) {
					sprintf(item_value, "%3d C", low_temp);
				} else {
					sprintf(item_value, "%3d F", celsiusToFahrenheit(low_temp));
				}
			} else {
				sprintf(item_value, "OFF");
			}
			break;
		case 6:													// Standby timeout
			if (low_temp)
				sprintf(item_value, "%3d s", low_to);
			else
				sprintf(item_value, "OFF");
			break;
		case 7:
			if (scr_saver) {
				sprintf(item_value, "%2d min", scr_saver);
			} else {
				sprintf(item_value, "OFF");
			}
			break;
		default:
			item_value[0] = '\0';
			break;
	}

	pD->menuItemShow("setup", menu_name[item], item_value, modify);
	return this;
}

//---------------------- Calibrate tip menu --------------------------------------
MCALMENU::MCALMENU(HW* pCore, MODE* cal_auto, MODE* cal_manual) : MODE(pCore) {
	mode_calibrate_tip = cal_auto; mode_calibrate_tip_manual = cal_manual;
}

void MCALMENU::init(void) {
	RENC*	pEnc	= &pCore->encoder;
	pEnc->reset(0, 0, 3, 1, 1, true);
	old_item		= 4;
	update_screen	= 0;
}

MODE* MCALMENU::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	uint8_t item 	= pEnc->read();
	uint8_t button	= pEnc->buttonStatus();

	if (button == 1) {
		update_screen = 0;										// Force to redraw the screen
	} else if (button == 2) {									// The button was pressed for a long time
	   	return mode_lpress;
	}

	if (old_item != item) {
		old_item = item;
		update_screen = 0;										// Force to redraw the screen
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 10000;

	if (button == 1) {											// The button was pressed
		switch (item) {
			case 0:												// Calibrate tip automatically
				mode_calibrate_tip->ironMode(true);
				return mode_calibrate_tip;
			case 1:												// Calibrate tip manually
				mode_calibrate_tip_manual->ironMode(true);
				return mode_calibrate_tip_manual;
			case 2:												// Initialize tip calibration data
				pCFG->resetTipCalibration();
				return mode_return;
			default:											// exit
				return mode_return;
		}
	}

	pD->menuItemShow("Calibrate", menu_list[item], 0, false);
	return this;
}

//---------------------- The automatic calibration tip mode ----------------------
/*
 * There are 4 temperature calibration points of the tip in the controller,
 * but during calibration procedure we will use more points to cover whole set
 * of the internal temperature values.
 */
void MCALIB::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->encoder;

	// Prepare to enter real temperature
	uint16_t min_t 		= 50;
	uint16_t max_t		= 600;
	if (!pCFG->isCelsius()) {
		min_t 	=  122;
		max_t 	= 1111;
	}
	PIDparam pp = pCFG->pidParamsSmooth();						// Load PID parameters to stabilize the temperature of unknown tip
	pIron->PID::load(pp);
	pEnc->reset(0, min_t, max_t, 1, 5, false);
	for (uint8_t i = 0; i < MCALIB_POINTS; ++i) {
		calib_temp[0][i] = 0;									// Real temperature. 0 means not entered yet
		calib_temp[1][i] = map(i, 0, MCALIB_POINTS-1, start_int_temp, int_temp_max / 2); // Internal temperature
	}
	ref_temp_index 	= 0;
	ready			= false;
	tuning			= false;
	old_encoder 	= 3;
	update_screen 	= 0;
	tip_temp_max 	= int_temp_max / 2;							// The maximum possible temperature defined in iron.h
}

/*
 * Calculate tip calibration parameter using linear approximation by Ordinary Least Squares method
 * Y = a * X + b, where
 * Y - internal temperature, X - real temperature. a and b are double coefficients
 * a = (N * sum(Xi*Yi) - sum(Xi) * sum(Yi)) / ( N * sum(Xi^2) - (sum(Xi))^2)
 * b = 1/N * (sum(Yi) - a * sum(Xi))
 */
bool MCALIB::calibrationOLS(uint16_t* tip, uint16_t min_temp, uint16_t max_temp) {
	long sum_XY = 0;											// sum(Xi * Yi)
	long sum_X 	= 0;											// sum(Xi)
	long sum_Y  = 0;											// sum(Yi)
	long sum_X2 = 0;											// sum(Xi^2)
	long N		= 0;

	for (uint8_t i = 0; i < MCALIB_POINTS; ++i) {
		uint16_t X 	= calib_temp[0][i];
		uint16_t Y	= calib_temp[1][i];
		if (X >= min_temp && X <= max_temp) {
			sum_XY 	+= X * Y;
			sum_X	+= X;
			sum_Y   += Y;
			sum_X2  += X * X;
			++N;
		}
	}

	if (N <= 2)													// Not enough real temperatures have been entered
		return false;

	double	a  = (double)N * (double)sum_XY - (double)sum_X * (double)sum_Y;
			a /= (double)N * (double)sum_X2 - (double)sum_X * (double)sum_X;
	double 	b  = (double)sum_Y - a * (double)sum_X;
			b /= (double)N;

	for (uint8_t i = 0; i < 4; ++i) {
		double temp = a * (double)pCore->cfg.referenceTemp(i) + b;
		tip[i] = round(temp);
	}
	if (tip[3] > int_temp_max) tip[3] = int_temp_max;			// Maximal possible temperature (main.h)
	return true;
}

// Find the index of the reference point with the closest temperature
uint8_t MCALIB::closestIndex(uint16_t temp) {
	uint16_t diff = 1000;
	uint8_t index = MCALIB_POINTS;
	for (uint8_t i = 0; i < MCALIB_POINTS; ++i) {
		uint16_t X = calib_temp[0][i];
		if (X > 0 && abs(X-temp) < diff) {
			diff = abs(X-temp);
			index = i;
		}
	}
	return index;
}

void MCALIB::updateReference(uint8_t indx) {					// Update reference points
	CFG*	pCFG	= &pCore->cfg;
	uint16_t expected_temp 	= map(indx, 0, MCALIB_POINTS, pCFG->tempMinC(), pCFG->tempMaxC());
	uint16_t r_temp			= calib_temp[0][indx];
	if (indx < 5 && r_temp > (expected_temp + expected_temp/4)) {	// The real temperature is too high
		tip_temp_max -= tip_temp_max >> 2;						// tip_temp_max *= 0.75;
		if (tip_temp_max < int_temp_max / 4)
			tip_temp_max = int_temp_max / 4;					// Limit minimum possible value of the highest temperature

	} else if (r_temp > (expected_temp + expected_temp/8)) { 	// The real temperature is biger than expected
		tip_temp_max += tip_temp_max >> 3;						// tip_temp_max *= 1.125;
		if (tip_temp_max > int_temp_max)
			tip_temp_max = int_temp_max;
	} else if (indx < 5 && r_temp < (expected_temp - expected_temp/4)) { // The real temperature is too low
		tip_temp_max += tip_temp_max >> 2;						// tip_temp_max *= 1.25;
		if (tip_temp_max > int_temp_max)
			tip_temp_max = int_temp_max;
	} else if (r_temp < (expected_temp - expected_temp/8)) { 	// The real temperature is lower than expected
		tip_temp_max += tip_temp_max >> 3;						// tip_temp_max *= 1.125;
		if (tip_temp_max > int_temp_max)
			tip_temp_max = int_temp_max;
	} else {
		return;
	}

	// rebuild the array of the reference temperatures
	for (uint8_t i = indx+1; i < MCALIB_POINTS; ++i) {
		calib_temp[1][i] = map(i, 0, MCALIB_POINTS-1, start_int_temp, tip_temp_max);
	}
}


void MCALIB::buildFinishCalibration(void) {
	CFG* 	pCFG 	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	uint16_t tip[4];
	if (calibrationOLS(tip, 150, pCFG->referenceTemp(2))) {
		uint8_t near_index	= closestIndex(pCFG->referenceTemp(3));
		tip[3] = map(pCFG->referenceTemp(3), pCFG->referenceTemp(2), calib_temp[0][near_index],
				tip[2], calib_temp[1][near_index]);
		if (tip[3] > int_temp_max) tip[3] = int_temp_max;		// Maximal possible temperature (main.h)

		uint8_t tip_index 	= pCFG->currentTipIndex();
		int16_t ambient 	= pIron->ambientTemp();
		pCFG->applyTipCalibtarion(tip, ambient);
		pCFG->saveTipCalibtarion(tip_index, tip, TIP_ACTIVE | TIP_CALIBRATED, ambient);
	}
}

MODE* MCALIB::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->encoder;

	uint16_t encoder	= pEnc->read();
    uint8_t  button		= pEnc->buttonStatus();

    if (encoder != old_encoder) {
    	old_encoder = encoder;
    	update_screen = 0;
    }

	if (!pIron->isIronConnected()) {
		PIDparam pp = pCFG->pidParams(use_iron);				// Restore default PID parameters
		pIron->PID::load(pp);
		return 0;												// Report an ERROR
	}

	if (button == 1) {											// The button pressed
		if (tuning) {											// New reference temperature was entered
			pIron->switchPower(false);
		    if (ready) {										// The temperature was stabilized and real data can be entered
		    	ready = false;
			    uint16_t temp	= pIron->averageTemp();			// The temperature of the IRON in internal units
			    uint16_t r_temp = encoder;						// The real temperature entered by the user
			    if (!pCFG->isCelsius())							// Always save the human readable temperature in Celsius
			    	r_temp = fahrenheitToCelsius(r_temp);
			    uint8_t ref = ref_temp_index;
			    calib_temp[0][ref] = r_temp;
			    calib_temp[1][ref] = temp;
			    if (r_temp < pCFG->tempMaxC() - 20) {
			    	updateReference(ref_temp_index);			// Update reference points
			    	++ref_temp_index;
			    	// Try to update the current tip calibration
			    	uint16_t tip[4];
			    	 if (calibrationOLS(tip, 150, 600))
			    		 pCFG->applyTipCalibtarion(tip, pIron->ambientTemp());
			    } else {										// Finish calibration
			    	ref_temp_index = MCALIB_POINTS;
			    }
		    } else {											// Stop heating, return from tuning mode
		    	tuning = false;
		    	update_screen = 0;
		    	return this;
		    }
		    tuning = false;
		}
		if (!tuning) {
			if (ref_temp_index < MCALIB_POINTS) {
				tuning = true;
				uint16_t temp = calib_temp[1][ref_temp_index];
				pIron->setTemp(temp);
				pIron->switchPower(true);
			} else {											// All reference points are entered
				buildFinishCalibration();
				PIDparam pp = pCFG->pidParams(use_iron);		// Restore default PID parameters
				pIron->PID::load(pp);
				return mode_lpress;
			}
		}
		update_screen = 0;
	} else if (!tuning && button == 2) {						// The button was pressed for a long time, save tip calibration
		buildFinishCalibration();
		PIDparam pp = pCFG->pidParams(use_iron);				// Restore default PID parameters
		pIron->PID::load(pp);
	    return mode_lpress;
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 500;

	int16_t	 ambient	= pIron->ambientTemp();
	uint16_t real_temp 	= encoder;
	uint16_t temp_set	= pIron->presetTemp();
	uint16_t temp 		= pIron->averageTemp();
	uint8_t  power		= pIron->avgPowerPcnt();
	uint16_t tempH 		= pCFG->tempToHuman(temp, ambient);

	if (temp >= int_temp_max) {									// Prevent soldering IRON overheat, save current calibration
		buildFinishCalibration();
		PIDparam pp = pCFG->pidParams(use_iron);				// Restore default PID parameters
		pIron->PID::load(pp);
		return mode_lpress;
	}

	if (tuning && (abs(temp_set - temp) <= 16) && (pIron->pwrDispersion() <= 200) && power > 1)  {
		if (!ready) {
			BUZZER::shortBeep();
			pEnc->write(tempH);
			ready = true;
	    }
	}

	uint8_t	int_temp_pcnt = 0;
	if (temp >= start_int_temp)
		int_temp_pcnt = map(temp, start_int_temp, int_temp_max, 0, 100);	// int_temp_max defined in vars.cpp
	pD->calibShow(pCFG->tipName(), ref_temp_index+1, tempH, real_temp, pCFG->isCelsius(), power, tuning, ready, int_temp_pcnt);
	return this;
}

//---------------------- The manual calibration tip mode -------------------------
/*
 * Here the operator should 'guess' the internal temperature readings for desired temperature.
 * Rotate the encoder to change temperature preset in the internal units
 * and controller would keep that temperature.
 * This method is more accurate one, but it requires more time.
 */
void MCALIB_MANUAL::init(void) {
	CFG*	pCFG		= &pCore->cfg;
	PIDparam pp 		= pCFG->pidParamsSmooth(use_iron);
	if (use_iron) {
		pCore->iron.PID::load(pp);
	} else {
		pCore->hotgun.PID::load(pp);
		pCore->hotgun.setFan(fan_speed);
	}
	pCFG->activateGun(!use_iron);
	pCore->encoder.reset(0, 0, 3, 1, 1, true);					// Select reference temperature point using Encoder
	pCFG->getTipCalibtarion(calib_temp);						// Load current calibration data
	ref_temp_index 		= 0;
	ready				= false;
	tuning				= false;
	temp_setready_ms	= 0;
	old_encoder			= 4;
	update_screen		= 0;
}

/*
 * Make sure the tip[0] < tip[1] < tip[2] < tip[3];
 * And the difference between next points is greater than req_diff
 * Change neighborhood temperature data to keep this difference
 */
void MCALIB_MANUAL::buildCalibration(int8_t ablient, uint16_t tip[], uint8_t ref_point) {
	if (tip[3] > int_temp_max) tip[3] = int_temp_max;			// int_temp_max is a maximum possible temperature (vars.cpp)

	const int req_diff = 200;
	if (ref_point <= 3) {										// tip[0-3] - internal temperature readings for the tip at reference points (200-400)
		for (uint8_t i = ref_point; i <= 2; ++i) {				// ref_point is 0 for 200 degrees and 3 for 400 degrees
			int diff = (int)tip[i+1] - (int)tip[i];
			if (diff < req_diff) {
				tip[i+1] = tip[i] + req_diff;					// Increase right neighborhood temperature to keep the difference
			}
		}
		if (tip[3] > int_temp_max)								// The high temperature limit is exceeded, temp_max. Lower all calibration
			tip[3] = int_temp_max;

		for (int8_t i = 3; i > 0; --i) {
			int diff = (int)tip[i] - (int)tip[i-1];
			if (diff < req_diff) {
				int t = (int)tip[i] - req_diff;					// Decrease left neighborhood temperature to keep the difference
				if (t < 0) t = 0;
				tip[i-1] = t;
			}
		}
	}
}

void MCALIB_MANUAL::restorePIDconfig(CFG *pCFG, IRON* pIron, HOTGUN* pHG) {
	PIDparam pp = pCFG->pidParams(use_iron);
	if (use_iron) {
		pIron->PID::load(pp);
	} else {
		pHG->PID::load(pp);
		pCFG->activateGun(false);
	}
}

MODE* MCALIB_MANUAL::loop(void) {
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	HOTGUN*	pHG		= &pCore->hotgun;
	RENC*	pEnc	= &pCore->encoder;

	uint16_t encoder	= pEnc->read();
    uint8_t  button		= pEnc->buttonStatus();

    if (encoder != old_encoder) {
    	old_encoder = encoder;
    	if (tuning) {											// Preset temperature (internal units)
    		if (use_iron) {
    			pIron->setTemp(encoder);
    		} else {
    			pHG->setTemp(encoder);
    		}
    		ready = false;
    		temp_setready_ms = HAL_GetTick() + 5000;    		// Prevent beep just right the new temperature setup
    	}
    	update_screen = 0;
    }

	int16_t ambient = pIron->ambientTemp();
    if (use_iron) {
    	if (!pIron->isIronConnected()) {
    		restorePIDconfig(pCFG, pIron, pHG);
    		return 0;
    	}
	} else if (temp_setready_ms && (HAL_GetTick() > temp_setready_ms) && !pHG->isGunConnected()) {
		restorePIDconfig(pCFG, pIron, pHG);
		return 0;
	}

	if (button == 1) {											// The button pressed
		if (tuning) {											// New reference temperature was confirmed
			pIron->switchPower(false);
			pHG->switchPower(false);
		    if (ready) {										// The temperature has been stabilized
		    	ready = false;
		    	uint16_t temp	= 0;
		    	if (use_iron)
		    		temp	= pIron->averageTemp();				// The temperature of the IRON in internal units
		    	else
		    		temp	= pHG->averageTemp();
			    uint8_t ref 	= ref_temp_index;
			    calib_temp[ref] = temp;
			    uint16_t tip[4];
			    for (uint8_t i = 0; i < 4; ++i) {
			    	tip[i] = calib_temp[i];
			    }
			    buildCalibration(ambient, tip, ref);			// ref is 0 for 200 degrees and 3 for 400 degrees
			    pCFG->applyTipCalibtarion(tip, ambient);
		    }
		    tuning	= false;
			encoder = ref_temp_index;
		    pEnc->reset(encoder, 0, 3, 1, 1, true);				// Turn back to the reference temperature point selection mode
		} else {												// Reference temperature index was selected from the list
			ref_temp_index 	= encoder;
			tuning 			= true;
			uint16_t tempH 	= pCFG->referenceTemp(encoder);		// Read the preset temperature from encoder
			uint16_t temp 	= pCFG->humanToTemp(tempH, ambient);
			pEnc->reset(temp, 100, int_temp_max, 5, 20, false); // int_temp_max declared in vars.cpp
			if (use_iron) {
				pIron->setTemp(temp);
				pIron->switchPower(true);
			} else {
				pHG->setTemp(temp);
				pHG->switchPower(true);
			}
		}
		update_screen = 0;
	} else if (button == 2) {									// The button was pressed for a long time, save tip calibration
		uint8_t tip_index = pCFG->currentTipIndex();			// IRON actual tip index
		buildCalibration(ambient, calib_temp, 10); 				// 10 is bigger then the last index in the reference temp. Means build final calibration
		pCFG->applyTipCalibtarion(calib_temp, ambient);
		pCFG->saveTipCalibtarion(tip_index, calib_temp, TIP_ACTIVE | TIP_CALIBRATED, ambient);
		restorePIDconfig(pCFG, pIron, pHG);
	    return mode_lpress;
	}

	uint8_t rt_index = encoder;									// rt_index is a reference temperature point index. Read it fron encoder
	if (tuning) {
		rt_index	= ref_temp_index;
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 500;

	uint16_t temp_set		= 0;								// Prepare the parameters to be displayed
	uint16_t temp			= 0;
	uint8_t  power			= 0;
	uint16_t pwr_disp		= 0;
	uint16_t pwr_disp_max	= 200;
	if (use_iron) {
		temp_set		= pIron->presetTemp();
		temp 			= pIron->averageTemp();
		power			= pIron->avgPowerPcnt();
		pwr_disp		= pIron->pwrDispersion();

	} else {
		temp_set		= pHG->presetTemp();
		temp 			= pHG->averageTemp();
		power			= pHG->avgPowerPcnt();
		pwr_disp		= pHG->pwrDispersion();
		pwr_disp_max	= 40;
	}
	if (tuning && (abs(temp_set - temp) <= 16) && (pwr_disp <= pwr_disp_max) && power > 1)  {
		if (!ready && temp_setready_ms && (HAL_GetTick() > temp_setready_ms)) {
			BUZZER::shortBeep();
			ready 				= true;
			temp_setready_ms	= 0;
	    }
	}

	uint16_t temp_setup = temp_set;
	if (!tuning) {
		uint16_t tempH 	= pCFG->referenceTemp(encoder);
		temp_setup 		= pCFG->humanToTemp(tempH, ambient);
	}

	pCore->dspl.calibManualShow(pCFG->tipName(), pCFG->referenceTemp(rt_index), temp, temp_setup,
			pCFG->isCelsius(), power, tuning, ready);
	return	this;
}

//---------------------- The Boost setup menu mode -------------------------------
void MMBST::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	delta_temp	= pCFG->boostTemp();							// The boost temp is in the internal units
	duration	= pCFG->boostDuration();
	mode		= 0;
	pEnc->reset(0, 0, 2, 1, 1, true);							// Select the boot menu item
	old_item	= 0;
	update_screen = 0;
}

MODE* MMBST::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	uint8_t item 		= pEnc->read();
	uint8_t  button		= pEnc->buttonStatus();

	if (button == 1) {
		update_screen = 0;										// Force to redraw the screen
	} else if (button == 2) {									// The button was pressed for a long time
		// Save the boost parameters to the current configuration. Do not write it to the EEPROM!
		pCFG->saveBoost(delta_temp, duration);
		return mode_lpress;
	}

	if (old_item != item) {
		old_item = item;
		switch (mode) {
			case 1:												// New temperature increment
				delta_temp	= item;
				break;
			case 2:												// New duration period
				duration	= item;
				break;
		}
		update_screen = 0;										// Force to redraw the screen
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 10000;

	if (!mode) {												// The boost menu item selection mode
		if (button == 1) {										// The button was pressed
			switch (item) {
				case 0:											// delta temperature
					{
					mode 	= 1;
					pEnc->reset(delta_temp, 0, 75, 5, 20, false);
					break;
					}
				case 1:											// duration
					{
					mode 	= 2;
					uint8_t dur	= duration;
					pEnc->reset(dur, 5, 80, 5, 20, false);
					break;
					}
				case 2:											// save
				default:
					// Save the boost parameters to the current configuration. Do not write it to the EEPROM!
					pCFG->saveBoost(delta_temp, duration);
					return mode_return;
			}
		}
	} else {													// Return to the item selection mode
		if (button == 1) {
			pEnc->reset(mode-1, 0, 2, 1, 1, true);
			mode = 0;
			return this;
		}
	}

	// Show current menu item
	char item_value[8];
	item_value[1] = '\0';
	if (mode) {
		item = mode - 1;
	}
	switch (item) {
		case 0:													// delta temperature
			if (delta_temp) {
				uint16_t delta_t = delta_temp;
				char sym = 'C';
				if (!pCFG->isCelsius()) {
					delta_t = (delta_t * 9 + 3) / 5;
					sym = 'F';
				}
				sprintf(item_value, "+%2d %c", delta_t, sym);
			} else {
				sprintf(item_value, "OFF");
			}
			break;
		case 1:													// duration (secs)
		    sprintf(item_value, "%2d s.", duration);
			break;
		default:
			item_value[0] = '\0';
			break;
	}

	pD->menuItemShow("Boost", boost_name[item], item_value, mode);
	return this;
}


//---------------------- The tune mode -------------------------------------------
void MTUNE::init(void) {
	RENC*	pEnc	= &pCore->encoder;
	uint16_t max_power = 0;
	if (use_iron) {
		max_power = pCore->iron.getMaxFixedPower();
		check_delay	= 0;										// IRON connection can be checked any time
	} else {
		HOTGUN*	pHG	= &pCore->hotgun;
		max_power 	= pHG->getMaxFixedPower();
		pHG->setFan(1500);										// Make sure the fan will be blowing well.
		check_delay		= HAL_GetTick() + 2000;					// Wait 2 seconds before checking Hot Air Gun
	}
	pEnc->reset(max_power/3, 0, max_power, 1, 5, false);
	old_power		= 0;
	powered			= true;
	check_connected	= false;
	pCore->dspl.mainInit();									// Clear power status message
	pCore->dspl.msgON();
}

MODE* MTUNE::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	IRON*	pIron	= &pCore->iron;
	HOTGUN*	pHG		= &pCore->hotgun;
	RENC*	pEnc	= &pCore->encoder;

    uint16_t power 	= pEnc->read();
    uint8_t  button	= pEnc->buttonStatus();

    if (!check_connected) {
    	check_connected = HAL_GetTick() >= check_delay;
    } else {
    	if (use_iron) {
    		if (!pIron->isIronConnected())
    			return 0;
    	} else {
    		if (pHG->isFanWorking() && !pHG->isGunConnected())
    				return 0;
    	}
    }

	if (button == 1) {											// The button pressed
		powered = !powered;
	    if (powered) pD->msgON(); else pD->msgOFF();
	} else if (button == 2) {									// The button was pressed for a long time
	    return mode_lpress;
	}

	if (!powered) power = 0;

    if (power != old_power) {
    	if (use_iron) {
    		pIron->fixPower(power);
    	} else {
    		pHG->fixPower(power);
    	}
    	old_power = power;
    }

    uint16_t tune_temp = gun_temp_maxC;							// vars.cpp
    if (use_iron) tune_temp = iron_temp_maxC;
    uint16_t temp		= 0;
    uint8_t	 p_pcnt		= 0;
	if (use_iron) {
		temp 		= pIron->averageTemp();
		p_pcnt		= pIron->avgPowerPcnt();

	} else {
		temp		= pHG->averageTemp();
		p_pcnt		= pHG->avgPowerPcnt();
	}

    pD->tuneShow(tune_temp, temp, p_pcnt);
    HAL_Delay(500);
    return this;
}

//---------------------- The PID coefficients tune mode --------------------------
void MTPID::init(void) {
	DSPL*	pD		= &pCore->dspl;
	RENC*	pEnc	= &pCore->encoder;

	pD->pidInit();
	pD->pidSetLowerAxisLabel("Dp");
	pEnc->reset(0, 0, 2, 1, 1, true);							// Select the coefficient to be modified
	pCore->iron.setTemp(1200);									// Use 'middle' temperature
	pCore->hotgun.setTemp(1200);
	pCore->hotgun.setFan(1500);
	data_update 		= 0;
	data_index 			= 0;
	modify				= false;
	on					= false;
	old_index			= 3;
	temp_setready_ms	= 0;
	update_screen 		= 0;
}

MODE* MTPID::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	HOTGUN* pHG		= &pCore->hotgun;
	RENC*	pEnc	= &pCore->encoder;

	uint16_t  index 	= pEnc->read();
	uint8_t  button		= pEnc->buttonStatus();

    if (use_iron) {
    	if (!pIron->isIronConnected())
    		return 0;
	} else if (temp_setready_ms && (HAL_GetTick() > temp_setready_ms) && !pHG->isGunConnected())
		return 0;

	if(button || old_index != index)
		update_screen = 0;

	if (HAL_GetTick() >= data_update) {
		data_update = HAL_GetTick() + 100;
		int16_t  temp = 0;
		uint32_t disp = 0;
		if (use_iron) {
			temp 	= pIron->averageTemp() - pIron->presetTemp();
			disp	= pIron->pwrDispersion();
		} else {
			temp 	= pHG->averageTemp() - pHG->presetTemp();
			disp	= pHG->pwrDispersion();
		}
		pD->pidPutData(temp, disp);
	}

	if (HAL_GetTick() < update_screen) return this;

	PID* pPID	= pHG;
	if (use_iron) pPID	= pIron;							// Pointer to the PID class instance

	if (modify) {											// The Coefficient is selected, start to show the Graphs
		update_screen = HAL_GetTick() + 100;
		if (button == 1) {									// Short button press: select another PID coefficient
			modify = false;
			pEnc->reset(data_index, 0, 2, 1, 1, true);
			return this;									// Restart the procedure
		} else if (button == 2) {							// Long button press: toggle the power
			on = !on;
			if (use_iron)
				pIron->switchPower(on);
			else
				pHG->switchPower(on);
			if (on) pD->pidInit();							// Reset display graph history
			BUZZER::shortBeep();
		}

		if (old_index != index) {
			old_index = index;
			pPID->changePID(data_index+1, index);
			pD->pidModify(data_index, index);
		}
		uint8_t pwr_pcnt = 0;
		if (use_iron)
			pwr_pcnt = pIron->avgPowerPcnt();
		else
			pwr_pcnt = pHG->avgPowerPcnt();
		pD->pidShowGraph(pwr_pcnt);
	} else {												// Selecting the PID coefficient to be tuned
		update_screen = HAL_GetTick() + 1000;

		if (old_index != index) {
			old_index = index;
			data_index  = index;
		}

		if (button == 1) {									// Short button press: select another PID coefficient
			modify = true;
			data_index  = index;
			// Prepare to change the coefficient [index]
			uint16_t k = 0;
			k = pPID->changePID(index+1, -1);				// Read the PID coefficient from the IRON or Hot Air Gun
			pEnc->reset(k, 0, 20000, 1, 10, false);
			return this;									// Restart the procedure
		} else if (button == 2) {							// Long button press: save the parameters and return to menu
			PIDparam pp = pPID->dump();
			pCFG->savePID(pp, use_iron);
			BUZZER::shortBeep();
			return mode_lpress;
		}

		uint16_t pid_k[3];
		for (uint8_t i = 0; i < 3; ++i) {
			pid_k[i] = 	pPID->changePID(i+1, -1);
		}
		pD->pidShowMenu(pid_k, data_index);
	}
	return this;
}

//---------------------- The Hot Air Gun main working mode -----------------------
void MWORK_GUN::init(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	HOTGUN*	pHG		= &pCore->hotgun;

	edit_temp			= true;
	return_to_temp		= 0;
	ready				= false;
	ready_clear			= 0;
	pCFG->activateGun(true);								// Load the Hot Air Gun calibration data (like another tip)
	pD->mainInit();
	uint16_t	fan		= pCFG->gunFanPreset();
	pHG->setFan(fan);										// Preset temperature will be set up in loop method
	pD->msgOFF();
	int16_t	 ambient 	= pCore->iron.ambientTemp();
	uint16_t temp_setH	= pCFG->gunTempPreset();
	uint16_t temp_set	= pCFG->humanToTemp(temp_setH, ambient);
	pHG->setTemp(temp_set);
	pD->fanSpeed(pHG->presetFanPcnt());
	pHG->switchPower(true);
	pD->msgON();
	if (keep_iron) {										// Turn on the IRON because we want to keep it running
		pCore->iron.switchPower(true);
	}
	old_param		= 0;
	update_screen	= 0;									// Force to redraw the screen
	return_to_temp	= 1;									// Initialize Hot Air Gun configuration at the main loop
	fan_animate		= 1;									// Do spin the fan icon
	fan_angle		= 0;
	SCRSAVER::init(pCFG->getScrTo());
}

MODE* MWORK_GUN::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	HOTGUN*	pHG		= &pCore->hotgun;
	RENC*	pEnc	= &pCore->encoder;

    int16_t	 ambient 	= pCore->iron.ambientTemp();

    // The fan speed modification mode has 'return_to_temp' timeout
	if (return_to_temp && HAL_GetTick() >= return_to_temp) {// This reads the Hot Air Gun configuration Also
		bool celsius 		= pCFG->isCelsius();
		uint16_t temp_setH	= pCFG->gunTempPreset();
		uint16_t t_min		= pCFG->tempMinC();				// The minimum preset temperature, defined in iron.h
		uint16_t t_max		= pCFG->tempMaxC();				// The maximum preset temperature
		if (!celsius) {										// The preset temperature saved in selected units
			t_min	= celsiusToFahrenheit(t_min);
			t_max	= celsiusToFahrenheit(t_max);
		}
		pEnc->reset(temp_setH, t_min, t_max, 1, 5, false);
		edit_temp		= true;
		return_to_temp	= 0;
		old_param		= temp_setH;
	}

    uint16_t param 		= pEnc->read();
    uint8_t  button		= pEnc->buttonStatus();

    if (iron_standby && !pHG->isGunReedOpen()) {			// If the REED switch is closed, return to iron standby mode
    	pCFG->saveConfig();									// Save configuration into EEPROM
    	pCFG->activateGun(false);							// Load the current tip calibration data
    	if (keep_iron && iron_working) {
    		keep_iron = false;								// Be paranoid, disable IRON
    		return iron_working;
    	}
    	return iron_standby;
    }

    // In the Screen saver mode, any rotary encoder change should be ignored
    if ((button || param != old_param) && scrSaver()) {
    	button = 0;
    	pEnc->write(old_param);
		SCRSAVER::reset();
		update_screen = 0;
    }

    if (button) {											// The button was pressed, toggle temp/fan
    	update_screen = 0;
    	if (edit_temp) {									// Switch to edit fan speed
    		uint16_t fan = pHG->presetFan();
    		uint16_t max = pHG->maxFanSpeed();
    		pEnc->reset(fan, 0, max, 5, 20, false);
    		edit_temp 		= false;
    		old_param		= fan;
    		return_to_temp	= HAL_GetTick() + edit_fan_timeout;
    	} else {
    		return_to_temp	= HAL_GetTick();				// Force to return to edit temperature
    		return this;
    	}
    }

    bool scr_saver_reset = (button > 0);
    if (param != old_param) {								// Changed preset temperature or fan speed
    	old_param = param;									// In first loop the preset temperature will be setup for sure
    	uint16_t t	= pHG->presetTemp();
    	uint16_t f	= pHG->presetFan();
    	if (edit_temp) {
    		t = pCFG->humanToTemp(param, ambient);
    		pHG->setTemp(t);
    	} else {
    		f = param;
    		pHG->setFan(f);
    		pD->fanSpeed(pHG->presetFanPcnt());
    		return_to_temp	= HAL_GetTick() + edit_fan_timeout;
    	}
    	uint16_t temp_setH	= pCFG->tempToHuman(t, ambient);
    	pCFG->saveGunPreset(temp_setH, f);
    	update_screen 	= 0;								// Force to redraw the screen
    	scr_saver_reset	= true;
    }
    if (scr_saver_reset) SCRSAVER::reset();

	if (fan_animate && HAL_GetTick() >= fan_animate && pHG->isGunConnected()) {
		pD->animateFan(fan_angle);
		++fan_angle;
		fan_angle &= 0x3;
		fan_animate = HAL_GetTick() + 100;
	}

    if (HAL_GetTick() < update_screen) return this;
    update_screen = HAL_GetTick() + 500;

    int16_t  temp_set	= pHG->presetTemp();
    int16_t  temp 		= pHG->averageTemp();
    uint16_t pd			= pHG->pwrDispersion();
    uint8_t  pwr		= pHG->avgPowerPcnt();

    if (!ready && (abs(temp_set - temp) < 50) && (pd <= 7) && (pwr > 0)) {
    	ready = true;
    	ready_clear	= HAL_GetTick() + 5000;
    	BUZZER::shortBeep();
    	pD->msgReady();
    }

    if (ready_clear && HAL_GetTick() >= ready_clear) {
    	pD->msgON();
    	ready_clear	= 0;
    }
    uint16_t temp_setH	= pCFG->gunTempPreset();
    uint16_t tempH		= pCFG->tempToHuman(temp, ambient);
    uint16_t iron_temp	= pCore->iron.alternateTemp();		// Average IRON temperature or 0 if IRON is powered off
    if (iron_temp > 0)
    	iron_temp = pCFG->tempToHuman(iron_temp, ambient, DEV_IRON);

    if (scrSaver()) {
        pD->scrSave(tempH, iron_temp);
    } else {
    	pD->mainShow(temp_setH, tempH, ambient, pwr, pCFG->isCelsius(), pCFG->isTipCalibrated(), iron_temp, fan_angle+1, false);
    }
    return this;
}

//---------------------- Hot Air Gun setup menu ----------------------------------
MENU_GUN::MENU_GUN(HW* pCore, MODE* calib, MODE* pot_tune, MODE* pid_tune) : MODE(pCore) {
	mode_calibrate	= calib;
	mode_tune		= pot_tune;
	mode_pid		= pid_tune;
}

void MENU_GUN::init(void) {
	pCore->encoder.reset(0, 0, 4, 1, 1, true);
	old_item		= 5;
	update_screen	= 0;
}

MODE* MENU_GUN::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->encoder;

	uint8_t item 	= pEnc->read();
	uint8_t button	= pEnc->buttonStatus();

	if (button == 1) {
		update_screen = 0;										// Force to redraw the screen
	} else if (button == 2) {									// The button was pressed for a long time
	   	return mode_lpress;
	}

	if (old_item != item) {
		old_item = item;
		update_screen = 0;										// Force to redraw the screen
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 10000;

	if (button == 1) {											// The button was pressed
		switch (item) {
			case 0:												// Calibrate Hot Air Gun
				if (mode_calibrate) {
					mode_calibrate->ironMode(false);
					return mode_calibrate;
				}
				break;
			case 1:												// Tune Hot air Gun potentiometer
				if (mode_tune) {
					mode_tune->ironMode(false);
					return mode_tune;
				}
				break;
			case 2:												// Tune Hot Air Gun PID parameters
				if (mode_pid) {
					mode_pid->ironMode(false);
					return mode_pid;
				}
				break;
			case 3:												// Initialize Hot Air Gun calibration data
				pCFG->resetTipCalibration();
				return mode_return;
			default:											// exit
				return mode_return;
		}
	}

	pD->menuItemShow("Hot Gun", menu_list[item], 0, false);
	return this;
}

//---------------------- The Fail mode: display error message --------------------
void MFAIL::init(void) {
	RENC*	pEnc	= &pCore->encoder;
	pEnc->reset(0, 0, 1, 1, 1, false);
	BUZZER::failedBeep();
	update_screen = 0;
}

MODE* MFAIL::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	RENC*	pEnc	= &pCore->encoder;
	if (pEnc->buttonStatus()) {
		//pD->resetScale();
		return mode_return;
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 60000;

	pD->errorShow();
	return this;
}

//---------------------- The Debug mode: display internal parameters ------------
void MDEBUG::init(void) {
	gun_mode = pCore->hotgun.isGunReedOpen();
	if (gun_mode) {
		pCore->encoder.reset(0, 0, max_fan_power,  1, 5, false);
	} else {
		pCore->encoder.reset(0, 0, max_iron_power, 1, 5, false);
	}
	update_screen = 0;
}

MODE* MDEBUG::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	IRON*	pIron	= &pCore->iron;
	HOTGUN*	pHG		= &pCore->hotgun;

	bool gun_active = pHG->isGunReedOpen();
	if (gun_active != gun_mode) {								// Current Mode has been changed
		gun_mode = gun_active;
		pIron->fixPower(0);
		pHG->fanFixed(0);
		old_power = 0;
		if (gun_mode) {											// Switch to gun mode
			pCore->encoder.reset(0, 0, max_fan_power,  1, 5, false);
		} else {
			pCore->encoder.reset(0, 0, max_iron_power, 1, 5, false);
		}
	}

	uint16_t pwr 	= pCore->encoder.read();
	if (pwr != old_power) {
		old_power = pwr;
		update_screen = 0;
		if (gun_mode) {
			pHG->fanFixed(pwr);
		} else {
			pIron->fixPower(pwr);
		}
	}
	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 500;

	uint16_t data[6];
	data[0]		= pIron->temp();
	data[1] 	= pIron->ironCurrent();
	data[2]		= pHG->averageTemp();
	data[3]		= pHG->fanCurrent();
	data[4]		= pIron->isIronConnected();
	data[5]		= pHG->isGunConnected();
	pD->debugShow(gun_mode, pwr, data);
	return this;
}

