// CSE3442 Project
// William Bozarth and Sigmund Koenigseder

//**********************************************************************************************
//                                       Hardware Target
//**********************************************************************************************
//
// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz
//
// Hardware configuration:
//    Green LED:
//        PF3 drives an NPN transistor that powers the green LED
//
//    UART Interface:
//        U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//        The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//        Configured to 115,200 baud, 8N1
//
//    Frequency counter and timer input:
//        FREQ_IN (WT5CCP0) on PD6
//
//    LSensor:
//        AIN0/PE3 is driven by the sensor

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "uart0.h"
#include "shell.h"
#include "reboot.h"
#include "wait.h"
#include "pwm0.h"
#include "timers.h"
#include "adc0.h"
#include "led.h"
#include "eeprom.h"

//
// Includes, Defines, and
//
#define PUSH_BUTTON PORTF,4

// Function to Initialize System Clock
void initHw(void)
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S) | SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_2;

    // Enable clocks
    enablePort(PORTF);
    _delay_cycles(3);

    // Configure LED and pushbutton pins
    selectPinPushPullOutput(GREEN_LED);
    selectPinDigitalInput(PUSH_BUTTON);
}

// Blocking function that returns only when SW1 is pressed
void waitPbPress(void)
{
    //wait until push button pressed
    while(getPinValue(PUSH_BUTTON));

    //wait specified time
    waitMicrosecond(10000);
}

