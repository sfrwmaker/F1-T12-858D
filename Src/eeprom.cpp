/*
 * eeprom.cpp
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 */
 
#include <string.h>
#include <stdlib.h>
#include "eeprom.h"
#include "iron_tips.h"

bool EEPROM::init(void) {
	// Read all the records in the EEPROM and find min and max record IDs
	uint32_t 	min_rec_ID 	= 0xffffffff;
	uint16_t 	min_rec_ch 	= 0;
	uint32_t 	max_rec_ID 	= 0;
	uint16_t 	max_rec_ch 	= 0;
	uint16_t 	records 	= 0;

	if (HAL_OK != HAL_I2C_IsDeviceReady(hi2c, eeprom_address<<1, 2, 2)) {
		can_write = false;
		return can_write;
	}

	can_write = true;
	for (uint16_t chunk = 0; chunk < cfg_chunks; ++chunk) {
		if (readChunk(chunk)) {
			RECORD* cfg = (RECORD*)data;
			if (CFG_checkSum(cfg, false)) {
				++records;
				if (min_rec_ID 	> cfg->ID) {
					min_rec_ID 	= cfg->ID;
					min_rec_ch	= chunk;
				}
				if (max_rec_ID < cfg->ID) {
					max_rec_ID 	= cfg->ID;
					max_rec_ch 	= chunk;
				}
			} else {
				break;
			}
		} else {
			can_write	= false;
			break;
		}
	}

	if (records == 0) {
		w_chunk		= r_chunk = 0;
	    return can_write;
	}

	r_chunk = max_rec_ch;
	if (records < cfg_chunks) {								// The EEPROM is not full
	    w_chunk = r_chunk + 1;
	    if (w_chunk >= cfg_chunks) w_chunk = 0;
	} else {
		w_chunk = min_rec_ch;
	}
	return can_write;
}

uint16_t EEPROM::tipDataTotal(void) {
	uint16_t tip_space 		= requiredTipSpace();
	uint16_t tips_per_chunk = eeprom_chunk_size / tip_space;
	return tip_chunks * tips_per_chunk;
}

bool EEPROM::loadRecord(RECORD* config_record) {
	if (readChunk(r_chunk)) {
		RECORD*   cfg = (RECORD*)data;
		if (CFG_checkSum(cfg, false)) {
			memcpy(config_record, cfg, sizeof(RECORD));
			return true;
		}
	}
	return false;
}

bool EEPROM::saveRecord(RECORD* config_record) {
	if (!can_write) return can_write;

	config_record->ID ++;
	CFG_checkSum(config_record, true);
	memcpy(data, (uint8_t*)config_record, sizeof(RECORD));
	if (writeChunk(w_chunk)) {
		r_chunk = w_chunk;
		if (++w_chunk >= cfg_chunks) w_chunk = 0;
		return true;
	}
	return false;
}

/*
 * Load tip configuration from EEPROM.
 * As soon one tip record fits to 16 bytes, two tips are saved in one chunk
 */
TIP_IO_STATUS EEPROM::loadTipData(TIP* tip, uint8_t tip_chunk_index) {
	uint16_t tip_space 		= requiredTipSpace();
	uint16_t tips_per_chunk = eeprom_chunk_size / tip_space;
	if (tip_chunk_index > tip_chunks * tips_per_chunk)
		return EPR_INDEX;
	uint16_t tip_chunk 		= tip_chunk_index / tips_per_chunk + eeprom_chunks - tip_chunks;
	uint8_t	 index 			= (tip_chunk_index % tips_per_chunk) * tip_space;

	if (readChunk(tip_chunk)) {								// load whole EEPROM chunk
		TIP* tmp_tip = (TIP *)&data[index];					// load tip record (first or second)
		if (TIP_checkSum(tmp_tip, false)) {					// CRC of the tip record is correct
			memcpy(tip, tmp_tip, sizeof(TIP));				// Copy the tip record from the data buffer
			return EPR_OK;
		}
		return EPR_CHECKSUM;
	}
	return EPR_IO;
}

