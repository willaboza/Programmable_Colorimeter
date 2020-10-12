// led.h
// William Bozarth
// Created on: October 8, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef LED_H_
#define LED_H_

#include <stdint.h>
#include <stdbool.h>
#include "led.h"

//
// Includes and Defines
#define RAMP_SPEED 20000
#define DELTA_MAX  500000
#define GREEN_LED PORTF,3

extern bool sampleLed;

//----- Trigger Variables ---------------
extern uint16_t ledRed;
extern uint16_t ledGreen;
extern uint16_t ledBlue;

//------- Test Variables ---------------
extern uint16_t redLedCal[1024];
extern uint16_t greenLedCal[1024];
extern uint16_t blueLedCal[1024];
extern uint16_t rgbLeds[3];
extern bool validTest;
extern bool printTest;

//-------- match E Variables ------------
extern uint8_t matchValue;
extern float eColor;
extern bool matchMode;
extern bool match;
extern bool testMode;

typedef struct _DELTA_MODE {
    bool mode;
    float methodResult;
    float methodValues[16];
    float sum;
    int index;
    int difference;
    int value;
} DELTA_MODE;

extern DELTA_MODE delta;

uint16_t getRgbValue(char * buffer);
void testLED(void);
void calibrateLed(void);
void getMeasurement(void);
void setTriplet(void);
int normalizeRgbColor(int measurement);
void deltaD(uint8_t index);
void euclidNorm(void);
void rampLed(uint16_t ledCal[], uint16_t leds[]);

#endif /* LED_H_ */
