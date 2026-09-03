/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Laboratorio 5 - TMR y UART
  ******************************************************************************
  * @attention
  *
  * Parte 1: Medición de frecuencia con TIM2 Input Capture.
  * Parte 2: Envío de la frecuencia medida por USART2.
  * Parte 3: Generación simultánea de dos señales cuadradas con TIM6 y TIM7.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * TIM2 cuenta a 1 MHz:
 *
 * TIM2_CLK = 48 MHz
 * PSC = 47
 *
 * 48 MHz / (47 + 1) = 1 MHz
 *
 * Por lo tanto:
 * 1 cuenta de TIM2 = 1 us
 */
#define FTIM_CNT 1000000UL

/*
 * PARTE 3
 *
 * TIM6 y TIM7 también reciben un reloj de 48 MHz.
 *
 * Se selecciona:
 * PSC = 47999
 *
 * f_CNT = 48 MHz / (47999 + 1)
 *       = 1000 Hz
 *
 * Por lo tanto:
 * 1 cuenta = 1 ms
 *
 * TIM6:
 * ARR = 249
 * Evento cada 250 ms.
 * Como el LED hace toggle cada 250 ms:
 * 250 ms ON + 250 ms OFF = periodo total de 500 ms.
 *
 * TIM7:
 * ARR = 999
 * Evento cada 1000 ms = 1 s.
 * Como el LED hace toggle cada 1 s:
 * 1 s ON + 1 s OFF = periodo total de 2 s.
 */
#define TIM_BASE_PRESCALER 47999U
#define TIM6_ARR           249U
#define TIM7_ARR           999U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* -------------------------------------------------------------------------- */
/* PARTE 1 - VARIABLES PARA INPUT CAPTURE                                     */
/* -------------------------------------------------------------------------- */

/* Indica que ya tenemos dos flancos y podemos calcular la frecuencia. */
volatile uint8_t flagMedicion = 0;

/*
 * 0 = esperamos primer flanco
 * 1 = esperamos segundo flanco
 */
volatile uint8_t numFlanco = 0;

/* TIM2 es de 32 bits, por eso las capturas son uint32_t. */
volatile uint32_t flanco1 = 0;
volatile uint32_t flanco2 = 0;

/* Diferencia entre las dos capturas. */
uint32_t delta = 0;

/*
 * Frecuencia multiplicada por 100.
 *
 * Ejemplo:
 * 100025 representa 1000.25 Hz.
 */
uint32_t frecuencia_x100 = 0;

/* -------------------------------------------------------------------------- */
/* PARTE 2 - VARIABLES PARA UART                                              */
/* -------------------------------------------------------------------------- */

/* Buffer para enviar la frecuencia por USART2. */
char uartBuffer[64];

/* -------------------------------------------------------------------------- */
/* PARTE 3 - BANDERAS DE LOS DOS TIMERS DE SALIDA                            */
/* -------------------------------------------------------------------------- */

/*
 * Las interrupciones de TIM6 y TIM7 únicamente levantan una bandera.
 * El cambio físico de los LEDs se realiza en el while(1), siguiendo
 * la misma estructura utilizada en los ejemplos del laboratorio.
 */
