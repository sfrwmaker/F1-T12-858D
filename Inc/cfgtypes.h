/*
 * cfgtypes.h
 *
 *  Created on: 15 aug. 2019
 *      Author: Alex
 */

#ifndef CFGTYPES_H_
#define CFGTYPES_H_
#include "iron_tips.h"

/*
 * The configuration bit map:
 * CFG_CELSIUS		- The temperature units: Celsius (1) or Fahrenheit (0)
 * CFG_BUZZER		- Is the Buzzer Enabled (1)
 * CFG_KEEP_IRON	- Is keep the iron working while in Hot Air Gun mode
 */
typedef enum { CFG_CELSIUS = 1, CFG_BUZZER = 2, CFG_KEEP_IRON = 4 } CFG_BIT_MASK;

/* Configuration record in the EEPROM (after the tip table) has the following format:
 * Records are aligned by 2**n bytes (in this case, 32 bytes)
 *
 * Boost is a bit map. The upper 4 bits are boost increment temperature (n*5 Celsius), i.e.
 * 0000 - disabled
 * 0001 - +4  degrees
 * 1111 - +75 degrees
 * The lower 4 bits is the boost time ((n+1)* 5 seconds), i.e.
 * 0000 -  5 seconds
 * 0001 - 10 seconds
 * 1111 - 80 seconds
 */
typedef struct s_config RECORD;
struct s_config {
	uint32_t	ID;									// The configuration record ID
	uint16_t	crc;								// The checksum
	uint16_t	iron_temp;							// The IRON preset temperature in degrees (Celsius or Fahrenheit)
	uint16_t	gun_temp;							// The Hot Air Gun preset temperature in degrees (Celsius or Fahrenheit)
	uint16_t	gun_fan_speed;						// The Hot Air Gun fan speed
	uint16_t	iron_Kp, iron_Ki, iron_Kd;			// The IRON PID coefficients
	uint16_t	gun_Kp,  gun_Ki,  gun_Kd;			// The Hot Air Gun PID coefficients
	uint16_t	low_temp;							// The low power temperature (C) or 0 if the tilt sensor is disabled
	uint8_t		low_to;								// The low power timeout (seconds)
	uint8_t		scr_save_timeout;					// The screen saver timeout (in minutes) [0-60]. Zero if disabled
	uint8_t		boost;								// Two 4-bits parameters: The boost increment temperature and boost time. See description above
	uint8_t		tip;								// Current tip index
	uint8_t		off_timeout;						// The Automatic switch-off timeout in minutes [0 - 30]
	uint8_t		bit_mask;							// See CFG_BIT_MASK
};

/* Configuration data of each initialized tip are saved in the upper area of the EEPROM.
 * Two tip record per one EEPROM chunk, as soon each tip recored requires 16 bytes only.
 * The tip configuration record has the following format:
 * 4 reference temperature points
 * tip status bitmap
 * tip suffix name
 */

typedef struct s_tip TIP;
struct s_tip {
	uint16_t	t200, t260, t330, t400;				// The internal temperature in reference points
	uint8_t		mask;								// The bit mask: TIP_ACTIVE + TIP_CALIBRATED
	char		name[tip_name_sz];					// T12 tip name suffix, JL02 for T12-JL02
	int8_t		ambient;							// The ambient temperature in Celsius when the tip being calibrated
	uint8_t		crc;								// CRC checksum
};

// This tip structure is used to show available tips when tip is activating
typedef struct s_tip_list_item	TIP_ITEM;
struct s_tip_list_item {
	uint8_t		tip_index;							// Index of the tip in the global list in EEPROM
	uint8_t		mask;								// The bit mask: 0 - active, 1 - calibrated
	char		name[tip_name_sz+5];				// Complete tip name, i.e. T12-***
};

/*
 * This structure presents a tip record for all possible tips, declared in iron_tips.c
 * During controller initialization phase, the buildTipTable() function creates
 * the tip list in memory of all possible tips. If the tip is calibrated, i.e. has a record
 * in the upper area of EEPROM, the tip record saves chunk number, where the calibration data resides
 */
typedef struct s_tip_table		TIP_TABLE;
struct s_tip_table {
	uint8_t		tip_chunk_index;					// The tip chunk index in the EEPROM
	uint8_t		tip_mask;							// The bit mask: 0 - active, 1 - calibrated
};

typedef enum tip_status { TIP_ACTIVE = 1, TIP_CALIBRATED = 2 } TIP_STATUS;

#endif
