/* Includes */
#include "main.h"
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"

/* Private variables */
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define ADS1246_CMD_WAKEUP      0x00	// Wake-up command: Exits power-down mode
#define ADS1246_CMD_RDATAC      0x14    // Read Data Continuously command: Enables automatic streaming of data after each conversion
#define ADS1246_CMD_SDATAC      0x16	// Stop Read Data Continuously command: Disables RDATAC mode so commands can be sent
#define ADS1246_CMD_WREG        0x40    // Write Register command: Base command to write to configuration registers (0x40 | reg_address)

/* ADS1246 Function Declarations */
void ADS1246_Reset(void);								  // Sends a hardware reset signal to the ADS1246 via the RESET pin (active low)
void ADS1246_Wakeup(void);								  // Sends the WAKEUP command to bring the ADS1246 out of power-down mode
void ADS1246_WriteRegister(uint8_t reg, uint8_t value);	  // Writes a single byte to a specified register in the ADS1246
void ADS1246_EnableRDATAC(void);						  // Sends the RDATAC command to enable continuous data read mode
void ADS1246_DisableRDATAC(void);						  // Sends the SDATAC command to disable continuous data read mode
uint32_t ADS1246_Read24bit(void);						  // Reads the 24-bit conversion result from the ADS1246
float ADS1246_ConvertToVoltage(uint32_t raw, float vref); // Converts a raw 24-bit ADC result into a voltage based on the reference voltage used

/* Sends a hardware reset pulse to the ADS1246 by toggling the RESET pin low then high */
void ADS1246_Reset(void) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); // Pull RESET pin low to initiate reset (active low)
    HAL_Delay(1); 										  // Small wait to let ADS1246 apply changes
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);   // Pull RESET pin high to complete the reset
    HAL_Delay(1);										  // Small delay to let the chip get ready
}

/* Sends the WAKEUP command to the ADS1246 to bring it out of power-down mode */
void ADS1246_Wakeup(void) {
    uint8_t cmd = 0x00; 								  // WAKEUP command
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // Set Chip Select (CS) low to start an SPI command transaction
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);	  // Transmit the WAKEUP command over SPI (sends 0x00 to ADS1246)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);   // Set Chip Select (CS) high to end the SPI transaction
}

/* Writes a single byte to a specified register on the ADS1246 */
void ADS1246_WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t cmd[3] = { 0x40 | reg, 0x00, value }; 		  // Byte 1: WREG command + reg address, Byte 2: write 1 register (0x00), Byte 3: data to write
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // Pull CS low to start the SPI transaction
    HAL_SPI_Transmit(&hspi2, cmd, 3, HAL_MAX_DELAY);	  // Send the 3-byte WREG command sequence over SPI
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);   // Pull CS high to end the SPI transaction and latch the command
    HAL_Delay(1);										  // Short delay to give the ADC time to apply the new register settings
}

/* Sends the RDATAC command to the ADS1246 to enable Read Data Continuously mode */
void ADS1246_EnableRDATAC(void) {
    uint8_t cmd = 0x14; 								  // RDATAC command
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // Pull CS (Chip Select) LOW to begin the SPI command transaction
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);	  // Transmit the RDATAC command over SPI
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);   // Pull CS HIGH to end the SPI command transaction
}

/* Sends the SDATAC command to the ADS1246 to disable Read Data Continuously mode */
void ADS1246_DisableRDATAC(void) {
    uint8_t cmd = 0x16; 								  // SDATAC command
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // Pull CS (Chip Select) LOW to begin the SPI command transaction
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);	  // Transmit the SDATAC command over SPI
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);   // Pull CS HIGH to end the SPI command transaction
}

/* Reads a 24-bit ADC conversion result from the ADS1246 over SPI */
uint32_t ADS1246_Read24bit(void) {
    uint8_t buf[3];										  // Buffer to hold the 3 bytes (24 bits) of data from the ADC
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // Pull CS (Chip Select) LOW to start the SPI transaction
    HAL_SPI_Receive(&hspi2, buf, 3, HAL_MAX_DELAY);		  // Receive 3 bytes of data over SPI from the ADS1246's DOUT line
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);   // Pull CS HIGH to end the SPI transaction

    // Combine the 3 bytes into a single 24-bit signed integer (MSB to LSB)
    uint32_t result = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    return result;										  // Return the final signed 32-bit result
}

/* Converts a raw 24-bit ADC result into a voltage based on the reference voltage */
float ADS1246_ConvertToVoltage(uint32_t raw, float vref) {
    return ((float)raw / 16777215.0f) * vref; 	          // 16777215 (0xFFFFFF) is the max value from a 24-bit ADC
}
