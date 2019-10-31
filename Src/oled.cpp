/*
 * oled.c
 *
 *  Created on: 31 èþë. 2019 ã.
 *      Author: Alex
 */
#include "oled.h"

extern "C" uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch (msg) {
		case U8X8_MSG_DELAY_NANO:							// delay arg_int * 1 nano second
			for (uint16_t i =0; i < arg_int; ++i);
			break;
		case U8X8_MSG_GPIO_AND_DELAY_INIT:
			HAL_Delay(1);
			break;
		case U8X8_MSG_DELAY_MILLI:
			HAL_Delay(arg_int);
			break;
		case U8X8_MSG_GPIO_DC:
			HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, (GPIO_PinState)arg_int);
			break;
		case U8X8_MSG_GPIO_RESET:
			HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, (GPIO_PinState)arg_int);
			break;
		default:
			break;
	}
	return 1;
}

extern "C" uint8_t u8x8_byte_stm32_hw_spi(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	switch (msg) {
		case U8X8_MSG_BYTE_SEND:
			while (HAL_SPI_STATE_READY != HAL_SPI_GetState(&SPI_HANDLER)) { }
			HAL_SPI_Transmit(&SPI_HANDLER, (uint8_t *)arg_ptr, arg_int, 100);
			break;
		case U8X8_MSG_BYTE_INIT:
			break;
		case U8X8_MSG_BYTE_SET_DC:
			HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, arg_int?GPIO_PIN_SET:GPIO_PIN_RESET);
			break;
		case U8X8_MSG_BYTE_START_TRANSFER:
			HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, (GPIO_PinState)(u8g2->display_info->chip_enable_level));
			u8g2->gpio_and_delay_cb(u8g2, U8X8_MSG_DELAY_NANO, u8g2->display_info->post_chip_enable_wait_ns, NULL);
			break;
		case U8X8_MSG_BYTE_END_TRANSFER:
			u8g2->gpio_and_delay_cb(u8g2, U8X8_MSG_DELAY_NANO, u8g2->display_info->pre_chip_disable_wait_ns, NULL);
			HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, (GPIO_PinState)(u8g2->display_info->chip_disable_level));
			break;
		default:
			return 0;
	}
	return 1;
}

extern "C" uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	static uint8_t dc = 0;

	switch(msg)  {
		case U8X8_MSG_BYTE_SEND:
			while (HAL_I2C_STATE_READY != HAL_I2C_GetState(&I2C_HANDLER)) { }
			HAL_I2C_Mem_Write(&I2C_HANDLER, OLED_I2C_ADDR<<1, (dc == 0)?0:0x40, 1, (uint8_t *)arg_ptr, arg_int, HAL_MAX_DELAY);
			break;
		case U8X8_MSG_BYTE_INIT:
			u8g2->gpio_and_delay_cb(u8g2, U8X8_MSG_DELAY_NANO, u8g2->display_info->post_chip_enable_wait_ns, NULL);
			break;
		case U8X8_MSG_BYTE_SET_DC:
			dc = arg_int;
			break;
		case U8X8_MSG_BYTE_START_TRANSFER:
			break;
		case U8X8_MSG_BYTE_END_TRANSFER:
			break;
		default:
			return 0;
	}
	return 1;
}

