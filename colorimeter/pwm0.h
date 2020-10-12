// pwm0.h
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef PWM0_H_
#define PWM0_H_

#include "pwm0.h"

#define MAX_PWM 1023

#define PWM0_RED_LED PORTB,5
#define PWM0_BLUE_LED PORTE,4
#define PWM0_GREEN_LED PORTE,5

//
// Structure Definition
//
typedef struct _PWM0_LEDS
{
    uint8_t redLed;
    uint8_t greenLed;
    uint8_t blueLed;
} PWM0_LEDS;

extern PWM0_LEDS pwm0;

void initPwm0(void);
void setRgbColor(uint16_t red, uint16_t green, uint16_t blue);
void setRedLed(uint16_t red);
void setGreenLed(uint16_t green);
void setBlueLed(uint16_t blue);

#endif /* PWM0_H_ */
