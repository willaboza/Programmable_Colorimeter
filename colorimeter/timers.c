// timers.c
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "gpio.h"
#include "led.h"
#include "eeprom.h"
#include "timers.h"

bool periodicMode = false;

// Function To Initialize Timers
void initTimer1(void)
{
    // Enable clocks
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    _delay_cycles(3);

    /*
    TIMER1_CTL_R   &= ~TIMER_CTL_TAEN;                        // Turn-off timer before reconfiguring
    TIMER1_CFG_R   = TIMER_CFG_32_BIT_TIMER;                  // Configure as 32-bit timer (A+B)
    TIMER1_TAMR_R  = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // Configure for periodic mode (count down)
    TIMER1_TAILR_R = 4000000;                                 // Set load value to 4,000,000
    TIMER1_IMR_R   = TIMER_IMR_TATOIM;                        // Turn-on interrupts
    TIMER1_TAV_R   = 0;
    NVIC_EN0_R     &= ~(1 << (INT_TIMER1A - INT_GPIOA));      // Turn-off vector number 37, or interrupt 21, (TIMER1A)
    TIMER1_CTL_R   |= TIMER_CTL_TAEN;                         // Turn-on timer
    */
}

// Enter info for  TIMER1 Interrupt Service Routine here
void timer1Isr(void)
{
    if(periodicMode)
    {
        // reset counter for next period
        TIMER1_TAV_R = 0;

        // get rgb triplet measurement and display
        getMeasurement();
    }

    // clear interrupt flag
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}

// Placeholder random number function
uint32_t random32(void)
{
    return TIMER1_TAV_R;
}

//function to set the period between measurements
void periodicT(uint16_t period)
{
    // If value of str is '0' or 'off' then deactivate periodic T
    if(period == 0)
    {
        disableIntTimer1();
        sendUart0String("  periodic T function is OFF\r\n");
    }
    // If value of str is between 1... 255 then store loadValue in TIMER1_TAILR_R,
    // set timeMode to false, periodicMode to true, and turn on Wide5Timer as a counter.
    else if(period >= 1 && period <= 255)
    {
        enableIntTimer1(period);
    }
}

// Turn-ON vector number 37 (Interrupt 21) for TIMER1A
void disableIntTimer1(void)
{
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;      // Turn-off timer before reconfiguring
    TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;
    NVIC_EN0_R   &= ~(1 << (INT_TIMER1A - INT_GPIOA));
    periodicMode = false;
}

// Turn-ON vector number 37 (Interrupt 21) for TIMER1A
void enableIntTimer1(uint16_t period)
{
    TIMER1_CTL_R   &= ~TIMER_CTL_TAEN;       // Turn-off timer before reconfiguring
    TIMER1_CFG_R   = TIMER_CFG_32_BIT_TIMER; // Configure as 32-bit timer (A+B)
    TIMER1_TAMR_R  = TIMER_TAMR_TAMR_PERIOD; // Configure for periodic mode (count down)
    TIMER1_TAILR_R = (period * 4000000);     // Multiply fc = 40MHz by resolution in units of 0.1s (4,000,000 = 40MHz*0.1)
    TIMER1_IMR_R   = TIMER_IMR_TATOIM;       // turn-on interrupts
    TIMER1_TAV_R   = 0;
    NVIC_EN0_R     |= (1 << (INT_TIMER1A - INT_GPIOA)); // Turn-on vector number 37, or interrupt 21, (TIMER1A)
    TIMER1_CTL_R   |= TIMER_CTL_TAEN;        // Turn-on timer
    periodicMode = true;
}
