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
#include "adc.h"
#include "quadspi.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "st7789.h"
#include "lcd.h"

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_QUADSPI_Init();
  MX_SPI2_Init();
  MX_TIM4_Init();
  MX_RTC_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */

  // Wait for wakeup to be released
  while( GPIO_PIN_RESET == HAL_GPIO_ReadPin( BUTTON_SW3_GPIO_Port, BUTTON_SW3_Pin ) );
  HAL_Delay( 50u );  // debounce

#define HAL_TIMER_PERIPH   (htim4)                  //!< Timer peripherial used by this driver
#define TIMER_CHANNEL      (TIM_CHANNEL_2)          //!< Timer channel used by this driver
#define PWM_DUTY           (htim4.Instance->CCR2)   //!< PWM duty cycle register
#define CLOCK_FREQ_HZ      (80000000)               //!< Clock frequency of the timer in Hz
  
  HAL_GPIO_WritePin( LCD_nPWR_GPIO_Port, LCD_nPWR_Pin, GPIO_PIN_RESET );
  HAL_GPIO_WritePin( LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET );
  
  ST7789_Init();
  LCD_init();
  ST7789_Fill_Color( BLACK );
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // Standby mode
    if( GPIO_PIN_RESET == HAL_GPIO_ReadPin( BUTTON_SW3_GPIO_Port, BUTTON_SW3_Pin ) )
    {
      // Wait for button to be released
      while( GPIO_PIN_RESET == HAL_GPIO_ReadPin( BUTTON_SW3_GPIO_Port, BUTTON_SW3_Pin ) );
      
      // Wait for debounce
      HAL_Delay( 200u );
      
      // Disable LCD & backlight
      HAL_GPIO_WritePin( LCD_nPWR_GPIO_Port, LCD_nPWR_Pin, GPIO_PIN_SET );
      HAL_GPIO_WritePin( LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_RESET );
      
      // Send QSPI flash to deep power-down
      QSPI_CommandTypeDef sCommand = { 0 };
      HAL_StatusTypeDef ret;
      /* Deep power down command --------------------------- */
      sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
      sCommand.Instruction = 0xB9u;
      sCommand.AddressMode = QSPI_ADDRESS_NONE;
      sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
      sCommand.DataMode = QSPI_DATA_NONE;
      sCommand.DummyCycles = 0;
      sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
      sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
      sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
      if ((ret = HAL_QSPI_Command(&hqspi, &sCommand,
                                  HAL_QPSI_TIMEOUT_DEFAULT_VALUE)) != HAL_OK)
      {
        //TODO: error handling
        HAL_Delay( 1000u );
      }
      
      // Enable wakeup on SW3 (== SYS_WAKEUP_PIN5, PC5)
      HAL_PWR_DisableWakeUpPin( PWR_CR3_EWUP );
      __HAL_PWR_CLEAR_FLAG( PWR_FLAG_WUF5 );
      HAL_PWREx_EnablePullUpPullDownConfig();
      HAL_PWREx_EnableGPIOPullUp( PWR_GPIO_C, PWR_GPIO_BIT_5 );
      HAL_PWR_EnableWakeUpPin( PWR_WAKEUP_PIN5_LOW );
      HAL_Delay( 50 );  // The pull-up resistor needs some time to activate
      
      // Disable SysTick timer
      HAL_SuspendTick();
      
      /* Set RTC back-up register RTC_BKP31R to indicate
         later on that system has entered shutdown mode  */
      WRITE_REG( RTC->BKP31R, 0x1 );
      
      // Enter deep sleep
      HAL_PWREx_EnableLowPowerRunMode();
      // Clear WUFx flags (Wakeup Flags) in PWR Status Register
      __HAL_PWR_CLEAR_FLAG( PWR_FLAG_WUF1 | PWR_FLAG_WUF2 | PWR_FLAG_WUF3 | PWR_FLAG_WUF4 | PWR_FLAG_WUF5 );
      // Enter Shutdown Mode --> point of no return
      HAL_PWREx_EnterSHUTDOWNMode();
      
      // If for some reason we get here...
      HAL_NVIC_SystemReset();
    }
    
    if( GPIO_PIN_RESET == HAL_GPIO_ReadPin( BUTTON_SW2_GPIO_Port, BUTTON_SW2_Pin ) )
    {
      ST7789_Test();
      ST7789_Fill_Color( BLACK );
    }
    
    if( GPIO_PIN_SET == HAL_GPIO_ReadPin( BUTTON_SW1_GPIO_Port, BUTTON_SW1_Pin ) )
    {
      uint32_t u32NewPeriod;
      for( uint32_t u32Frequency = 440u; u32Frequency < 4000u; u32Frequency += 10 )
      {
        u32NewPeriod = ( (CLOCK_FREQ_HZ/u32Frequency)/1000u ) - 1u;
        if( u32NewPeriod != HAL_TIMER_PERIPH.Init.Period )  // Set only if it's different from previous value
        {
          HAL_TIMER_PERIPH.Init.Prescaler = 1000u - 1u;
          HAL_TIMER_PERIPH.Init.Period = u32NewPeriod;
          HAL_TIM_PWM_Init( &HAL_TIMER_PERIPH );
        }
        // Set duty cycle
        PWM_DUTY = ( (uint32_t)(HAL_TIMER_PERIPH.Init.Period + 1u) * 127u )/255u;  
        HAL_TIM_PWM_Start( &HAL_TIMER_PERIPH, TIMER_CHANNEL );
        HAL_Delay(5);
      }
      PWM_DUTY = 0u;
    }
    
    static uint32_t u32Timer = 0u;
    if( HAL_GetTick() - u32Timer > 100u )
    {
      u32Timer = HAL_GetTick();
      
      // Measure battery/input voltage
      HAL_GPIO_WritePin( VMEAS_GND_GPIO_Port, VMEAS_GND_Pin, GPIO_PIN_RESET );
      HAL_Delay(1u);
      HAL_ADC_Start( &hadc1 );
      HAL_ADC_PollForConversion( &hadc1, 100 );
      uint16_t u16ADCmeasurement = HAL_ADC_GetValue( &hadc1 );
      HAL_ADC_Stop( &hadc1 );
      HAL_GPIO_WritePin( VMEAS_GND_GPIO_Port, VMEAS_GND_Pin, GPIO_PIN_SET );
      char acString[] = "Voltage: 0.00 V";
      snprintf( acString, sizeof( acString ), "Voltage: %1.2f V", (((float)u16ADCmeasurement)*(2.0f*3.0f/4095.0f)) );

      // Display charger status
      ST7789_WriteString( 10, 10, acString, Font_11x18, WHITE, BLACK );
      ST7789_WriteString( 10, 28, "nCHARGE: ", Font_11x18, WHITE, BLACK );
      if( GPIO_PIN_SET == HAL_GPIO_ReadPin( CHARGER_nCHARGE_GPIO_Port, CHARGER_nCHARGE_Pin ) )
      {
        ST7789_WriteString( 109, 28, "1", Font_11x18, WHITE, BLACK );
      }
      else
      {
        ST7789_WriteString( 109, 28, "0", Font_11x18, WHITE, BLACK );
      }
      ST7789_WriteString( 10, 46, "Standby: ", Font_11x18, WHITE, BLACK );
      if( GPIO_PIN_SET == HAL_GPIO_ReadPin( CHARGER_STANDBY_GPIO_Port, CHARGER_STANDBY_Pin ) )
      {
        ST7789_WriteString( 109, 46, "1", Font_11x18, WHITE, BLACK );
      }
      else
      {
        ST7789_WriteString( 109, 46, "0", Font_11x18, WHITE, BLACK );
      }
      
      // Display RTC
      RTC_DateTypeDef sdatestructureget;
      RTC_TimeTypeDef stimestructureget;
      /* Get the RTC current Time */
      HAL_RTC_GetTime( &hrtc, &stimestructureget, RTC_FORMAT_BIN );
      /* Get the RTC current Date */
      HAL_RTC_GetDate( &hrtc, &sdatestructureget, RTC_FORMAT_BIN );
      /* Display time Format : hh:mm:ss */
      snprintf( acString,sizeof( acString ),"%02d:%02d:%02d",stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds );
      ST7789_WriteString( 10, 64, acString, Font_16x26, WHITE, BLACK );
    }
    
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_4)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  while (LL_PWR_IsActiveFlag_VOS() != 0)
  {
  }
  LL_RCC_MSI_Enable();

   /* Wait till MSI is ready */
  while(LL_RCC_MSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_EnableRangeSelection();
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_6);
  LL_RCC_MSI_SetCalibTrimming(0);
  LL_PWR_EnableBkUpAccess();
  LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
  LL_RCC_LSE_Enable();

   /* Wait till LSE is ready */
  while(LL_RCC_LSE_IsReady() != 1)
  {

  }
  LL_RCC_MSI_EnablePLLMode();
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 40, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_EnableDomain_SYS();
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_SetSystemCoreClock(80000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  LL_RCC_PLLSAI1_ConfigDomain_48M(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 24, LL_RCC_PLLSAI1Q_DIV_2);
  LL_RCC_PLLSAI1_ConfigDomain_ADC(LL_RCC_PLLSOURCE_MSI, LL_RCC_PLLM_DIV_1, 24, LL_RCC_PLLSAI1R_DIV_2);
  LL_RCC_PLLSAI1_EnableDomain_48M();
  LL_RCC_PLLSAI1_EnableDomain_ADC();
  LL_RCC_PLLSAI1_Enable();

   /* Wait till PLLSAI1 is ready */
  while(LL_RCC_PLLSAI1_IsReady() != 1)
  {

  }
}

/* USER CODE BEGIN 4 */

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
