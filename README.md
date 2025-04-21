This STM32 project reads temperature using a 10k NTC thermistor and the ADS1246 24-bit ADC. The thermistor is connected in a voltage divider, and the STM32 reads the voltage via SPI, converts it to resistance, and then calculates the temperature using the Beta equation. Results are sent over UART.

1. Power the system – Connect your STM32 to your computer via USB or an external power source.

2. Initialize sensors – On boot, the ADS1246 is reset, configured, and prepared to start conversions.

3. Start reading – The system waits for the DRDY pin to go LOW, then reads raw 24-bit ADC data over SPI.

4. Convert the data – Raw data is converted to voltage, then to resistance, then to temperature using the Beta equation.

5. Output results – Temperature, resistance, voltage, and raw ADC values are printed over UART. You can view this in any serial monitor (e.g., PuTTY, Arduino Serial Monitor, etc.).
