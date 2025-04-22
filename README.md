## How It Works (Step-by-Step)

This STM32 project reads temperature using a 10k NTC thermistor and the ADS1246 24-bit ADC. The thermistor is wired in a voltage divider with a 10k resistor. The STM32 reads the analog voltage using the ADS1246 via SPI, converts that voltage to thermistor resistance, then calculates the temperature using the Beta equation. Results are printed over UART to any serial monitor.

### 1. Power the System  
Connect your STM32 (e.g., Nucleo-F401RE) to your computer via USB or an external power source. The firmware will start automatically.

### 2. Sensor Initialization  
The ADC is prepared with these commands:
- `ADS1246_Reset()` — Toggles RESET pin (PB7) low → high to restart the ADS1246.  
- `ADS1246_Wakeup()` — Sends `0x00` to wake from power-down.  
- `ADS1246_DisableRDATAC()` — Sends `0x16` to stop continuous read so we can write config settings.

### 3. Configure the ADS1246  
These register writes configure the input and behavior:
- `ADS1246_WriteRegister(0x00, 0x08);` → Use AIN0 as input, AINCOM as GND.  
- `ADS1246_WriteRegister(0x01, 0x04);` → Enable internal bias voltage on AIN0.  
- `ADS1246_WriteRegister(0x02, 0x00);` → Set gain = 1 and lowest data rate for accurate, slow readings.

### 4. Enable Continuous Reading  
After configuration, `ADS1246_EnableRDATAC()` sends `0x14` to resume continuous data output.

### 5. Wait for DRDY Pin  
In `main.c`, the code checks for new ADC data like this:
```
if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
```
This means the DRDY (Data Ready) pin is LOW, which indicates the ADS1246 has completed a conversion and new data is available.

### 6. Read and Process the Data  
Once DRDY is LOW:
```
uint32_t raw = ADS1246_Read24bit();
```
Then the system does the following:
- `float avg_voltage = ADS1246_ConvertToVoltage(raw, 3.3);`  
- `float adjusted_voltage = avg_voltage * 1.34f;` → Scaling factor to improve accuracy  
- `float resistance = calculate_resistance(adjusted_voltage);`  
  → Uses: `R_therm = R_fixed * Vout / (Vin - Vout)`  
- `float temperature = calculate_temperature(resistance);`  
  → Applies Beta Equation: `1/T = 1/T0 + ln(R/R0)/β`

### 7. Print Results Over UART  
The following values are sent over UART and can be viewed using PuTTY, Arduino Serial Monitor, etc.:
- Raw ADC Value  
- Voltage (V)  
- Resistance (Ohms)  
- Temperature (°C)

## How to Customize

**Change Thermistor Specs**  
Modify the constants at the top of `main.c`:
```
#define SERIES_RESISTOR     10000.0   // Value of fixed resistor in voltage divider
#define NOMINAL_RESISTANCE  10000.0   // Thermistor resistance at 25°C
#define NOMINAL_TEMPERATURE 25.0      // Room temperature in °C
#define BETA_COEFFICIENT    3950.0    // Beta constant from thermistor datasheet
```

**Adjust Logging Speed**  
Add a delay inside the main `while(1)` loop to control how fast readings are taken:
```
HAL_Delay(1000); // 1000 ms = 1 reading per second
```

**Tweak Voltage Scaling**  
If your values seem off, you can adjust the scaling factor:
```
float adjusted_voltage = avg_voltage * 1.34f;
```
This accounts for voltage drop or mismatch in your divider circuit. You can fine-tune this.

**Serial Output**  
UART output uses 9600 baud. You can view output using any serial monitor:
- PuTTY  
- Arduino Serial Monitor  
- Tera Term  
- RealTerm

Now the STM32 is reading temperature data from a thermistor with high resolution via SPI and displaying results in real-time.
