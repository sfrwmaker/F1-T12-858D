/*
 * stat.cpp
 *
 *  Created on: 13 рту. 2019 у.
 *      Author: Alex
 */

#include <stat.h>

int32_t EMP_AVERAGE::average(int32_t value) {
	uint8_t round_v = emp_k >> 1;
	update(value);
	return (emp_data + round_v) / emp_k;
}

void EMP_AVERAGE::update(int32_t value) {
	uint8_t round_v = emp_k >> 1;
	emp_data += value - (emp_data + round_v) / emp_k;
}

int32_t EMP_AVERAGE::read(void) {
	uint8_t round_v = emp_k >> 1;
	return (emp_data + round_v) / emp_k;
}

int32_t	HIST::read(void) {
	int32_t sum = 0;
	if (len == 0) return 0;
	if (len == 1) return queue[0];
	for (uint8_t i = 0; i < len; ++i) sum += queue[i];
	sum += len >> 1;								// round the average
	sum /= len;
	return sum;
}

int32_t	HIST::average(int32_t value) {
	update(value);
	return read();
}

void HIST::update(int32_t value) {
	if (len < max_len) {
		queue[len++] = value;
	} else {
		queue[index] = value;
		if (++index >= max_len) index = 0;			// Use ring buffer
	}
}

uint32_t HIST::dispersion(void) {
	if (len < 3) return 1000;
	uint32_t sum = 0;
	uint32_t avg = read();
	for (uint8_t i = 0; i < len; ++i) {
		int32_t q = queue[i];
		q -= avg;
		q *= q;
		sum += q;
	}
	sum += len >> 1;
	sum /= len;
	return sum;
}

void SWITCH::init(uint8_t h_len, uint16_t off, uint16_t on) {
	EMP_AVERAGE::length(h_len);
    if (on < off) on = off;
    on_val    	= on;
    off_val   	= off;
    mode		= false;
}

bool SWITCH::status(void) {
	if (mode) {
		if (EMP_AVERAGE::read() < off_val)
			mode = false;
	} else {
		if (EMP_AVERAGE::read() > on_val)
			mode = true;
	}
    return mode;
}

void SWITCH::update(uint16_t value) {
	uint16_t max_val = on_val + (on_val >> 1);
	if (value > max_val) value = max_val;
	EMP_AVERAGE::update(value);
}
