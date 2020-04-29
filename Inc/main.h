/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define IRON_POWER_Pin GPIO_PIN_0
#define IRON_POWER_GPIO_Port GPIOA
#define FAN_POWER_Pin GPIO_PIN_1
#define FAN_POWER_GPIO_Port GPIOA
#define IRON_CURRENT_Pin GPIO_PIN_2
#define IRON_CURRENT_GPIO_Port GPIOA
#define FAN_CURRENT_Pin GPIO_PIN_3
#define FAN_CURRENT_GPIO_Port GPIOA
#define IRON_TEMP_Pin GPIO_PIN_4
#define IRON_TEMP_GPIO_Port GPIOA
#define GUN_TEMP_Pin GPIO_PIN_5
#define GUN_TEMP_GPIO_Port GPIOA
#define AMBIENT_Pin GPIO_PIN_6
#define AMBIENT_GPIO_Port GPIOA
#define ENCODER_L_Pin GPIO_PIN_0
#define ENCODER_L_GPIO_Port GPIOB
#define ENCODER_L_EXTI_IRQn EXTI0_IRQn
#define ENCODER_B_Pin GPIO_PIN_1
#define ENCODER_B_GPIO_Port GPIOB
#define OLED_RESET_Pin GPIO_PIN_10
#define OLED_RESET_GPIO_Port GPIOB
#define OLED_DC_Pin GPIO_PIN_11
#define OLED_DC_GPIO_Port GPIOB
#define OLED_SCK_Pin GPIO_PIN_13
#define OLED_SCK_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_15
#define OLED_SDA_GPIO_Port GPIOB
#define GUN_REED_Pin GPIO_PIN_8
#define GUN_REED_GPIO_Port GPIOA
#define ENCODER_R_Pin GPIO_PIN_9
#define ENCODER_R_GPIO_Port GPIOA
#define OLED_CS_Pin GPIO_PIN_10
#define OLED_CS_GPIO_Port GPIOA
#define GUN_POWER_Pin GPIO_PIN_11
#define GUN_POWER_GPIO_Port GPIOA
#define AC_ZERO_Pin GPIO_PIN_12
#define AC_ZERO_GPIO_Port GPIOA
#define AC_RELAY_Pin GPIO_PIN_15
#define AC_RELAY_GPIO_Port GPIOA
#define TILT_SW_Pin GPIO_PIN_3
#define TILT_SW_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_9
#define BUZZER_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define FW_VERSION	("3.00")

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
