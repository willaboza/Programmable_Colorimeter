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
static bool periodicMode = false;
static uint16_t periodicValue = 0;
int loadValue = 0;

//
// Definitions
//
void initTimer(uint16_t period);
void timerIsr(void);
uint32_t random32(void);
void periodicT(uint16_t loadValue);

#endif /* TIMERS_H_ */
