// Analog UART Example (no filtering)
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL with LCD/Temperature Sensor
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// LM60 Temperature Sensor:
//   AN0/PE3 is driven by the sensor (Vout = 424mV + 6.25mV / degC with +/-2degC uncalibrated error)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "adc0.h"

void initAdc0(void)
{
    // Enable clocks
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;

    // Enable clocks
    enablePort(PORTE);
    _delay_cycles(3);

    // Configure AN0 as an analog input
    GPIO_PORTE_AFSEL_R |= 0x08;                      // select alternative functions for AN0 (PE3)
    GPIO_PORTE_DEN_R &= ~0x08;                       // turn off digital operation on pin PE3
    GPIO_PORTE_AMSEL_R |= 0x08;                      // turn on analog operation on pin PE3

    // Configure ADC
    ADC0_CC_R = ADC_CC_CS_SYSPLL;                    // select PLL as the time base (not needed, since default value)
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;                // disable sample sequencer 3 (SS3) for programming
    ADC0_EMUX_R = ADC_EMUX_EM3_PROCESSOR;            // select SS3 bit in ADCPSSI as trigger
    ADC0_SSMUX3_R = 0;                               // set first sample to AN0
    ADC0_SSCTL3_R = ADC_SSCTL3_END0;                 // mark first sample as the end
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;                 // enable SS3 for operation
}

int16_t readAdc0Ss3(void)
{
    ADC0_PSSI_R |= ADC_PSSI_SS3;                     // set start bit
    while (ADC0_ACTSS_R & ADC_ACTSS_BUSY);           // wait until SS3 is not busy
    return ADC0_SSFIFO3_R;                           // get single result from the FIFO
}

//Function used to return a Cumulative Average of raw results
uint16_t readRawResult(void)
{
    // Read Light Sensor PE3(AN0) Pin and return unsigned integer value
    return (uint16_t)readAdc0Ss3();
}
