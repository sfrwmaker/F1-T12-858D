/*
 * stat.h
 *
 *  Created on: 13 рту. 2019 у.
 *      Author: Alex
 *  Math statistic class
 */

#ifndef STAT_H_
#define STAT_H_

#include "main.h"

// Exponential average
class EMP_AVERAGE {
	public:
		EMP_AVERAGE(uint8_t h_length = 8)				{ emp_k = h_length; emp_data = 0; }
		void			length(uint8_t h_length)		{ emp_k = h_length; emp_data = 0; }
		void			reset(void)						{ emp_data = 0; }
		int32_t			average(int32_t value);
		void			update(int32_t value);
		int32_t			read(void);
	private:
		volatile	uint8_t 	emp_k 		= 8;
		volatile	uint32_t	emp_data	= 0;
};

#define H_LENGTH (16)
// Flat history data with round buffer
class HIST {
	public:
    	HIST(uint8_t h_length = H_LENGTH)				{ len = index = 0; max_len = h_length; }
    	void			length(uint8_t h_length)		{ len = index = 0; if (h_length > H_LENGTH) h_length = H_LENGTH; max_len = h_length; }
    	void			reset()							{ len = index = 0; }
    	int32_t			read(void);
    	int32_t			average(int32_t value);
    	void			update(int32_t value);
    	uint32_t		dispersion(void);               // the math dispersion of the data
    private:
    	volatile int32_t 	queue[H_LENGTH];
    	volatile uint8_t	len;						// The number of elements in the queue
    	volatile uint8_t	max_len;					// Maximum length of the queue, not greater than H_LENGTH
    	volatile uint8_t 	index;						// The current element position, use ring buffer
};

class SWITCH : public EMP_AVERAGE {
    public:
        SWITCH(uint8_t len=8) : EMP_AVERAGE(len)			{ }
        void        init(uint8_t h_len, uint16_t on = 500, uint16_t off = 500);
        bool        status(void);
        void		update(uint16_t value);
    private:
        bool        mode	 = false;               		// The switch mode on (true)/off
        int16_t    	on_val  = 500;                 			// Turn on  value
        int16_t    	off_val = 500;                 			// Turn off value
};

#endif
