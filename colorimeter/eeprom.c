// eeprom.h
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz


#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "eeprom.h"

STORED_COLORS color = {0};

int threshold = 0;
bool validCalibration = false;
bool calibrateMode = false;

// Function to initialize EEPROM
void initEeprom(void)
{
    SYSCTL_RCGCEEPROM_R = 1;
    _delay_cycles(6);
    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

// Function to write data to EEPROM
void writeEeprom(uint16_t add, uint32_t data)
{
    EEPROM_EEBLOCK_R = add >> 4; // Shift right 4 bits is same as dividing address by 16
    EEPROM_EEOFFSET_R = add & 0xF;
    EEPROM_EERDWR_R = data;
    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

// Function to read data from EEPROM
uint32_t readEeprom(uint16_t add)
{
    EEPROM_EEBLOCK_R = add >> 4;
    EEPROM_EEOFFSET_R = add & 0xF;
    return EEPROM_EERDWR_R;
}

// Function to Retrieve Current Address for Device
void readEepromAddress(void)
{
    uint32_t num = 0;
    if((num = readEeprom(0x0000)) != 0xFFFFFFFF)
    {
        //SOURCE_ADDRESS = num; // Retrieve Stored Address From EEPROM
    }
    else
    {
        //SOURCE_ADDRESS = 1; // NO Previous Address Stored in EEPROM, default = 1
    }
}
