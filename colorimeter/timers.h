// timers.h
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef TIMERS_H_
#define TIMERS_H_

//
// Includes and Defines
//
#include "timers.h"

//
// Global Variables
//
extern bool periodicMode;
extern uint16_t periodicValue;
extern int loadValue;

//
// Definitions
//
void initTimer1(uint16_t period);
void timer1Isr(void);
uint32_t random32(void);
void periodicT(uint16_t loadValue);

#endif /* TIMERS_H_ */