volatile uint8_t banderaLED1 = 0;
volatile uint8_t banderaLED2 = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
static void MX_USART2_UART_Init(void);

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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */

  /* ------------------------------------------------------------------------ */
  /* PARTE 2 - MENSAJE INICIAL DE UART                                        */
  /* ------------------------------------------------------------------------ */

  uint8_t mensajeInicio[] = "Laboratorio 5 listo\r\n";

  HAL_UART_Transmit(&huart2,
                    mensajeInicio,
                    sizeof(mensajeInicio) - 1,
                    1000);

  /* ------------------------------------------------------------------------ */
  /* PARTE 1 - INICIAR INPUT CAPTURE                                          */
  /* ------------------------------------------------------------------------ */

  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

  /* ------------------------------------------------------------------------ */
  /* PARTE 3 - INICIAR LOS DOS TIMERS DE LOS LEDS                            */
  /* ------------------------------------------------------------------------ */

  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Base_Start_IT(&htim7);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* ---------------------------------------------------------------------- */
    /* PARTE 3 - GENERACIÓN DE LAS DOS SEÑALES CUADRADAS                     */
    /* ---------------------------------------------------------------------- */

    if (banderaLED1 == 1)
    {
      banderaLED1 = 0;

      HAL_GPIO_TogglePin(LED1_GPIO_Port,
                         LED1_Pin);
    }

    if (banderaLED2 == 1)
    {
      banderaLED2 = 0;

      HAL_GPIO_TogglePin(LED2_GPIO_Port,
                         LED2_Pin);
    }

    /* ---------------------------------------------------------------------- */
    /* PARTE 1 - PROCESAMIENTO DE LA MEDICIÓN DE FRECUENCIA                  */
    /* ---------------------------------------------------------------------- */

    if (flagMedicion == 1)
    {
      flagMedicion = 0;

      if (flanco2 >= flanco1)
      {
        delta = flanco2 - flanco1;
      }
      else
      {
        delta = (0xFFFFFFFFUL - flanco1)
                + flanco2
                + 1UL;
      }

      if (delta > 0)
      {
        frecuencia_x100 =
            (FTIM_CNT * 100UL) / delta;

        /* ------------------------------------------------------------------ */
        /* PARTE 2 - CONVERTIR Y ENVIAR LA FRECUENCIA POR UART               */
        /* ------------------------------------------------------------------ */

        int longitud = snprintf(
            uartBuffer,
            sizeof(uartBuffer),
            "Frecuencia: %lu.%02lu Hz\r\n",
            (unsigned long)(frecuencia_x100 / 100UL),
            (unsigned long)(frecuencia_x100 % 100UL)
        );

        if (longitud > 0)
        {
          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)uartBuffer,
              (uint16_t)longitud,
              1000
          );
        }
      }

      HAL_TIM_IC_Start_IT(
          &htim2,
          TIM_CHANNEL_1
      );
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

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(
      PWR_REGULATOR_VOLTAGE_SCALE3
  );

  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSI;

  RCC_OscInitStruct.HSIState =
      RCC_HSI_ON;

  RCC_OscInitStruct.HSICalibrationValue =
      RCC_HSICALIBRATION_DEFAULT;

  RCC_OscInitStruct.PLL.PLLState =
      RCC_PLL_ON;

  RCC_OscInitStruct.PLL.PLLSource =
      RCC_PLLSOURCE_HSI;

  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 384;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)
      != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource =
      RCC_SYSCLKSOURCE_PLLCLK;

  RCC_ClkInitStruct.AHBCLKDivider =
      RCC_SYSCLK_DIV1;

  RCC_ClkInitStruct.APB1CLKDivider =
      RCC_HCLK_DIV4;

  RCC_ClkInitStruct.APB2CLKDivider =
      RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(
          &RCC_ClkInitStruct,
          FLASH_LATENCY_3)
      != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 47;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;

  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  *
  * PARTE 3:
  * Update Event cada 250 ms.
  * LED1 cambia cada 250 ms -> periodo total de 500 ms.
  */
static void MX_TIM6_Init(void)
{
  /* USER CODE BEGIN TIM6_Init 0 */

  __HAL_RCC_TIM6_CLK_ENABLE();

  /* USER CODE END TIM6_Init 0 */

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = TIM_BASE_PRESCALER;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = TIM6_ARR;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */
}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  *
  * PARTE 3:
  * Update Event cada 1 segundo.
  * LED2 cambia cada 1 s -> periodo total de 2 segundos.
  */
static void MX_TIM7_Init(void)
{
  /* USER CODE BEGIN TIM7_Init 0 */

  __HAL_RCC_TIM7_CLK_ENABLE();

  /* USER CODE END TIM7_Init 0 */

  htim7.Instance = TIM7;
  htim7.Init.Prescaler = TIM_BASE_PRESCALER;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = TIM7_ARR;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(TIM7_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM7_IRQn);

  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */
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

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(
      GPIOA,
      LD2_Pin | LED1_Pin,
      GPIO_PIN_RESET
  );

  HAL_GPIO_WritePin(
      LED2_GPIO_Port,
      LED2_Pin,
      GPIO_PIN_RESET
  );

  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(
      B1_GPIO_Port,
      &GPIO_InitStruct
  );

  GPIO_InitStruct.Pin = LD2_Pin | LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(
      GPIOA,
      &GPIO_InitStruct
  );

  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(
      LED2_GPIO_Port,
      &GPIO_InitStruct
  );

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* -------------------------------------------------------------------------- */
/* PARTE 1 - CALLBACK DE INPUT CAPTURE                                        */
/* -------------------------------------------------------------------------- */

void HAL_TIM_IC_CaptureCallback(
    TIM_HandleTypeDef *htim)
{
  if ((htim->Instance == TIM2) &&
      (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1))
  {
    if (numFlanco == 0)
    {
      flanco1 =
          HAL_TIM_ReadCapturedValue(
              htim,
              TIM_CHANNEL_1
          );

      numFlanco = 1;
    }
    else
    {
      flanco2 =
          HAL_TIM_ReadCapturedValue(
              htim,
              TIM_CHANNEL_1
          );

      HAL_TIM_IC_Stop_IT(
          htim,
          TIM_CHANNEL_1
      );

      numFlanco = 0;
      flagMedicion = 1;
    }
  }
}

/* -------------------------------------------------------------------------- */
/* PARTE 3 - CALLBACK DE LOS TIMERS BÁSICOS                                  */
/* -------------------------------------------------------------------------- */

void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    banderaLED1 = 1;
  }

  if (htim->Instance == TIM7)
  {
    banderaLED2 = 1;
  }
}

/* -------------------------------------------------------------------------- */
/* PARTE 3 - MANEJADORES DE INTERRUPCIÓN DE TIM6 Y TIM7                      */
/* -------------------------------------------------------------------------- */

void TIM6_DAC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim6);
}

void TIM7_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim7);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  __disable_irq();

  while (1)
  {
  }

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
  * @brief Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  */
void assert_failed(
    uint8_t *file,
    uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
