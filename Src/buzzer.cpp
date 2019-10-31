/*
 * buzzer.cpp
 *
 *  Created on: 14 рту. 2019 у.
 *      Author: Alex
 */

#include "buzzer.h"
#include "main.h"

BUZZER::BUZZER(void) {
	TIM4->CCR4 	= 0;
}

void BUZZER::playTone(uint16_t period_mks, uint16_t duration_ms) {
	TIM4->ARR 	= period_mks-1;
	TIM4->CCR4 	= period_mks >> 1;
	HAL_Delay(duration_ms);
	TIM4->CCR4 	= 0;
}

void BUZZER::shortBeep(void) {
	//if (!enabled) return;
	playTone(284, 160);
}

void BUZZER::doubleBeep(void) {
	//if (!enabled) return;
	playTone(284, 160);
	HAL_Delay(100);
	playTone(284, 160);
}

void BUZZER::lowBeep(void) {
	//if (!enabled) return;
	playTone(2840, 300);
}

void BUZZER::failedBeep(void) {
	//if (!enabled) return;
	playTone(284, 160);
	HAL_Delay(50);
	playTone(2840, 60);
	HAL_Delay(50);
    playTone(1420, 160);
}

