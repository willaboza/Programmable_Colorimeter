// eeprom.h
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef EEPROM_H_
#define EEPROM_H_

//
// Includes and Defines
//
#include <stdbool.h>
#include "eeprom.h"

#define TOTAL_COLORS 16

typedef struct _STORED_COLORS {
    //--- color N and erase N Variables -----
    uint16_t index;
    int redValue[TOTAL_COLORS];
    int greenValue[TOTAL_COLORS];
    int blueValue[TOTAL_COLORS];
    bool validBit[TOTAL_COLORS];
} STORED_COLORS;

extern STORED_COLORS color;

//----- Calibrate Variables ------------
extern int threshold = 0;
extern bool validCalibration = false;
extern bool calibrateMode = false;

void initEeprom(void);
void writeEeprom(uint16_t add, uint32_t data);
uint32_t readEeprom(uint16_t add);
void readEepromAddress(void);

#endif /* EEPROM_H_ */
