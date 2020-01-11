/*
 * config.cpp
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 */

#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "iron.h"
#include "tools.h"
#include "eeprom.h"
#include "buzzer.h"
#include "iron_tips.h"

/*
 * The configuration data consists of two separate items:
 * 1. The configuration record (struct s_config)
 * 2. The tip calibration record (struct s_tip_list_item)
 * The external EEPROM IC, at24c32a, divided to two separate area
 * to store configuration records of each type. See eeprom.c for details
 */

#define	 NO_TIP_CHUNK	255									// The flag showing that the tip was not found in the EEPROM

// Initialize the configuration. Find the actual record in the EEPROM.
CFG_STATUS CFG::init(void) {
	TIP_CFG::activateGun(false);
	tip_table = (TIP_TABLE*)malloc(sizeof(TIP_TABLE) * TIPS::loaded());
	uint8_t tips_loaded = 0;

	if (EEPROM::init()) {
		if (tip_table) {
			tips_loaded = buildTipTable(tip_table);
		}

		if (loadRecord(&a_cfg)) {
			correctConfig(&a_cfg);
		} else {
			setDefaults();
		}

		selectTip(0);										// Load Hot Air Gun calibtarion data (virtual tip)
		selectTip(a_cfg.tip);								// Load tip configuration data into a_tip variable
		CFG_CORE::syncConfig();								// Save spare configuration
		if (tips_loaded > 0) {
			return CFG_OK;
		} else {
			return CFG_NO_TIP;
		}
	} else {
		setDefaults();
		TIP_CFG::defaultCalibration(0);						// 0 means Hot Air Gun
		selectTip(1);
		CFG_CORE::syncConfig();
	}
	return CFG_READ_ERROR;
}

// Load calibration data of the tip from EEPROM. If the tip is not calibrated, initialize the calibration data with the default values
bool CFG::selectTip(uint8_t index) {
	if (!tip_table) return false;
	bool result = true;
	uint8_t tip_chunk_index = tip_table[index].tip_chunk_index;
	if (tip_chunk_index == NO_TIP_CHUNK) {
		TIP_CFG::defaultCalibration(index == 0);			// index == 0 means Hot Air Gun
		return false;
	}
	TIP tip;
	if (loadTipData(&tip, tip_chunk_index) != EPR_OK) {
		TIP_CFG::defaultCalibration(index == 0);			// index == 0 means Hot Air Gun
		result = false;
	} else {
		if (!(tip.mask & TIP_CALIBRATED)) {					// Tip is not calibrated, load default config
			TIP_CFG::defaultCalibration(index == 0);
		} else if (!isValidTipConfig(&tip)) {
			TIP_CFG::defaultCalibration(index == 0);
		} else {											// Tip configuration record is completely correct
			TIP_CFG::load(tip, index == 0);
		}
	}
	return result;
}

// Change the current tip. Save configuration to the EEPROM
void CFG::changeTip(uint8_t index) {
	if (selectTip(index)) {
		a_cfg.tip	= index;
		saveConfig();
	}
}

// Return current tip index (or 0 if Hot Air Gun is active)
uint8_t CFG::currentTipIndex(void) {
	if (!TIP_CFG::gunActive())
		return a_cfg.tip;
	else
		return 0;
}

/*
 * Translate the internal temperature of the IRON or Hot Air Gun to the human readable units (Celsius or Fahrenheit)
 * Parameters:
 * temp 		- Device temperature in internal units
 * ambient		- The ambient temperature
 * force_mode	-
 */
