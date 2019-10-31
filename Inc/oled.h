/*
 * oled.h
 *
 *  Created on: 31 èþë. 2019 ã.
 *      Author: Alex
 */

#ifndef OLED_H_
#define OLED_H_

#include "stm32f1xx_hal.h"
#include "myU8g2lib.h"
#include "font.h"
#include "main.h"

#define		SPI_HANDLER hspi2 								// use your SPI handler
#define 	I2C_HANDLER	hi2c1								// use your I2C handler
extern 		SPI_HandleTypeDef SPI_HANDLER;
extern		I2C_HandleTypeDef I2C_HANDLER;

#define		OLED_I2C_ADDR	(0x3C)

extern "C" uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
extern "C" uint8_t u8x8_byte_stm32_hw_spi(u8x8_t *u8g2, uint8_t msg, uint8_t arg_int, void *arg_ptr);
extern "C" uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#endif
