/*
 * buzzer.h
 *
 *  Created on: 14 рту. 2019 у.
 *      Author: Alex
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#ifndef __BUZZ_H
#define __BUZZ_H
#include "main.h"

class BUZZER {
	public:
		BUZZER(void);
		void		lowBeep(void);
		void		shortBeep(void);
		void		doubleBeep(void);
		void		failedBeep(void);
	private:
		void		playTone(uint16_t period_mks, uint16_t duration_ms);
};

#endif

#endif /* BUZZER_H_ */