uint16_t CFG::tempToHuman(uint16_t temp, int16_t ambient, CFG_TEMP_DEVICE force_device) {
	uint16_t tempH = TIP_CFG::tempCelsius(temp, ambient, force_device);
	if (!CFG_CORE::isCelsius())
		tempH = celsiusToFahrenheit(tempH);
	return tempH;
}
// Translate the temperature from human readable units (Celsius or Fahrenheit) to the internal units
uint16_t CFG::humanToTemp(uint16_t t, int16_t ambient) {
	int d = ambient - TIP_CFG::ambientTemp();
	uint16_t t200	= referenceTemp(0) + d;
	uint16_t t400	= referenceTemp(3) + d;
	uint16_t tmin	= tempMinC();
	uint16_t tmax	= tempMaxC();
	if (!CFG_CORE::isCelsius()) {
		t200 = celsiusToFahrenheit(t200);
		t400 = celsiusToFahrenheit(t400);
		tmin = celsiusToFahrenheit(tmin);
		tmax = celsiusToFahrenheit(tmax);
	}
	t = constrain(t, tmin, tmax);

	uint16_t left 	= 0;
	uint16_t right 	= int_temp_max;
	uint16_t temp = map(t, t200, t400, TIP_CFG::calibration(0), TIP_CFG::calibration(3));

	if (temp > (left+right)/ 2) {
		temp -= (right-left) / 4;
	} else {
		temp += (right-left) / 4;
	}

	for (uint8_t i = 0; i < 20; ++i) {
		uint16_t tempH = tempToHuman(temp, ambient);
		if (tempH == t) {
			return temp;
		}
		uint16_t new_temp;
		if (tempH < t) {
			left = temp;
			 new_temp = (left+right)/2;
			if (new_temp == temp)
				new_temp = temp + 1;
		} else {
			right = temp;
			new_temp = (left+right)/2;
			if (new_temp == temp)
				new_temp = temp - 1;
		}
		temp = new_temp;
	}
	return temp;
}

// Build the complete tip name (including "T12-" prefix)
const char* CFG::tipName(void) {
	uint8_t tip_index = 0;
	if (!TIP_CFG::gunActive())
		tip_index = a_cfg.tip;
	static char tip_name[tip_name_sz+5];
	return buildFullTipName(tip_name, tip_index);
}

// Save current configuration to the EEPROM
void CFG::saveConfig(void) {
	if (CFG_CORE::areConfigsIdentical())
		return;
	saveRecord(&a_cfg);										// calculates CRC and changes ID
	CFG_CORE::syncConfig();
}

void CFG::savePID(PIDparam &pp, bool iron) {
	if (iron) {
		a_cfg.iron_Kp	= pp.Kp;
		a_cfg.iron_Ki	= pp.Ki;
		a_cfg.iron_Kd	= pp.Kd;
	} else {
		a_cfg.gun_Kp	= pp.Kp;
		a_cfg.gun_Ki	= pp.Ki;
		a_cfg.gun_Kd	= pp.Kd;
	}
	saveRecord(&a_cfg);
	CFG_CORE::syncConfig();
}

// Save new IRON tip calibration data to the EEPROM only. Do not change active configuration
void CFG::saveTipCalibtarion(uint8_t index, uint16_t temp[4], uint8_t mask, int8_t ambient) {
	TIP tip;
	tip.t200		= temp[0];
	tip.t260		= temp[1];
	tip.t330		= temp[2];
	tip.t400		= temp[3];
	tip.mask		= mask;
	tip.ambient		= ambient;
	tip_table[index].tip_mask	= mask;
	const char* name	= TIPS::name(index);
	if (name && isValidTipConfig(&tip)) {
		strncpy(tip.name, name, tip_name_sz);
		uint8_t tip_chunk_index = tip_table[index].tip_chunk_index;
		if (tip_chunk_index == NO_TIP_CHUNK) {				// This tip data is not in the EEPROM, it was not active!
			tip_chunk_index = freeTipChunkIndex();
			if (tip_chunk_index == NO_TIP_CHUNK) {			// Failed to find free slot to save tip configuration
				BUZZER::failedBeep();
				return;
			}
			tip_table[index].tip_chunk_index	= tip_chunk_index;
			tip_table[index].tip_mask			= mask;
		}
		if (saveTipData(&tip, tip_table[index].tip_chunk_index) == EPR_OK)
			BUZZER::shortBeep();
		else
			BUZZER::failedBeep();
	}

}

