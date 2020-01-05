/*
 * eeprom.h
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 *
 * The data in the EEPROM is addressed by chunks.
 * There are 128 chunks of 32 bytes in the EEPROM IC at24c32a.
 * First 64 chunks [0-63] are used to store configuration data.
 * One record per chunk as soon the configuration record can fit into one chunk.
 * To save EEPROM rewrite cycles, new record is written to the next free chunk, increasing record ID.
 * When the controller starts, it reads all the chunks in the configuration area and find the last record
 * that has the biggest record ID.
 *
 * Last 64 chunks [64-127] are used to store the tip configuration data.
 * As soon as tip configuration requires only 16 bytes, two records can fit to the chunk.
 * Only active and calibrated tips are stored in this area.
 * When the controller starts, it reads all the chunks in the tip area and builds tip configuration table (tip_table, see config.c).
 * The tip_table tip_chunk_index field is the index of the tip in tip configuration area.
 * index = 0 means the first (of two) record in the first tip configuration chunk (64 chunk of the EEPROM).
 * index = 1 means the second record record in the first tip configuration chunk (64 chunk of the EEPROM).
 *
 * For chunk manipulations two functions are used: readChunk() and writeChunk().
 * These functions read and write the EEPROM chunk from/to static data buffer.
 * To increase performance, last read and written chunk index is stored to chunk_in_data variable.
 * readChunk( function returns immediately, if data in the buffer is already actual.
 */

#ifndef EEPROM_H_
#define EEPROM_H_
#include "main.h"
#include "cfgtypes.h"

typedef enum tip_io_status {EPR_OK = 0, EPR_IO, EPR_CHECKSUM, EPR_INDEX} TIP_IO_STATUS;

#define eeprom_chunk_size	(32)						// Number of bytes in one EEPROM chunk

class EEPROM {
	public:
		EEPROM(I2C_HandleTypeDef* pHi2c)				{ hi2c = pHi2c; }
		bool			init();
		uint16_t 		tipDataTotal(void);
		bool			loadRecord(RECORD* config_record);
		bool 			saveRecord(RECORD* config_record);	// Modifies the record: increment the ID and calculate CRC
		TIP_IO_STATUS 	loadTipData(TIP* tip, uint8_t tip_chunk_index);
		TIP_IO_STATUS	saveTipData(TIP* tip, uint8_t tip_chunk_index);
		void 			clearConfigArea(void);
		void			forceReloadChunk(void)			{ chunk_in_data	= 65535; }
	private:
		bool 			readChunk(uint16_t chunk_index);
		bool 			writeChunk(uint16_t chunk_index);
		uint8_t 		CFG_checkSum(RECORD* cfg, bool write);
		uint8_t 		TIP_checkSum(TIP* tip, bool write);
		uint16_t 		requiredTipSpace(void);
		I2C_HandleTypeDef* 	hi2c	= 0;
		bool		can_write				= false;	// The flag indicates that data can be saved to the EEPROM
		uint16_t	r_chunk					= 0;		// Chunk number of the correct record in EEPROM to be read
		uint16_t	w_chunk					= 0;		// Chunk number in the EEPROM to start write new record
		uint8_t  	data[eeprom_chunk_size];			// Data buffer for one EEPROM chunk
		uint16_t	chunk_in_data			= 65535;	// Current chunk number in the data buffer [0-(eeprom_chunks-1)]. For caching
		const uint16_t		eeprom_chunks 	= 128;		// The number of chunks in my EEPROM IC
		const uint16_t  	eeprom_address 	= 0x50;		// AT24C32 EEPROM IC address on the I2C bus
		const uint16_t		cfg_chunks		= 64;		// The space of EEPROM (in chunks) dedicated to the configuration data
		const uint16_t		tip_chunks		= 64;		// The maximum number of chunks used to store the configured tips
};

#endif
