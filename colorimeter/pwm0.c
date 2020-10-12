// pwm0.c
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "uart0.h"
#include "pwm0.h"

// PWM0_LEDS  = {0};

void initPwm0(void)
{
    // Enable clocks
    SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;
    _delay_cycles(3);

    enablePort(PORTB);
    _delay_cycles(3);

    enablePort(PORTE);
    _delay_cycles(3);

    // Configure PWM0 pins
    selectPinOpenDrainOutput(PWM0_RED_LED);
    selectPinOpenDrainOutput(PWM0_BLUE_LED);
    selectPinOpenDrainOutput(PWM0_GREEN_LED);

    setPinAuxFunction(PWM0_RED_LED, GPIO_PCTL_PB5_M0PWM3);
    setPinAuxFunction(PWM0_BLUE_LED, GPIO_PCTL_PE4_M0PWM4);
    setPinAuxFunction(PWM0_GREEN_LED, GPIO_PCTL_PE5_M0PWM5);

    // output 3 on PWM0, gen 1b, cmpb
    PWM0_1_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
    // output 4 on PWM0, gen 2a, cmpa
    PWM0_2_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE;
    // output 5 on PWM0, gen 2b, cmpb
    PWM0_2_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;

    // set period to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
    PWM0_1_LOAD_R = 1024;
    PWM0_2_LOAD_R = 1024;

    // invert outputs for duty cycle increases with increasing compare values
    PWM0_1_CMPB_R = 0;                               // red off (0=always low, 1023=always high)
    PWM0_2_CMPB_R = 0;                               // green off
    PWM0_2_CMPA_R = 0;                               // blue off

    PWM0_1_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1
    PWM0_2_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 2
    PWM0_ENABLE_R = PWM_ENABLE_PWM3EN | PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN;// enable outputs
}

//Set Red,Green,and Blue LED Colors
void setRgbColor(uint16_t red, uint16_t green, uint16_t blue)
{
    PWM0_1_CMPB_R = red;      //set value recorded for red
    PWM0_2_CMPA_R = blue;     //set value recorded for blue
    PWM0_2_CMPB_R = green;    //set value recorded for green
}

// Change value of red LED
void setRedLed(uint16_t red)
{
    PWM0_1_CMPB_R = red;      //set value recorded for red
}

// Change value of green LED
void setGreenLed(uint16_t green)
{
    PWM0_2_CMPB_R = green;    //set value recorded for green
}

// Change value of blue LED
void setBlueLed(uint16_t blue)
{
    PWM0_2_CMPA_R = blue;     //set value recorded for blue
}
