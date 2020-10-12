// adc0.h
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef ADC0_H_
#define ADC0_H_

#include "adc0.h"

void initAdc0(void);
int16_t readAdc0Ss3(void);
uint16_t readRawResult(void);

#endif /* ADC0_H_ */