//
// Start of Main Function
//
void main(void){

    // Initialize hardware
    initHw();
    initEeprom();
    initUart0();
    initAdc0();
    initPwm0();
    initAdc0();
    initTimer1();
    // initWatchdog();

    // Setup UART0 Baud Rate
    setUart0BaudRate(115200, 40e6);

    // Declare Variables
    uint16_t period = 1;
    USER_DATA userInput = {0};

    // Set variables to correct initial values
    resetUserInput(&userInput);

    // Load colors stored in EEPROM
    loadColors();

    //test red LED at startup
    setRgbColor(1023,0,0);
    waitMicrosecond(1000000);

    //test green LED at startup
    setRgbColor(0,1023,0);
    waitMicrosecond(1000000);

    //test blue LED at startup
    setRgbColor(0,0,1023);
    waitMicrosecond(1000000);

    // turn all LEDs off
    setRgbColor(0, 0, 0);

    //test green LED is functioning correctly (turn on and then off)
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

    //Print Main Menu
    printMainMenu();

    //stay in while loop until program is exited to get next input from the terminal
    while(true)
    {
        // If User Input detected, then process input
        if(kbhitUart0())
        {
            // Disable Timer1 if in periodic mode
            if(periodicMode)
            {
                disableIntTimer1();
            }

            // Get User Input
            getsUart0(&userInput);

            // Tokenize User Input
            parseFields(&userInput);
        }

        // Perform Command from User Input
        if(userInput.endOfString && isCommand(&userInput, "reset", 1))
        {
            // Store Learned Colors in EEPROM before reseting device
            storeColors();

            // Set flag for micro-controller reboot
            rebootFlag = true;

            // Reset Input from User to Rx Next Command
        }
        else if(userInput.endOfString && isCommand(&userInput, "calibrate", 1))
        {
            // Instructs the hardware to calibrate the white color balance
            // and displays the duty cycle information when complete
            threshold = getFieldInteger(&userInput, 1);

            calibrateMode = true;
            testMode = false;
            calibrateLed(threshold);

            //turn off LEDs when finished with test
            setRgbColor(0,0,0);
        }
        else if(userInput.endOfString && isCommand(&userInput, "color", 2))  //Stores the current color as color reference N (N = 0..15)
        {
            bool flag = false;
            char buffer[QUEUE_BUFFER_LENGTH];

            color.index = getFieldInteger(&userInput, 1);

            //turn deltaMode off to print values when using trigger.
            if(delta.mode)
            {
                delta.mode = false;
                flag = true;
            }

            //take measurement and store value
            getMeasurement();

            color.redValue[color.index] = ledRed;
            color.greenValue[color.index] = ledGreen;
            color.blueValue[color.index] = ledBlue;
            color.validBit[color.index] = true;

            //turn deltaMode back on if it was in that state when entering the command
            if(flag == true)
            {
                delta.mode = true;
            }

            snprintf(buffer, sizeof(buffer), "  color %u stored.\r\n", color.index);
            sendUart0String(buffer);
        }
        else if(userInput.endOfString && isCommand(&userInput, "erase", 2)) //Erases color reference N (N = 0..15)
        {
            int index;
            char buffer[QUEUE_BUFFER_LENGTH];

            index = getFieldInteger(&userInput, 1);

            //delete color from user input
            if(color.validBit[index])
            {
                // Erase color in EEPROM
                eraseColor(index);

                color.validBit[index] = false;

                //print raw results in comparison
                snprintf(buffer, sizeof(buffer), "  color %u erased.\r\n", index);
                sendUart0String(buffer);
            }
            else
            {
                sendUart0String("  NO color to erase.\r\n");
            }
        }
        else if(userInput.endOfString && isCommand(&userInput, "periodic", 2)) // Configures the hardware to send an RGB triplet in 8-bit calibrated format every 0.1 x T seconds, where T = 0..255 or off
        {
            period = getFieldInteger(&userInput, 1);

            if(!validCalibration)
            {
                sendUart0String("  NO calibration performed.\r\n");
            }
            else
            {
                // Need to modify value entered into periodicT function
                periodicT(period);
            }
        }
        else if(userInput.endOfString && isCommand(&userInput, "delta", 2))
        {
            delta.value = getFieldInteger(&userInput, 1);

            enableIntTimer1(period);

            if(delta.value < 0)
            {
                delta.value = 0;
            }
            else if(delta.value > 255)
            {
                delta.value = 255;
            }

            // Configures the hardware to send an RGB triplet when the RMS average
            // of the RGB triplet vs the long-term average (IIR filtered, alpha = 0.9)
            // changes by more than D, where D = 0..255 or off
            delta.mode = true;
        }
        else if(userInput.endOfString && isCommand(&userInput, "match", 2))
        {
            matchValue = getFieldInteger(&userInput, 1);

            enableIntTimer1(period);

            // Configures the hardware to send an RGB triplet when the Euclidean
            // distance (error) between a sample and one of the color reference (R,G,B)
            // is less than E, where E = 0..255 or off
            if(matchValue == 0)
            {
                matchMode = false;
            }
            else if (matchValue < 256)
            {
                matchMode = true;
            }
        }
        else if(userInput.endOfString && isCommand(&userInput, "trigger", 1)) // Configures the hardware to send an RGB triplet immediately
        {
            delta.mode = false;

            getMeasurement();
        }
        else if(userInput.endOfString && isCommand(&userInput, "button", 1)) // Configures the hardware to send an RGB triplet when the PB is pressed
        {
            delta.mode = false;

            sendUart0String("  Press Push Button to Continue.\r\n");

            // Wait for PB press
            waitPbPress();

            // Get Measurement after PB pressed
            getMeasurement();
        }
        else if(userInput.endOfString && isCommand(&userInput, "led", 2)) // Manipulate green status LED
        {
            char buffer[10];

            getFieldString(&userInput, buffer, 1);

            //Disable the green status LED
            if(strcmp(buffer, "off") == 0)
            {
                setPinValue(GREEN_LED, 0); // Turn off GREEN_LED

                sampleLed = false;
            }
            else if(strcmp(buffer, "on") == 0) //Enable the green status LED
            {
                setPinValue(GREEN_LED, 1); // Turn on GREEN_LED

                sampleLed = false;
            }
            else if(strcmp(buffer, "sample") == 0) //Blinks the green status LED for each sample taken
            {
                sampleLed = true;
            }
        }
        else if(userInput.endOfString && isCommand(&userInput, "test", 1))
        {
            //Drives up the LED from a DC of 0 to 255 on red, green, and blue LEDs
            //separately and outputs the uncalibrated 12-bit light intensity in tabular form

            if(periodicMode) //turn off periodic mode for test command
            {
                disableIntTimer1();
                periodicMode = false; //turn off Timer1Isr
                sendUart0String("  Periodic mode disabled.\r\n");
            }

            calibrateMode = false;
            testMode = true;
            testLED();              //perform test of r,g,b LEDs
            setRgbColor(0,0,0);     //turn off LEDs when finished with test

        }
        else if(userInput.endOfString && isCommand(&userInput, "print", 2)) // Manipulate green status LED
        {
            char buffer[10];

            getFieldString(&userInput, buffer, 1);

            //Disable the green status LED
            if(strcmp(buffer, "off") == 0)
            {
                printTest = false;
            }
            else if(strcmp(buffer, "on") == 0) //Enable the green status LED
            {
                printTest = true;
            }
            else if(strcmp(buffer, "colors") == 0)
            {
                printLearnedColors();
            }
        }
        else if(userInput.endOfString && isCommand(&userInput, "set", 4))
        {
            uint16_t red, green, blue;

            // Get RGB values
            red = getFieldInteger(&userInput, 1);
            green = getFieldInteger(&userInput, 2);
            blue = getFieldInteger(&userInput, 3);

            // Set RGB  values
            setRgbColor(red, green, blue);
        }

        // Reset user input
        if(userInput.endOfString)
        {
            resetUserInput(&userInput);
        }
    }
}
