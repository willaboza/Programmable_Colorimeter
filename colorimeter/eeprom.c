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
#include "uart0.h"

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

// Store Color in EEPROM
void storeColors(void)
{
    uint16_t i;
    for(i = 0; i < 16; i++)
    {
        if(color.validBit[i])
        {
            writeEeprom((16 * i), (uint32_t)color.validBit[i]);
            writeEeprom((16 * i) + 1, color.redValue[i]);
            writeEeprom((16 * i) + 2, color.greenValue[i]);
            writeEeprom((16 * i) + 3, color.blueValue[i]);
        }
    }
}

// Load colors stored in EEPROM
void loadColors(void)
{
    uint32_t num;

    for(int i = 0; i < 16; i++)
    {
        if((num = readEeprom(16*i)) != 0xFFFFFFFF)
        {
            color.validBit[i]   = (bool)num;
            color.redValue[i]   = readEeprom(16*i + 1);
            color.greenValue[i] = readEeprom(16*i + 2);
            color.blueValue[i]  = readEeprom(16*i + 3);
        }
    }
}

// Erase color specified by user
void eraseColor(int index)
{
    int i;

    writeEeprom((16 * index), 0xFFFFFFFF);

    for(i = 1; i < 4; i++)
    {
        writeEeprom((16 * index) + i, 0xFFFFFFFF);
    }
}


// Displays all colors learned by device for user
void printLearnedColors(void)
{
    int i;
    char buffer[25];

    for(i = 0; i < 16; i++)
    {
        if(color.validBit[i])
        {
            snprintf(buffer, sizeof(buffer), "  color %u: %u,%u,%u\r\n", i, color.redValue[i], color.greenValue[i], color.blueValue[i]);
            sendUart0String(buffer);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "  color %u: none\r\n", i);
            sendUart0String(buffer);
        }
    }
}
