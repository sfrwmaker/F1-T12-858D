/*
 * encoder.h
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 */
 
#ifndef ENCODER_H_
#define ENCODER_H_
#include "main.h"
#include "stat.h"

class RENC {
	public:
		RENC(GPIO_TypeDef* aPORT, uint16_t aPIN, GPIO_TypeDef* bPORT, uint16_t bPIN);
		void 		addButton(GPIO_TypeDef* ButtonPORT, uint16_t ButtonPIN);
		uint8_t		buttonStatus(void);
		bool		write(int16_t initPos);
		void    	reset(int16_t initPos, int16_t low, int16_t upp, uint8_t inc, uint8_t fast_inc, bool looped);
		void 		encoderIntr(void);
		void 		setTimeout(uint16_t timeout_ms)			{ over_press = timeout_ms; }
		void    	setIncrement(uint8_t inc)           	{ increment = fast_increment = inc; }
		uint8_t		getIncrement(void)                 		{ return increment; }
		int16_t 	read(void)                          	{ return pos; }
	private:
		EMP_AVERAGE			avg;							// Do average the button readings to mainain the button status
		int16_t				min_pos	= 0;					// Minimum value of rotary encoder
		int16_t				max_pos	= 0;					// Maximum value of roraty encoder
		uint16_t			over_press = 0;					// Maximum time in ms the button can be pressed
		bool              	is_looped = false;          	// Whether the encoder is looped
		uint8_t            	increment = 0;              	// The value to add or substract for each encoder tick
		uint8_t             fast_increment = 0;         	// The value to change encoder when it runs quickly
		volatile uint32_t 	rpt		= 0;                	// Time in ms when the encoder was rotated
		volatile uint32_t 	changed	= 0;                	// Time in ms when the value was changed
		volatile int16_t  	pos		= 0;                	// Encoder current position
		volatile bool       s_up	= false;				// The status of the secondary channel
		bool				i_b_rel	= false;				// Ignore button release event
		bool				b_on	= false;				// The button current position: true - pressed
		uint32_t 			bpt		= 0;                	// Time in ms when the button was pressed (press time)
		uint32_t			b_check	= 0;					// Time in ms when the button should be checked
		GPIO_TypeDef* 		b_port	= 0;					// The PORT of the press button
		GPIO_TypeDef*     	m_port	= 0;					// The PORT of the main channel
		GPIO_TypeDef*		s_port	= 0;          			// The PORT of the secondary channel
		uint16_t			b_pin	= 0;					// The PIN number of the button
		uint16_t			m_pin	= 0;					// The PIN number of the main channel
		uint16_t			s_pin	= 0;	    			// The PIN number of the secondary channel
        const uint8_t     	trigger_on		= 100;			// avg limit to change button status to on
        const uint8_t     	trigger_off 	= 50;			// avg limit to change button status to off
        const uint8_t     	avg_length   	= 4;			// avg length
        const uint8_t		b_check_period	= 20;			// The button check period, ms
		const uint16_t 		long_press		= 1500;			// If the button was pressed more that this timeout, we assume the long button press
		const uint16_t		fast_timeout	= 300;			// Time in ms to change encoder quickly
		const uint16_t		def_over_press	= 2500;			// Default value for button over press timeout (ms)
};

#endif
