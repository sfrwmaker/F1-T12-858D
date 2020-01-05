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
	avg.length(avg_length);
}

void RENC::reset(int16_t initPos, int16_t low, int16_t upp, uint8_t inc, uint8_t fast_inc, bool looped) {
	min_pos = low; max_pos = upp;
	if (!write(initPos)) initPos = min_pos;
	increment = fast_increment = inc;
	if (fast_inc > increment) fast_increment = fast_inc;
	is_looped = looped;
}

/*
 * The Encoder button current status
 * 0	- not pressed
 * 1	- short press
 * 2	- long press
 */
uint8_t	RENC::buttonStatus(void) {
	if (HAL_GetTick() >= b_check) {							// It is time to check the button status
		b_check = HAL_GetTick() + b_check_period;
		uint8_t s = 0;
		if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(b_port, b_pin))	// if port state is low, the button pressed
			s = trigger_on << 1;
		if (b_on) {
			if (avg.average(s) < trigger_off)
				b_on = false;
		} else {
			if (avg.average(s) > trigger_on)
				b_on = true;
		}

		if (b_on) {                                           	// Button status is 'pressed'
			uint32_t n = HAL_GetTick() - bpt;
			if ((bpt == 0) || (n > over_press)) {
				bpt = HAL_GetTick();
			} else if (n > long_press) {                    	// Long press
				if (i_b_rel) {
					return 0;
				} else{
					i_b_rel = true;
					return 2;
				}
			}
		} else {                                            	// Button status is 'not pressed'
			if (bpt == 0 || i_b_rel) {
				bpt = 0;
				i_b_rel = false;
				return 0;
			}
			uint32_t e = HAL_GetTick() - bpt;
			bpt = 0;											// Ready for next press
			if (e < over_press) {								// Long press already managed
				return 1;
			}
		}
	}
    return 0;
}

bool RENC::write(int16_t initPos)	{
	if ((initPos >= min_pos) && (initPos <= max_pos)) {
		pos = initPos;
		return true;
	}
	return false;
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