// Toggle (activate/deactivate) tip activation flag. Do not change active tip configuration
bool CFG::toggleTipActivation(uint8_t index) {
	if (!tip_table)	return false;
	TIP tip;
	uint8_t tip_chunk_index = tip_table[index].tip_chunk_index;
	if (tip_chunk_index == NO_TIP_CHUNK) {					// This tip data is not in the EEPROM, it was not active!
		tip_chunk_index = freeTipChunkIndex();
		if (tip_chunk_index == NO_TIP_CHUNK) return false;	// Failed to find free slot to save tip configuration
		const char *name = TIPS::name(index);
		if (name) {
			strncpy(tip.name, name, tip_name_sz);			// Initialize tip name
			tip.mask = TIP_ACTIVE;
			if (saveTipData(&tip, tip_chunk_index) == EPR_OK) {
				if (isTipCorrect(tip_chunk_index, &tip)) {
					tip_table[index].tip_chunk_index	= tip_chunk_index;
					tip_table[index].tip_mask			= tip.mask;
					return true;
				}
			}
		}
	} else {												// Tip configuration data exists in the EEPROM
		if (loadTipData(&tip, tip_chunk_index) == EPR_OK) {
			tip.mask ^= TIP_ACTIVE;
			if (saveTipData(&tip, tip_chunk_index) == EPR_OK) {
				tip_table[index].tip_mask			= tip.mask;
				return true;
			}
		}
	}
	return false;
}

// Check the TIP data was written correctly
bool CFG::isTipCorrect(uint8_t tip_chunk_index, TIP *tip) {
	bool same_name = true;
	forceReloadChunk();							// Reread the chunk, disable EEPROM cache
	TIP read_tip;
	if (loadTipData(&read_tip, tip_chunk_index) == EPR_OK) {
		for (uint8_t i = 0; i < tip_name_sz; ++i) {
			if (read_tip.name[i] != tip->name[i]) {
				same_name = false;
				break;
			}
		}
	}
	return same_name;
}

 // Build the tip list starting from the previous tip
int	CFG::tipList(uint8_t second, TIP_ITEM list[], uint8_t list_len, bool active_only) {
	if (!tip_table) {										// If tip_table is not initialized, return empty list
		for (uint8_t tip_index = 0; tip_index < list_len; ++tip_index) {
			list[tip_index].name[0] = '\0';					// Clear whole list
		}
		return 0;
	}

	uint8_t loaded = 0;
	// Seek backward for one more tip
	for (int tip_index = second - 1; tip_index > 0; --tip_index) { // Do not insert Hot Air Gun 'tip' (tip_index == 0) into the list 		
		if (!active_only || (tip_table[tip_index].tip_mask & TIP_ACTIVE)) {
			list[loaded].tip_index	= tip_index;
			list[loaded].mask		= tip_table[tip_index].tip_mask;
			buildFullTipName(list[loaded].name, tip_index);
			++loaded;
			break;											// Load just one tip
		}
	}

	for (uint8_t tip_index = second; tip_index < TIPS::loaded(); ++tip_index) {
		if (tip_index == 0) continue;						// Skip Hot Air Gun 'tip'
		if (active_only && !(tip_table[tip_index].tip_mask & TIP_ACTIVE)) // This tip is not active, but active tip list required
			continue;										// Skip this tip
		list[loaded].tip_index	= tip_index;
		list[loaded].mask		= tip_table[tip_index].tip_mask;
		buildFullTipName(list[loaded].name, tip_index);
		++loaded;
		if (loaded >= list_len)	break;
	}
	for (uint8_t tip_index = loaded; tip_index < list_len; ++tip_index) {
		list[tip_index].name[0] = '\0';						// Clear rest of the list
	}
	return loaded;
}

// Initialize the configuration area. Save default configuration to the EEPROM
void CFG::initConfigArea(void) {
	clearConfigArea();
	setDefaults();
	saveRecord(&a_cfg);
	clearAllTipsCalibration();
}

