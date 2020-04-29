/*
 * config.h
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include "main.h"
#include "pid.h"
#include "eeprom.h"
#include "cfgtypes.h"
#include "iron_tips.h"
#include "pid.h"
#include "buzzer.h"

typedef enum cfg_status {CFG_OK = 0, CFG_NO_TIP, CFG_READ_ERROR} CFG_STATUS;
typedef enum temp_force {DEV_DEFAULT = 0, DEV_IRON = 1, DEV_GUN = 2} CFG_TEMP_DEVICE;

/*
 * The actual configuration record is loaded from the EEPROM chunk into a_cfg variable
 * The spare copy of the  configuration record is preserved into s_cfg variable
 * When update request arrives, configuration record writes into EEPROM if spare copy is different from actual copy
 */
class CFG_CORE: public TIPS {
	public:
		CFG_CORE(void)									{ }
		bool		isCelsius(void) 					{ return a_cfg.bit_mask & CFG_CELSIUS;	}
		bool		isBuzzerEnabled(void)				{ return a_cfg.bit_mask & CFG_BUZZER; 	}
		bool		isKeepIron(void)					{ return a_cfg.bit_mask & CFG_KEEP_IRON;}
		bool		isReedType(void)					{ return a_cfg.bit_mask & CFG_SWITCH;	}
		uint16_t	tempPresetHuman(void) 				{ return a_cfg.iron_temp;				}
		uint16_t	gunTempPreset(void)					{ return a_cfg.gun_temp;				}
		uint16_t	gunFanPreset(void)					{ return a_cfg.gun_fan_speed;			}
		uint8_t		getOffTimeout(void) 				{ return a_cfg.off_timeout; 			}
		uint16_t	getLowTemp(void)					{ return a_cfg.low_temp; 				}
		uint8_t		getLowTO(void)						{ return a_cfg.low_to; 					}	// 5-seconds intervals
		uint8_t		getScrTo(void)						{ return a_cfg.scr_save_timeout;		}
		void		setup(uint8_t off_timeout, bool buzzer, bool celsius, bool keep_iron, bool reed, uint16_t low_temp, uint8_t low_to, uint8_t scr_saver);
		void 		savePresetTempHuman(uint16_t temp_set);
		void		saveGunPreset(uint16_t temp, uint16_t fan = 0);
		uint8_t		boostTemp(void);
		uint8_t		boostDuration(void);
		void		saveBoost(uint8_t temp, uint8_t duration);
		void		restoreConfig(void);
		PIDparam	pidParams(bool iron);
		PIDparam 	pidParamsSmooth(bool iron = true);
	protected:
		void		setDefaults(void);
		void		correctConfig(RECORD *cfg);
		void		syncConfig(void);
		bool		areConfigsIdentical(void);
		RECORD		a_cfg;								// active configuration
	private:
		RECORD		s_cfg;								// spare configuration, used when save the configuration to the EEPROM
};

typedef struct s_TIP_RECORD	TIP_RECORD;
struct s_TIP_RECORD {
	uint16_t	calibration[4];
	uint8_t		mask;
	int8_t		ambient;
};

class TIP_CFG {
	public:
		TIP_CFG(void)									{ }
		bool 		isTipCalibrated(void) 				{ return tip[0].mask & TIP_CALIBRATED; 	}
		uint16_t	tempMinC(void)						{ return t_minC;						}
		uint16_t	tempMaxC(void)						{ return t_maxC;						}
		bool		gunActive(void)						{ return gun_active;					}
		void		load(const TIP& tip, bool gun = false);
		void		dump(TIP* tip, bool gun = false);
		int8_t		ambientTemp(void);
		uint16_t	calibration(uint8_t index);
		void		activateGun(bool gun);				// Activate Hot Air Gun calibration (or return to the current tip)
		uint16_t	referenceTemp(uint8_t index, CFG_TEMP_DEVICE force_device = DEV_DEFAULT);
		uint16_t	tempCelsius(uint16_t temp, int16_t ambient, CFG_TEMP_DEVICE force_device = DEV_DEFAULT);
		void		getTipCalibtarion(uint16_t temp[4]);
		void		applyTipCalibtarion(uint16_t temp[4], int8_t ambient);
		void		resetTipCalibration(void);
	protected:
		void 		defaultCalibration(bool gun = false);
		bool		isValidTipConfig(TIP *tip);
	private:
		TIP_RECORD	tip[2];								// Active IRON tip (0) and Hot Air Gun virtual tip (1)
		uint16_t	t_minC				= 0;
		uint16_t	t_maxC				= 0;
		bool		gun_active			= false;		// Current Working mode: Hot Air Gun or soldering IRON
		const uint16_t	temp_ref_iron[4]	= { 200, 260, 330, 400};
		const uint16_t	temp_ref_gun[4]		= { 200, 300, 400, 500};
};

class CFG : public EEPROM, public CFG_CORE, public TIP_CFG, public BUZZER {
	public:
		CFG(I2C_HandleTypeDef* pHi2c): EEPROM(pHi2c) 	{ }
		CFG_STATUS	init(void);
		uint16_t 	tipChunksTotal(void);
		uint16_t	tempToHuman(uint16_t temp, int16_t ambient, CFG_TEMP_DEVICE force_device = DEV_DEFAULT);
		uint16_t	humanToTemp(uint16_t temp, int16_t ambient);
		const char* tipName(void);
		void     	changeTip(uint8_t index);
		uint8_t		currentTipIndex(void);
		void		saveTipCalibtarion(uint8_t index, uint16_t temp[4], uint8_t mask, int8_t ambient);
		bool		toggleTipActivation(uint8_t index);
		int			tipList(uint8_t second, TIP_ITEM list[], uint8_t list_len, bool active_only);
		void		saveConfig(void);
		void		savePID(PIDparam &pp, bool iron = true);
		void 		initConfigArea(void);
		void		clearAllTipsCalibration(void);
	private:
		bool 		selectTip(uint8_t index);
		uint8_t		buildTipTable(TIP_TABLE tt[]);
		char* 		buildFullTipName(char tip_name[tip_name_sz], const uint8_t index);
		uint8_t		freeTipChunkIndex(void);
		bool 		isTipCorrect(uint8_t tip_chunk_index, TIP *tip);
		TIP_TABLE	*tip_table = 0;						// Tip table - chunk number of the tip or 0xFF if does not exist in the EEPROM
};

#endif
