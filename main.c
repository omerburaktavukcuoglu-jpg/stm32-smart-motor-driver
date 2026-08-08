/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

DAC_HandleTypeDef hdac;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// Sistem Durum Değişkenleri
uint8_t System_Initialized = 0; // 0: Kurulum Modu, 1: Sürüş Modu
uint8_t Emergency_Stop = 0;     // 1: Acil Durdurma Aktif

// Hız Değişkenleri
uint32_t Pot_Value = 0;
uint32_t Target_Speed = 0;
uint32_t Actual_Speed = 0;

// Mesafe Sensörü (Input Capture) Değişkenleri
uint32_t IC_Val1 = 0;
uint32_t IC_Val2 = 0;
uint32_t Difference = 0;
uint8_t Is_First_Captured = 0;  // 0: Yükselen kenar, 1: Düşen kenar
float Distance = 0.0;           // Ölçülen anlık mesafe (cm)

// UART Terminal Mesajları İçin Buffer
char uart_buf[100];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // htim3 olarak güncellendi
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2); // Mesafe Sensörü Kesmeli Giriş Yakalamayı Başlat
  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);         // Voltmetre Çıkışını (PA4) Başlat

  sprintf(uart_buf, "Lutfen potansiyometre ile bir baslangic hizi giriniz...\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), 100);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (Emergency_Stop == 1)
	    {
	      // Sistem kilitlendi, hiçbir şey yapma, sonsuz döngüde kal
	      HAL_Delay(100);
	      continue;
	    }

	    // 1. AŞAMA: AÇILIŞ VE KURULUM MODU
	    if (System_Initialized == 0)
	    {
	      HAL_ADC_Start(&hadc1); // Potansiyometre ADC'sini tetikle
	      if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
	      {
	        Pot_Value = HAL_ADC_GetValue(&hadc1);     // 0 - 4095 arası değer okur
	        Target_Speed = (Pot_Value * 1000) / 4095; // 0 - 1000 arası PWM sınırına eşitle
	      }

	      // Voltmetre ibresini oynat (DAC çıkışı 12-bit: 0 - 4095)
	      HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, Pot_Value);

	      // Terminale anlık pot bilgisini bas
	      sprintf(uart_buf, "Ayarlanan Hedef Hiz: %lu / 1000 (Onay icin Mavi Butona Basiniz)\r\n", Target_Speed);
	      HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), 100);

	      HAL_Delay(200); // Terminali şişirmemek için gecikme
	    }

	    // 2. AŞAMA: SÜRÜŞ VE GÜVENLİK MODU (Sistem onaylandıktan sonra)
	    else if (System_Initialized == 1)
	    {
	      // HC-SR04 Tetikleme (Sens_r_Trig pinine 10 mikrosaniye lojik 1 gönderilir)
	      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_SET);
	      // 10 mikrosaniyelik hassas donanımsal boş döngü gecikmesi (168 MHz için)
	      for(volatile int i = 0; i < 168; i++);
	      HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_RESET);

	      // Sürüş esnasında da anlık pot değişimini takip et (Dinamik hedef hız)
	      HAL_ADC_Start(&hadc1);
	      if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
	      {
	        Pot_Value = HAL_ADC_GetValue(&hadc1);
	        Target_Speed = (Pot_Value * 1000) / 4095;
	      }

	      // --- BÖLGE KONTROLLERİ ---

	      // A. GÜVENLİ BÖLGE (Mesafe > 20 cm)
	      if (Distance > 20.0)
	      {
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);   // Yeşil Aç
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET); // Diğerleri Kapat

	        Actual_Speed = Target_Speed; // Kısıtlama yok, tam hız

	        HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_RESET); // Sessiz
	      }

	      // B. UYARI BÖLGESİ (10 cm <= Mesafe <= 20 cm)
	      else if (Distance >= 10.0 && Distance <= 20.0)
	      {
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);   // Turuncu Aç
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_14, GPIO_PIN_RESET);

	        // Doğrusal Hız Düşürme Formülü
	        float factor = (Distance - 10.0) / 10.0;
	        Actual_Speed = (uint32_t)(Target_Speed * factor);

	        // Seyrek/Orantılı Bip Sesi
	        HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_SET);
	        HAL_Delay(20);
	        HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_RESET);
	        HAL_Delay((uint32_t)(Distance * 15)); // Mesafe azaldıkça gecikme kısalır, ses sıklaşır
	      }

	      // C. RİSKLİ BÖLGE (5 cm <= Mesafe < 10 cm)
	      else if (Distance >= 5.0 && Distance < 10.0)
	      {
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);   // Kırmızı Aç
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13, GPIO_PIN_RESET);

	        Actual_Speed = (uint32_t)(Target_Speed * 0.15); // %15 Güvenli Düşük Tork

	        // Sık Bip Alarmı
	        HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_SET);
	        HAL_Delay(30);
	        HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_RESET);
	        HAL_Delay(50);
	      }

	      // D. KRİTİK DURMA NOKTASI (Mesafe < 5 cm)
	      else if (Distance < 5.0)
	      {
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);   // Kırmızı Yanmaya Devam Eder
	        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13, GPIO_PIN_RESET);

	        Actual_Speed = 0; // TAM FREN

	        HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_SET); // KESİNTİSİZ DÜZ SES
	      }

	      // --- MOTOR VE VOLTMETRE GÜNCELLEME ---
	      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, Actual_Speed);// PE5 PWM Güncelle

	      // Voltmetreyi gerçek hıza (Actual_Speed) oranlayarak güncelle
	      uint32_t Dac_Output_Val = (Actual_Speed * 4095) / 1000;
	      HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, Dac_Output_Val);

	      // TELEMETRİ: Hercules Terminal Raporlaması
	      sprintf(uart_buf, "Mesafe: %.2f cm | Hedef Hiz: %lu | Gercek Hiz: %lu\r\n", Distance, Target_Speed, Actual_Speed);
	      HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), 50);

	      HAL_Delay(100);
	    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 167;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 167;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Sens_r_Trig_Pini_GPIO_OUTPUT_GPIO_Port, Sens_r_Trig_Pini_GPIO_OUTPUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GREEN_LED_GPIO_OUTPUT_Pin|YELLOW_LED_GPIO_OUTOUT_Pin|RED_LED_GPIO_OUTPUT_Pin|BLUE_LED_GPIO_OUTPUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Buzzer_GPIO_OUTPUT_Pin */
  GPIO_InitStruct.Pin = Buzzer_GPIO_OUTPUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Buzzer_GPIO_OUTPUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Acil_Butonu_EXTI0_Pin */
  GPIO_InitStruct.Pin = Acil_Butonu_EXTI0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(Acil_Butonu_EXTI0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Sens_r_Trig_Pini_GPIO_OUTPUT_Pin */
  GPIO_InitStruct.Pin = Sens_r_Trig_Pini_GPIO_OUTPUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Sens_r_Trig_Pini_GPIO_OUTPUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GREEN_LED_GPIO_OUTPUT_Pin YELLOW_LED_GPIO_OUTOUT_Pin RED_LED_GPIO_OUTPUT_Pin BLUE_LED_GPIO_OUTPUT_Pin */
  GPIO_InitStruct.Pin = GREEN_LED_GPIO_OUTPUT_Pin|YELLOW_LED_GPIO_OUTOUT_Pin|RED_LED_GPIO_OUTPUT_Pin|BLUE_LED_GPIO_OUTPUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// 1. MAVİ BUTON (EXTI) KESME FONKSİYONU
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == Acil_Butonu_EXTI0_Pin) // PA0 Kontrolü
  {
    if(System_Initialized == 0)
    {
      // İlk açılışta basıldıysa kurulumu bitir, sürüşü başlat
      System_Initialized = 1;
      sprintf(uart_buf, "\r\n--- SURUS MODU BASLADI ---\r\n");
      HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), 100);
    }
    else if(System_Initialized == 1 && Emergency_Stop == 0)
    {
      // Sürüş esnasında basıldıysa ACİL STOP moduna geç
      Emergency_Stop = 1;

      // Motoru anında durdur (PE5)
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);

      // Tüm LED'leri kapat, sadece Mavi LED (PD15) yansın
      HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);

      // Buzzer'ı sustur
      HAL_GPIO_WritePin(Buzzer_GPIO_OUTPUT_GPIO_Port, Buzzer_GPIO_OUTPUT_Pin, GPIO_PIN_RESET);

      sprintf(uart_buf, "\r\n!!! ACIL STOP AKTIF! SISTEM KILITLENDI !!!\r\n");
      HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), 100);
    }
  }
}

// 2. MESAFE SENSÖRÜ (TIM1 Input Capture) KESME FONKSİYONU
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) // TIM1_CH2 (PE11)
    {
      if (Is_First_Captured == 0) // Yükselen kenar yakalandıysa
      {
        IC_Val1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        Is_First_Captured = 1;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
      }
      else if (Is_First_Captured == 1) // Düşen kenar yakalandıysa
      {
        IC_Val2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        __HAL_TIM_SET_COUNTER(htim, 0);

        if (IC_Val2 >= IC_Val1)
        {
          Difference = IC_Val2 - IC_Val1;
        }
        else
        {
          Difference = (65535 - IC_Val1) + IC_Val2;
        }

        Distance = (float)Difference / 58.0; // Santimetreye çevrim

        Is_First_Captured = 0;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
      }
    }
  }
}
/* USER CODE END 4 */




/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