void CFG::clearAllTipsCalibration(void) {
	TIP tmp_tip;

	for (uint8_t i = 0; i < TIPS::loaded(); ++i) {
		if (tip_table[i].tip_chunk_index != NO_TIP_CHUNK) {
			uint8_t m = tip_table[i].tip_mask;
			// Check The tip is calibrated
			if ((m & TIP_ACTIVE) && (m & TIP_CALIBRATED)) {
				if (loadTipData(&tmp_tip, i) == EPR_OK) {
					tmp_tip.mask 			= TIP_ACTIVE;	// Clear calibrated flag
					tip_table[i].tip_mask	= TIP_ACTIVE;
					if (saveTipData(&tmp_tip, i) != EPR_OK) {
						break;								// Stop writing to EEPROM on the first IO error
					}
				}
			}
		}
	}
}

/*
 * Builds the tip configuration table: reads whole tip configuration area and search for configured or active tip
 * If the tip found, updates the tip_table array with the tip chunk number
 */
uint8_t	CFG::buildTipTable(TIP_TABLE tt[]) {
	for (uint8_t i = 0; i < TIPS::loaded(); ++i) {
		tt[i].tip_chunk_index 	= NO_TIP_CHUNK;
		tt[i].tip_mask 			= 0;
	}

	TIP  tmp_tip;
	int	 tip_index 	= 0;
	int loaded 		= 0;
	for (int i = 0; i < tipDataTotal(); ++i) {
		switch (loadTipData(&tmp_tip, i)) {
			case EPR_OK:
				tip_index = TIPS::index(tmp_tip.name);
				// Loaded existing tip data once
				if (tip_index >= 0 && tmp_tip.mask > 0 && tt[tip_index].tip_chunk_index == NO_TIP_CHUNK) {
					tt[tip_index].tip_chunk_index 	= i;
					tt[tip_index].tip_mask			= tmp_tip.mask;
					++loaded;
				}
				break;
			case EPR_IO:									// Exit immediately in case of IO error
				return loaded;
			default:										// Continue the procedure on all other errors
				break;
		}
	}
	return loaded;
}

// Build full name of the current tip. Add prefix "T12-" for the "usual" tip or use complete name for "N*" tips
char* CFG::buildFullTipName(char *tip_name, const uint8_t index) {
	const char *name = TIPS::name(index);
	if (name) {
		if (index == 0 || name[0] == 'N') {					// Do not modify Hot Air Gun 'tip' name nor N* names
			strncpy(tip_name, name, tip_name_sz);
			tip_name[tip_name_sz] = '\0';
		} else {											// All other names should be prefixed with 'T12-'
			strcpy(tip_name, "T12-");
			strncpy(&tip_name[4], name, tip_name_sz);
			tip_name[tip_name_sz+4] = '\0';
		}
	} else {
		strcpy(tip_name, "T12-def");
	}
	return tip_name;
}

// Compare two configurations
bool CFG_CORE::areConfigsIdentical(void) {
	if (a_cfg.iron_temp 		!= s_cfg.iron_temp) 		return false;
	if (a_cfg.gun_temp 			!= s_cfg.gun_temp) 			return false;
	if (a_cfg.gun_fan_speed 	!= s_cfg.gun_fan_speed)		return false;
	if (a_cfg.low_temp			!= s_cfg.low_temp)			return false;
	if (a_cfg.low_to			!= s_cfg.low_to)			return false;
	if (a_cfg.tip 				!= s_cfg.tip)				return false;
	if (a_cfg.off_timeout 		!= s_cfg.off_timeout)		return false;
	if (a_cfg.bit_mask			!= s_cfg.bit_mask)			return false;
	if (a_cfg.scr_save_timeout	!= s_cfg.scr_save_timeout)	return false;
	if (a_cfg.boost				!= s_cfg.boost)				return false;
	return true;
};

