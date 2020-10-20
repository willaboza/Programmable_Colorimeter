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
#include <stdint.h>
#include <stdbool.h>
#include "eeprom.h"

#define TOTAL_COLORS 16

typedef struct _STORED_COLORS {
    //--- color N and erase N Variables -----
    uint8_t index;
    bool validBit[TOTAL_COLORS];
    uint32_t redValue[TOTAL_COLORS];
    uint32_t greenValue[TOTAL_COLORS];
    uint32_t blueValue[TOTAL_COLORS];
} STORED_COLORS;

extern STORED_COLORS color;

//----- Calibrate Variables ------------
extern int threshold;
extern bool validCalibration;
extern bool calibrateMode;

void initEeprom(void);
void writeEeprom(uint16_t add, uint32_t data);
uint32_t readEeprom(uint16_t add);
void readEepromAddress(void);
void storeColors(void);
void loadColors(void);
void eraseColor(int index);
void printLearnedColors(void);

#endif /* EEPROM_H_ */