TIP_IO_STATUS EEPROM::saveTipData(TIP* tip, uint8_t tip_chunk_index) {
	uint16_t tip_space 		= requiredTipSpace();
	uint16_t tips_per_chunk = eeprom_chunk_size / tip_space;
	if (tip_chunk_index > tip_chunks * tips_per_chunk)
		return EPR_INDEX;
	uint16_t tip_chunk 		= tip_chunk_index / tips_per_chunk + eeprom_chunks - tip_chunks;
	uint8_t	 index 			= (tip_chunk_index % tips_per_chunk) * tip_space;

	if (readChunk(tip_chunk)) {								// load whole EEPROM chunk
		TIP* tmp_tip = (TIP *)&data[index];					// choose correct record index (1 or 2)
		memcpy(tmp_tip, tip, sizeof(TIP));					// Replace tip configuration in the data buffer
		TIP_checkSum(tmp_tip, true);						// calculate CRC inside the data buffer
		if (writeChunk(tip_chunk))							// Rewrite whole chunk
			return EPR_OK;
	}
	return EPR_IO;											// Here can be any of IO error: read or write
}


// Clear bottom area of the EEPROM, where the configuration data is
void EEPROM::clearConfigArea(void) {
	for (uint8_t i = 0; i < eeprom_chunk_size; ++i)
		data[i] = 0xFF;
	for (int i = 0; i < cfg_chunks; ++i) {
		uint32_t addr = i * eeprom_chunk_size;
		if (HAL_I2C_Mem_Write(hi2c, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, eeprom_chunk_size, 100) != HAL_OK) {
			break;											// Stop writing immediately in case of error
		}
		HAL_Delay(10);										// Let the data to be saved in the EEPROM
	}
	init();
}

// Calculate the space required to store TIP configuration. (defined in config.h). The space size should be multiple by 2**N
uint16_t EEPROM::requiredTipSpace(void) {
	uint16_t tip_sz = sizeof(TIP);
	for (long i = 1; i <= eeprom_chunk_size; i <<= 1) {
		if (i >= tip_sz) {
			return i;
		}
	}
	return eeprom_chunk_size;
}

// Read the EEPROM whole chunk
bool EEPROM::readChunk(uint16_t chunk_index) {
	if (chunk_index == chunk_in_data) return true;
	if (chunk_index >= eeprom_chunks) return false;

	uint16_t addr = chunk_index * eeprom_chunk_size;
	if (HAL_I2C_Mem_Read(hi2c, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, eeprom_chunk_size, 100) == HAL_OK) {
		chunk_in_data = chunk_index;
		return true;
	}
	return false;
}

// Write the EEPROM whole chunk
bool EEPROM::writeChunk(uint16_t chunk_index) {
	if (chunk_index >= eeprom_chunks) return false;

	uint16_t addr = chunk_index * eeprom_chunk_size;
	chunk_in_data = eeprom_chunks;							// Mark the buffer as dirty
	if (HAL_I2C_Mem_Write(hi2c, eeprom_address<<1, addr, I2C_MEMADD_SIZE_16BIT, data, eeprom_chunk_size, 100) == HAL_OK) {
		chunk_in_data = chunk_index;
		HAL_Delay(20);
		return true;
	}
	HAL_Delay(20);
	return false;
}

// Checks the CRC of the RECORD structure. Returns true if OK. Replace the CRC with the correct value if write is true
uint8_t EEPROM::CFG_checkSum(RECORD* cfg, bool write) {
	uint16_t 	summ 		= 117;							// To avoid good check sum with all-zero, start with 117
	uint16_t    rec_summ 	= cfg->crc;
	cfg->crc				= 0;
	uint8_t*	d 			= (uint8_t*)cfg;
	for (uint8_t i = 0; i < sizeof(RECORD); ++i) {
		summ <<= 1; summ += d[i];
	}
	bool res = (rec_summ == summ);
	if (write) cfg->crc = summ;
	return res;
}

// Checks the CRC inside tip structure. Returns true if OK, replaces the CRC with the correct value
uint8_t EEPROM::TIP_checkSum(TIP* tip, bool write) {
	uint32_t summ = tip->t200;
	summ <<= 1; summ += tip->t260;
	summ <<= 1; summ += tip->t330;
	summ <<= 1; summ += tip->t400;
	summ <<= 1; summ += tip->mask;
	summ <<= 1; summ += tip->ambient;
	for (int i = 0; i < tip_name_sz; ++i) {
		summ <<= 1; summ += (uint8_t)tip->name[i];
	}
	summ += 117;											// To avoid good check sum with all-zero
	uint8_t res = (tip->crc == (summ & 0xFF));
	if (write) tip->crc = summ & 0xFF;
	return res;
}