// Find the tip_chunk_index in the TIP EEPROM AREA which is not used
uint8_t	CFG::freeTipChunkIndex(void) {
	for (uint8_t index = 0; index < tipDataTotal(); ++index) {
		bool chunk_allocated = false;
		for (uint8_t i = 0; i < TIPS::loaded(); ++i) {
			if (tip_table[i].tip_chunk_index == index) {
				chunk_allocated = true;
				break;
			}
		}
		if (!chunk_allocated) {
			return index;
		}
	}
	// Try to find not active TIP
	for (uint8_t i = 0; i < TIPS::loaded(); ++i) {
		if (tip_table[i].tip_chunk_index != NO_TIP_CHUNK) {
			if (!(tip_table[i].tip_mask & TIP_ACTIVE)) {	// The data is allocated for tip, but tip is not activated
				tip_table[i].tip_chunk_index 	= NO_TIP_CHUNK;
				tip_table[i].tip_mask			= 0;
				return i;
			}
		}
	}
	return NO_TIP_CHUNK;
}

//---------------------- CORE_CFG class functions --------------------------------
void CFG_CORE::setDefaults(void) {
	a_cfg.iron_temp			= 235;
	a_cfg.gun_temp			= 300;
	a_cfg.gun_fan_speed		= 1200;
	a_cfg.tip				= 1;							// The first IRON tip. Tip #0 is the Hot Air Gun
	a_cfg.off_timeout		= 0;
	a_cfg.low_temp			= 0;
	a_cfg.low_to			= 5;
	a_cfg.bit_mask			= CFG_CELSIUS | CFG_BUZZER;
	a_cfg.scr_save_timeout	= 0;
	a_cfg.boost				= 0;
	a_cfg.iron_Kp			= 2300;
	a_cfg.iron_Ki			=   50;
	a_cfg.iron_Kd			=  735;
	a_cfg.gun_Kp			=  200;
	a_cfg.gun_Ki			=   64;
	a_cfg.gun_Kd			=  195;
}

void CFG_CORE::correctConfig(RECORD *cfg) {
	uint16_t iron_tempC = cfg->iron_temp;
	uint16_t gun_tempC	= cfg->gun_temp;
	if (!(cfg->bit_mask & CFG_CELSIUS)) {
		iron_tempC	= fahrenheitToCelsius(iron_tempC);
		gun_tempC	= fahrenheitToCelsius(gun_tempC);
	}
	iron_tempC	= constrain(iron_tempC, iron_temp_minC, iron_temp_maxC);
	gun_tempC	= constrain(gun_tempC,  gun_temp_minC,  gun_temp_maxC);
	if (!(cfg->bit_mask & CFG_CELSIUS)) {
		iron_tempC	= celsiusToFahrenheit(iron_tempC);
		gun_tempC 	= celsiusToFahrenheit(gun_tempC);
	}
	cfg->iron_temp		= iron_tempC;
	cfg->gun_temp		= gun_tempC;
	if (cfg->off_timeout > 30)		cfg->off_timeout 		= 30;
	if (cfg->tip > TIPS::loaded())	cfg->tip 				= 1;
	if (cfg->scr_save_timeout > 60) cfg->scr_save_timeout 	= 60;
}

// Apply main configuration parameters: automatic off timeout, buzzer and temperature units
void CFG_CORE::setup(uint8_t off_timeout, bool buzzer, bool celsius, bool keep_iron, uint16_t low_temp, uint8_t low_to, uint8_t scr_saver) {
	bool cfg_celsius		= a_cfg.bit_mask & CFG_CELSIUS;
	a_cfg.off_timeout		= off_timeout;
	a_cfg.scr_save_timeout	= scr_saver;
	a_cfg.low_temp			= low_temp;
	if (low_to < 5) low_to = 5;
	a_cfg.low_to			= low_to;
	if (cfg_celsius	!= celsius) {							// When we change units, the temperature should be converted
		if (celsius) {										// Translate preset temp. from Fahrenheit to Celsius
			a_cfg.iron_temp	= fahrenheitToCelsius(a_cfg.iron_temp);
			a_cfg.gun_temp	= fahrenheitToCelsius(a_cfg.gun_temp);
		} else {											// Translate preset temp. from Celsius to Fahrenheit
			a_cfg.iron_temp	= celsiusToFahrenheit(a_cfg.iron_temp);
			a_cfg.gun_temp	= celsiusToFahrenheit(a_cfg.gun_temp);
		}
	}
	a_cfg.bit_mask	= 0;
	if (celsius)	a_cfg.bit_mask |= CFG_CELSIUS;
	if (buzzer)		a_cfg.bit_mask |= CFG_BUZZER;
	if (keep_iron)	a_cfg.bit_mask |= CFG_KEEP_IRON;
}

