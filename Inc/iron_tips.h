/*
 * iron_tips.h
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 */

#ifndef IRON_TIPS_H_
#define IRON_TIPS_H_
#include "main.h"

// The length of the tip name
#define	 	tip_name_sz		(5)

class TIPS {
	public:
		TIPS()													{ }
		uint16_t		loaded(void);
		const char* 	name(uint8_t index);
		int 			index(const char *name);
};


#endif
