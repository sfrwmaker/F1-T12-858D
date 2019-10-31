/*
 * encoder.cpp
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 */

#include "encoder.h"

RENC::RENC(GPIO_TypeDef* aPORT, uint16_t aPIN, GPIO_TypeDef* bPORT, uint16_t bPIN) {
	rpt 		= 0;
	m_port 		= aPORT; s_port = bPORT; m_pin = aPIN; s_pin = bPIN;
	pos 		= 0;
	min_pos 	= -32767; max_pos = 32766; increment = 1;
	changed 	= 0;
	s_up		= false;
	is_looped	= false;
	increment	= fast_increment = 1;
}

void RENC::addButton(GPIO_TypeDef* ButtonPORT, uint16_t ButtonPIN) {
	bpt 		= 0;
	b_port 		= ButtonPORT;
	b_pin  		= ButtonPIN;
	over_press	= def_over_press;
}

void RENC::reset(int16_t initPos, int16_t low, int16_t upp, uint8_t inc, uint8_t fast_inc, bool looped) {
	min_pos = low; max_pos = upp;
	if (!write(initPos)) initPos = min_pos;
	increment = fast_increment = inc;
	if (fast_inc > increment) fast_increment = fast_inc;
	is_looped = looped;
}

uint8_t	RENC::buttonStatus(void) {
	if (bpt > 0) {											// The button was pressed
		if ((HAL_GetTick() - bpt) >= long_press) {
			mode 	= 0;
			if (i_b_rel)									// Already long press event was detected
				return 0;
			i_b_rel	= true;									// Ignore button next release event
			return 2;										// Long press
		}
	}
	uint8_t m = mode; mode = 0;
	return m;
}

bool RENC::write(int16_t initPos)	{
	if ((initPos >= min_pos) && (initPos <= max_pos)) {
		pos = initPos;
		return true;
	}
	return false;
}

void RENC::buttonIntr(void) {                 				// Interrupt function, called when the button status changed
	uint32_t now_t = HAL_GetTick();
	bool keyUp = (HAL_GPIO_ReadPin(b_port, b_pin) == GPIO_PIN_SET);
	if (!keyUp) {                                  			// The button has been pressed
		if ((bpt == 0) || (now_t - bpt > over_press)) {
			bpt 	= now_t;
			i_b_rel = false;
		}
	} else {												// The button was released
		if (bpt > 0) {										// The button has been pressed
			if ((now_t - bpt) < bounce) {
				i_b_rel = false;
				return;										// Prevent bouncing
			}
			else if ((now_t - bpt) < long_press)
				mode = 1; 									// short press
			else if ((now_t - bpt) < over_press)
				mode = 2;                     				// long press
			else
				mode = 0;									// over press
			bpt = 0;
		}
		if (i_b_rel) {
			i_b_rel = false;
			mode = 0;
		}
	}
}

void RENC::encoderIntr(void) {					// Interrupt function, called when the channel A of encoder changed
	bool mUp = (HAL_GPIO_ReadPin(m_port, m_pin) == GPIO_PIN_SET);
	uint32_t now_t = HAL_GetTick();
	if (!mUp) {                                     // The main channel has been "pressed"
		if ((rpt == 0) || (now_t - rpt > over_press)) {
			rpt = now_t;
			s_up = (HAL_GPIO_ReadPin(s_port, s_pin) == GPIO_PIN_SET);
    	}
	} else {
		if (rpt > 0) {
				uint8_t inc = increment;
				if ((now_t - rpt) < over_press) {
					if ((now_t - changed) < fast_timeout) inc = fast_increment;
						changed = now_t;
						if (s_up) pos -= inc; else pos += inc;
						if (pos > max_pos) {
							if (is_looped)
								pos = min_pos;
							else
								pos = max_pos;
						}
						if (pos < min_pos) {
							if (is_looped)
								pos = max_pos;
							else
								pos = min_pos;
						}
				}
				rpt = 0;
		}
	}
}