void CFG_CORE::savePresetTempHuman(uint16_t temp_set) {
	a_cfg.iron_temp = temp_set;
}

void CFG_CORE::saveGunPreset(uint16_t temp_set, uint16_t fan) {
	a_cfg.gun_temp 		= temp_set;
	a_cfg.gun_fan_speed	= fan;
}

void CFG_CORE::syncConfig(void)	{
	memcpy(&s_cfg, &a_cfg, sizeof(RECORD));
}

void CFG_CORE::restoreConfig(void) {
	memcpy(&a_cfg, &s_cfg, sizeof(RECORD));					// restore configuration from spare copy
}

/*
 * Boost is a bit map. The upper 4 bits are boost increment temperature (n*5 Celsius), i.e.
 * 0000 - disabled
 * 0001 - +5  degrees
 * 1111 - +75 degrees
 * The lower 4 bits is the boost time ((n+1)* 5 seconds), i.e.
 * 0000 -  5 seconds
 * 0001 - 10 seconds
 * 1111 - 80 seconds
 */

uint8_t	CFG_CORE::boostTemp(void){
	uint8_t t = a_cfg.boost >> 4;
	return t * 5;
}

uint8_t	CFG_CORE::boostDuration(void) {
	uint8_t d = a_cfg.boost & 0xF;
	return (d+1)*5;
}

// Save boost parameters to the current configuration
void CFG_CORE::saveBoost(uint8_t temp, uint8_t duration) {
	if (temp > 75)		temp = 75;
	if (duration > 80)	duration = 80;
	if (duration < 5)   duration = 5;
	temp += 4;
	temp /= 5;
	a_cfg.boost = temp << 4;
	a_cfg.boost &= 0xF0;
	a_cfg.boost |= ((duration-1)/5) & 0xF;
}

// PID parameters: Kp, Ki, Kd
PIDparam CFG_CORE::pidParams(bool iron) {
	if (iron)
		return PIDparam(a_cfg.iron_Kp, a_cfg.iron_Ki, a_cfg.iron_Kd);
	else
		return PIDparam(a_cfg.gun_Kp, a_cfg.gun_Ki, a_cfg.gun_Kd);
}

// PID parameters: Kp, Ki, Kd for smooth work, i.e. tip calibration
PIDparam CFG_CORE::pidParamsSmooth(bool iron) {
	if (iron)
		return PIDparam(575, 10, 200);
	else
		return PIDparam(150, 64, 50);
}

//---------------------- CORE_CFG class functions --------------------------------
void TIP_CFG::load(const TIP& ltip, bool gun) {
	uint8_t i = uint8_t(gun);
	tip[i].calibration[0]	= ltip.t200;
	tip[i].calibration[1]	= ltip.t260;
	tip[i].calibration[2]	= ltip.t330;
	tip[i].calibration[3]	= ltip.t400;
	tip[i].mask				= ltip.mask;
	tip[i].ambient			= ltip.ambient;
}

void TIP_CFG::dump(TIP* ltip, bool gun) {
	uint8_t i = uint8_t(gun);
	ltip->t200		= tip[i].calibration[0];
	ltip->t260		= tip[i].calibration[1];
	ltip->t330		= tip[i].calibration[2];
	ltip->t400		= tip[i].calibration[3];
	ltip->mask		= tip[i].mask;
	ltip->ambient	= tip[i].ambient;
}

int8_t TIP_CFG::ambientTemp(void) {
	uint8_t i = uint8_t(gun_active);
	return tip[i].ambient;
}

