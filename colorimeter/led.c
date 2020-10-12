// led.h
// William Bozarth
// Created on: October 8, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "led.h"
#include "uart0.h"
#include "gpio.h"
#include "pwm0.h"
#include "adc0.h"
#include "wait.h"
#include "eeprom.h"

DELTA_MODE delta = {0};

//Return LED Value for Passing to setRgbColor()
uint16_t getRgbValue(char * buffer)
{
    uint16_t led;

    //fetch value from 0 ---> 1023 and store in buffer. Convert string to integer and return
    //the integer back to parent function.

    //getsUart0(buffer, QUEUE_BUFFER_LENGTH);
    led = atoi(buffer);

    return led;
}

//function to test led functionality
void testLED(void)
{
    setRgbColor(0,0,0);

    sendUart0String("wait.... ");
    sendUart0String("\r\n");

    // Ramp Red LED
    rampLed(redLedCal, rgbLeds);

    // Turn off all LEDs
    setRgbColor(0,0,0);

    // Ramp Green LED
    rampLed(greenLedCal, rgbLeds);

    // Turn off all LEDs
    setRgbColor(0,0,0);

    // Ramp Blue LED
    rampLed(blueLedCal, rgbLeds);

    // Turn off all LEDs
    setRgbColor(0,0,0);

    sendUart0String("\r\n");
    validTest = true;
    testMode = false;
}

// Function to ramp led from 0 -> 1023
void rampLed(uint16_t ledCal[], uint16_t leds[])
{
    int i, num;
    char buffer[QUEUE_BUFFER_LENGTH];

    //Ramp LED
    for (i = 0; i < 1024; i++)
    {
         setRgbColor(i, 0, 0);
         waitMicrosecond(RAMP_SPEED);
         num = readRawResult();

         if(testMode == true)
         {
             ledCal[i] = num;
         }
         else if(calibrateMode == true && num <= threshold)
         {
             leds[0] = i;
         }
         else if(calibrateMode == true && num > threshold)
         {
             break;
         }

         if(printTest == true)
         {
             //no spaces between the 4 numbers as these are copy and pasted
             //into excel for plotting purposes.
             snprintf(buffer, sizeof(buffer), "%d,0,0,%u\r\n", i, num);
             sendUart0String(buffer);
             sendUart0String("\r\n");
         }
    }
}

void calibrateLed(void)
{
    bool ok = true;
    char buffer[QUEUE_BUFFER_LENGTH];

    if(threshold > 4095)
    {
        sendUart0String("Threshold Value Outside of Range (0 to 4095).\r\n");
        ok = false;
    }
    else
    {
        testLED();
    }
    //Set threshold values for R,G, and B if no errors detected.
    if(ok == true )
    {
        validCalibration = true;

        snprintf(buffer, sizeof(buffer), "(PWMr: %u,PWMg: %u,PWMb: %u)\r\n", rgbLeds[0], rgbLeds[1], rgbLeds[2]);
        sendUart0String(buffer);
        sendUart0String("\r\n");
    }
    calibrateMode = false;
}

