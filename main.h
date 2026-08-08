/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f4xx_hal.h"

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
#define Buzzer_GPIO_OUTPUT_Pin GPIO_PIN_1
#define Buzzer_GPIO_OUTPUT_GPIO_Port GPIOC
#define Acil_Butonu_EXTI0_Pin GPIO_PIN_0
#define Acil_Butonu_EXTI0_GPIO_Port GPIOA
#define Acil_Butonu_EXTI0_EXTI_IRQn EXTI0_IRQn
#define Potansiyometre_Analog_giri__ADC1_Pin GPIO_PIN_1
#define Potansiyometre_Analog_giri__ADC1_GPIO_Port GPIOA
#define Voltmetre_Analog___k___DAC_Pin GPIO_PIN_4
#define Voltmetre_Analog___k___DAC_GPIO_Port GPIOA
#define TIM3_CH1_motor_pwm_Pin GPIO_PIN_6
#define TIM3_CH1_motor_pwm_GPIO_Port GPIOA
#define Sens_r_Trig_Pini_GPIO_OUTPUT_Pin GPIO_PIN_7
#define Sens_r_Trig_Pini_GPIO_OUTPUT_GPIO_Port GPIOE
#define Echo_giri__yakalama_TIM__Pin GPIO_PIN_11
#define Echo_giri__yakalama_TIM__GPIO_Port GPIOE
#define GREEN_LED_GPIO_OUTPUT_Pin GPIO_PIN_12
#define GREEN_LED_GPIO_OUTPUT_GPIO_Port GPIOD
#define YELLOW_LED_GPIO_OUTOUT_Pin GPIO_PIN_13
#define YELLOW_LED_GPIO_OUTOUT_GPIO_Port GPIOD
#define RED_LED_GPIO_OUTPUT_Pin GPIO_PIN_14
#define RED_LED_GPIO_OUTPUT_GPIO_Port GPIOD
#define BLUE_LED_GPIO_OUTPUT_Pin GPIO_PIN_15
#define BLUE_LED_GPIO_OUTPUT_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