uint16_t TIP_CFG::calibration(uint8_t index) {
	if (index >= 4)
		return 0;
	uint8_t i = uint8_t(gun_active);
	return tip[i].calibration[index];
}

void TIP_CFG::activateGun(bool gun) {
	if (gun) {
		t_minC	= gun_temp_minC;
		t_maxC	= gun_temp_maxC;
	} else {
		t_minC	= iron_temp_minC;
		t_maxC	= iron_temp_maxC;
	}
	gun_active = gun;
}

uint16_t TIP_CFG::referenceTemp(uint8_t index, CFG_TEMP_DEVICE force_device) {
	if (index >= 4)
		return 0;

	bool gun = gun_active;
	if (force_device != DEV_DEFAULT) {
		gun = (force_device == DEV_GUN);
	}
	if (gun)
		return temp_ref_gun[index];
	else
		return temp_ref_iron[index];
}

// Translate the internal temperature of the IRON or Hot Air Gun to Celsius
uint16_t TIP_CFG::tempCelsius(uint16_t temp, int16_t ambient, CFG_TEMP_DEVICE force_device) {
	uint8_t i 		= uint8_t(gun_active);						// Select appropriate calibration tip or gun
	int16_t tempH 	= 0;
	if (force_device != DEV_DEFAULT) {
		i = (force_device == DEV_GUN)? 1: 0;
	}

	// The temperature difference between current ambient temperature and ambient temperature during tip calibration
	int d = ambient - tip[i].ambient;
	if (temp < tip[i].calibration[0]) {							// less than first calibration point
	    tempH = map(temp, 0, tip[i].calibration[0], ambient, referenceTemp(0, force_device)+d);
	} else {
		if (temp <= tip[i].calibration[3]) {					// Inside calibration interval
			for (uint8_t j = 1; j < 4; ++j) {
				if (temp < tip[i].calibration[j]) {
					tempH = map(temp, tip[i].calibration[j-1], tip[i].calibration[j],
							referenceTemp(j-1, force_device)+d, referenceTemp(j, force_device)+d);
					break;
				}
			}
		} else {											// Greater than maximum
			tempH = map(temp, tip[i].calibration[1], tip[i].calibration[3],
					referenceTemp(1, force_device)+d, referenceTemp(3, force_device)+d);
		}
	}
	tempH = constrain(tempH, ambient, 999);
	return tempH;
}

// Return the reference temperature points of the IRON tip calibration
void TIP_CFG::getTipCalibtarion(uint16_t temp[4]) {
	uint8_t i = uint8_t(gun_active);
	for (uint8_t j = 0; j < 4; ++j)
		temp[j]	= tip[i].calibration[j];
}

// Apply new IRON tip calibration data to the current configuration
void TIP_CFG::applyTipCalibtarion(uint16_t temp[4], int8_t ambient) {
	uint8_t i = uint8_t(gun_active);
	for (uint8_t j = 0; j < 4; ++j)
		tip[i].calibration[j]	= temp[j];
	tip[i].ambient	= ambient;
	tip[i].mask		= TIP_CALIBRATED | TIP_ACTIVE;
	if (tip[i].calibration[3] > int_temp_max) tip[i].calibration[3] = int_temp_max;
}

// Initialize the tip calibration parameters with the default values
void TIP_CFG::resetTipCalibration(void) {
	defaultCalibration(gun_active);
}

// Apply default calibration parameters of the tip; Prevent overheating of the tip
void TIP_CFG::defaultCalibration(bool gun) {
	uint8_t i = uint8_t(gun);
	tip[i].calibration[0]	=  680;
	tip[i].calibration[1]	=  964;
	tip[i].calibration[2]	= 1290;
	tip[i].calibration[3]	= 1600;
	tip[i].ambient			= default_ambient;					// vars.cpp
	tip[i].mask				= TIP_ACTIVE;
}

bool TIP_CFG::isValidTipConfig(TIP *tip) {
	return (tip->t200 < tip->t260 && tip->t260 < tip->t330 && tip->t330 < tip->t400);
}
