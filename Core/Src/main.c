/* Includes */
#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "ads1246.h"

/* Define constants for thermistor calculations */
#define SERIES_RESISTOR     10000.0   // 10k series resistor
#define NOMINAL_RESISTANCE 	10000.0	  // Thermistor resistance (in ohms) at 25 degrees Celsius (room temperature)
#define NOMINAL_TEMPERATURE 25.0      // Reference temperature in Celsius at which the thermistor has nominal resistance
#define BETA_COEFFICIENT    3950.0    // Beta constant (typical for 10k NTC thermistors)

/* Private variables */
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef huart2;

/* Private Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);

/* Function Prototypes */
float calculate_resistance(float avg_voltage);   // Converts the averaged voltage from the ADC into thermistor resistance (in Ohms)
float calculate_temperature(float resistance);   // Converts thermistor resistance into temperature using the Beta parameter equation
void send_uart_message(const char *msg);         // Sends a string message over UART


int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI2_Init();

  ADS1246_Reset();					 // Perform hardware reset on the ADS1246 (toggle RESET pin)
  ADS1246_Wakeup();					 // Send WAKEUP command to bring ADS1246 out of power-down mode
  ADS1246_DisableRDATAC(); 			 // Disable continuous read mode to allow register configuration

  ADS1246_WriteRegister(0x00, 0x08); // Set input MUX: AIN0 (thermistor) as positive input, AINCOM (GND) as negative input
  ADS1246_WriteRegister(0x01, 0x04); // Enable internal bias voltage on AIN0 to allow voltage reading
  ADS1246_WriteRegister(0x02, 0x00); // Set gain to 1x (no amplification) and slowest data rate for stable, accurate thermistor readings

  ADS1246_EnableRDATAC(); 			 // Re-enable continuous data mode

  while (1)
  {
	  // If new ADC data is ready, process and print voltage, resistance, and temperature
      if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) // Check if DRDY is LOW (new data ready)
      {
    	  uint32_t raw = ADS1246_Read24bit();  					 // Read raw 24-bit ADC value
          char buffer[50];

          // Debug print: Raw ADC value
          snprintf(buffer, sizeof(buffer), "Raw: %ld\r\n", raw);
          send_uart_message(buffer);

          // Convert raw to voltage
          float avg_voltage = ADS1246_ConvertToVoltage(raw, 3.3);
          snprintf(buffer, sizeof(buffer), "Voltage: %.2f V\r\n", avg_voltage);
          send_uart_message(buffer);

          // Calculate resistance
          float adjusted_voltage = avg_voltage * 1.34f;		    // Slightly boost voltage to make temperature reading more accurate
          float resistance = calculate_resistance(adjusted_voltage);
          snprintf(buffer, sizeof(buffer), "Resistance: %.2f Ohms\r\n", resistance);
          send_uart_message(buffer);

          // Calculate temperature
          float temperature = calculate_temperature(resistance);
          snprintf(buffer, sizeof(buffer), "Temp: %.2f *C\r\n", temperature);
          send_uart_message(buffer);
      }
   }
}

/* Function to calculate thermistor resistance from measured voltage */
float calculate_resistance(float Vout)
{
	// Apply voltage divider formula to solve for thermistor resistance
    return SERIES_RESISTOR * Vout / (3.3f - Vout);	// R_therm = R_fixed * Vout / (V_in - Vout), where V_in = 3.3V
}

/* Function to calculate temperature in Celsius using the Beta equation */
float calculate_temperature(float resistance)
{
	// Beta equation: 1/T = 1/T0 + (1/BETA) * ln(R/R0)
    float T0 = NOMINAL_TEMPERATURE + 273.15f;       			   // Convert nominal temperature (25 degrees Celsius) to Kelvin
    float lnRatio = logf(resistance / NOMINAL_RESISTANCE);		   // Calculate the natural log of the ratio between measured resistance and nominal resistance
    float inverse_T = (1.0f / T0) + (lnRatio / BETA_COEFFICIENT);  // Calculate the inverse of temperature in Kelvin using the Beta formula
    float tempK = 1.0f / inverse_T;								   // Invert to get temperature in Kelvin
    return tempK - 273.15f;                         			   // Convert Kelvin back to Celsius and return
}

/* Function to send messages via UART */
void send_uart_message(const char *msg)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY); // Transmit message via UART
}

/* ------------------------------------------------------------------------------- */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_SPI2_Init(void)
{
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
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
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, reset_Pin|START_Pin|chip_select_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DRDY_Pin */
  GPIO_InitStruct.Pin = DRDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DRDY_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : reset_Pin START_Pin chip_select_Pin */
  GPIO_InitStruct.Pin = reset_Pin|START_Pin|chip_select_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
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