//Function to call when needing to measure for trigger and button commands
void getMeasurement(void)
{
    char buffer[QUEUE_BUFFER_LENGTH];
    uint16_t temp = 0;

    if(validCalibration == true)
    {
        //store Red LED value
        setRgbColor(0,0,0);
        setRgbColor(rgbLeds[0], 0, 0);
        waitMicrosecond(20000);
        temp = readRawResult();
        ledRed = normalizeRgbColor(temp);

        //store Green LED value
        setRgbColor(0,0,0);
        setRgbColor(0, rgbLeds[1], 0);
        waitMicrosecond(20000);
        temp = readRawResult();
        ledGreen = normalizeRgbColor(temp);

        //store Blue LED value
        setRgbColor(0,0,0);
        setRgbColor(0, 0, rgbLeds[2]);
        waitMicrosecond(20000);
        temp = readRawResult();
        ledBlue = normalizeRgbColor(temp);

        //turn of r,g,b LEDS after measurements
        setRgbColor(0, 0, 0);

        if(delta.mode == true)
        {
            // Need to modify index values
            deltaD(1);
        }

        //if match mode is turned on calculate the Euclidean Distance between measured value
        //and stored colors. If eColor < matchValue then display matched color on terminal
        if(matchMode == true)
        {
            for(color.index = 0; color.index < TOTAL_COLORS; color.index++)
            {
                if(color.validBit[color.index])
                {
                    euclidNorm();

                    if(eColor < matchValue)
                    {
                        snprintf(buffer, sizeof(buffer), "color %u\r\n", color.index);
                        sendUart0String(buffer);
                        sendUart0String("\r\n");
                    }
                }
            }
        }

        if(sampleLed)
        {
            setPinValue(GREEN_LED, 1);
        }
        waitMicrosecond(10000);

        //If in delta mode and change in r,g,b is greater than deltaValue then print new values
        if(delta.mode == true && delta.difference > delta.value)
        {
            //print raw results in comparison
            snprintf(buffer, sizeof(buffer), "(r: %u,g: %u,b: %u).\r\n", ledRed, ledGreen, ledBlue);
            sendUart0String(buffer);
            sendUart0String("\r\n");
        }
        //if not in delta mode print measured values
        else if(!delta.mode)
        {
            //print raw results in comparison
            snprintf(buffer, sizeof(buffer), "(r: %u,g: %u,b: %u).\r\n", ledRed, ledGreen, ledBlue);
            sendUart0String(buffer);
            sendUart0String("\r\n");
        }
        if(sampleLed)
        {
            setPinValue(GREEN_LED, 1);
        }
    }
    else
    {
        sendUart0String("No valid calibration performed. Please perform calibration.\r\n");
    }
}

//return measured value in the range of 0... 255 using
// m = (m/th)*(tMax-tMin), where th = threshold, tMax
//and tMin is target max and min, and m = measured value
int normalizeRgbColor(int measurement)
{
    int temp;
    float numerator, denominator, ratio;
    char buffer[QUEUE_BUFFER_LENGTH];

    snprintf(buffer, sizeof(buffer), "%u.0\0",measurement);
    numerator = atof(buffer);
    snprintf(buffer, sizeof(buffer), "%u.0\0",threshold);
    denominator = atof(buffer);
    ratio = (numerator/denominator) * 255.0;
    temp = ratio;

    if(temp > 255)
    {
        temp = 255;
    }

    return temp;
}

//function to set the delta value for displaying measured values when greater than D
void deltaD(uint8_t index)
{
    float avg = 0.0;

    delta.methodResult = sqrt((ledRed * ledRed) + (ledGreen * ledGreen) + (ledBlue * ledBlue));
    delta.sum -= delta.methodValues[index];
    delta.sum += delta.methodResult;
    delta.methodValues[index] = delta.methodResult;
    index = (index + 1) % 16;

    avg = 0.9*(delta.sum/16) + (0.1 * delta.methodResult);
    delta.difference = abs((int)delta.methodResult - (int)avg);
}

//function used to calculate the Euclidean distance
void euclidNorm(void)
{
    uint16_t colorDiffRed, colorDiffBlue, colorDiffGreen;
    float red, green, blue;
    char buffer[QUEUE_BUFFER_LENGTH];

    //calculate (r - ri)^2
    colorDiffRed= ledRed - color.redValue[color.index];
    colorDiffRed = colorDiffRed * colorDiffRed;
    snprintf(buffer, sizeof(buffer), "%u.0\0",colorDiffRed);
    red = atof(buffer);

    //calculate (g - gi)^2
    colorDiffGreen = ledGreen - color.greenValue[color.index];
    colorDiffGreen = colorDiffGreen * colorDiffGreen;
    snprintf(buffer, sizeof(buffer), "%u.0\0",colorDiffGreen);
    green = atof(buffer);

    //calculate (b - bi)^2
    colorDiffBlue = ledBlue - color.blueValue[color.index];
    colorDiffBlue = colorDiffBlue * colorDiffBlue;
    snprintf(buffer, sizeof(buffer), "%u.0\0",colorDiffBlue);
    blue = atof(buffer);

    //calculate square root of above differences
    eColor= sqrt (red + green + blue);
}
