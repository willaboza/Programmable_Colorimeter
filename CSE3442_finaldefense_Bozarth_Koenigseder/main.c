// CSE3442 Project
// William Bozarth and Sigmund Koenigseder

//**********************************************************************************************
//                                       Hardware Target
//**********************************************************************************************

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
//    Green LED:
//        PF3 drives an NPN transistor that powers the green LED

//    UART Interface:
//        U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//        The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//        Configured to 115,200 baud, 8N1

//    Frequency counter and timer input:
//        FREQ_IN (WT5CCP0) on PD6

//    LSensor:
//        AIN0/PE3 is driven by the sensor
//************************************************************************************************
//                          Device Includes, Defines, and Assembler Directives
//************************************************************************************************

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "tm4c123gh6pm.h"

#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
#define PUSH_BUTTON  (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 4*4)))

#define GREEN_LED_MASK 8
#define PUSH_BUTTON_MASK 16
#define MAX_CHARS 80
#define MAX_FIELDS 5
#define MAX_PWM 1023
#define DELTA_MAX 500000
#define TOTAL_COLORS 16
#define RAMP_SPEED 20000

/**********************************************************************************************************/
//                                    Global Variables
/**********************************************************************************************************/

//--- getString and Token Variables ---
static uint8_t pos[MAX_FIELDS] = {0};
static uint8_t type[MAX_FIELDS] = {0};
static uint8_t fieldCount = 0;
bool tokenFlag = false;

//------- delta D Variables ------------
static int deltaValue = 0;
static bool deltaMode = false;
float methodResult = 0.0;
float methodValues[16] = {0.0};
float sum = 0.0;
int index = 0;
int difference = 0;

//----- Trigger Variales ---------------
static uint16_t ledRed = 0;
static uint16_t ledGreen = 0;
static uint16_t ledBlue = 0;

//------- Test Variables ---------------
static uint16_t redLedCal[1024] = {0};
static uint16_t greenLedCal[1024] = {0};
static uint16_t blueLedCal[1024] = {0};
static uint16_t rgbLeds[3] = {0};
bool validTest = false;
bool printTest = false;

//----- Calibrate Variables ------------
static int threshold = 0;
bool validCalibration = false;
bool calibrateMode = false;

//---- Command Variables ----------------
char c;
static char buffer[MAX_CHARS];
static char cmd[MAX_CHARS];
static bool characterTypedFlag = false;
bool sampleLed = false;
bool ok = true;

//--- periodic Mode Variables -----------
static bool periodicMode = false;
static uint16_t periodicValue = 0;
int loadValue = 0;

//--- color N and erase N Variables -----
uint16_t colorIndex = 0;
int colorRedValue[TOTAL_COLORS] = {0};
int colorGreenValue[TOTAL_COLORS] = {0};
int colorBlueValue[TOTAL_COLORS] = {0};
bool colorValid[TOTAL_COLORS] = {false};

//-------- match E Variables ------------
uint8_t matchValue = 0;
float eColor = 0.0;
bool matchMode = false;
bool match = false;
bool testMode = false;
/**********************************************************************************************************/
//                                    Function Declarations
/**********************************************************************************************************/

void waitMicrosecond(uint32_t us);
void putcUart0(char c);
void putsUart0(char* str);
void getsUart0(char* str, uint8_t maxChars);
void testLED();
void calibrateLed();
void waitPbPress();
void getMeasurement();
void printMenu();
void setTriplet();
void tokenizeString();
void setRgbColor(uint16_t red, uint16_t green, uint16_t blue);
void periodicT();
void deltaD();
void euclidNorm();
void getMenuCmd();
char getcUart0();
char* getString(uint8_t args);
int normalizeRgbColor(int measurement);
int16_t readAdc0Ss3();
uint16_t getRgbValue(char* buffer);
uint16_t readRawResult();
uint16_t getValue(uint16_t args);
bool isCommand(const char * str, uint8_t args);

/**********************************************************************************************************/
//                                    Start of Subroutines
/**********************************************************************************************************/

//initialize the Wide5Timer for setting interrupts using frequency.
void setCounterMode()
{
    WTIMER5_CTL_R &= ~TIMER_CTL_TAEN;                // turn-off counter before reconfiguring
    WTIMER5_CFG_R = 4;                               // configure as 32-bit counter (A only)
    WTIMER5_TAMR_R = TIMER_TAMR_TAMR_CAP | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
    WTIMER5_CTL_R = 0;                               //
    WTIMER5_IMR_R = 0;                               // turn-off interrupts
    WTIMER5_TAV_R = 0;                               // zero counter for first period
    WTIMER5_CTL_R |= TIMER_CTL_TAEN;                 // turn-on counter
    NVIC_EN3_R &= ~(1 << (INT_WTIMER5A-16-96));      // turn-off interrupt 120 (WTIMER5A)
}

//----------------------------------- Initialize Hardware -------------------------------------------------
void initHw(){
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz// PWM is system clock / 2
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S)| SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_2;

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

//-------------------------------- Enable GPIO port A,B,E, and F peripherals --------------------------------------

    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOA | SYSCTL_RCGC2_GPIOB | SYSCTL_RCGC2_GPIOD | SYSCTL_RCGC2_GPIOE | SYSCTL_RCGC2_GPIOF;
    SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R2;           // turn-on SSI2 clocking
    SYSCTL_RCGC0_R |= SYSCTL_RCGC0_PWM0;             // turn-on PWM0 module
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;       // turn-on timer
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R5;     // turn-on wide timer 5 clocking
    SYSCTL_RCGCADC_R |= 1;                           // turn on ADC module 0 clocking
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;         // turn-on UART0, leave other UARTs in same status

//------------------------- Configure Green and Red on-board LED and pushbutton pin -------------------------------

    // Configure LED and pushbutton pins
    GPIO_PORTF_DIR_R = GREEN_LED_MASK;  // bits 1, 2, and 3 are outputs, other pins are inputs
    GPIO_PORTF_DR2R_R = GREEN_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R = PUSH_BUTTON_MASK | GREEN_LED_MASK;  // enable LEDs and pushbuttons
    GPIO_PORTF_PUR_R = PUSH_BUTTON_MASK; // enable internal pull-up for push button

//----------------------------- Configure Three Backlight LEDs (R, G, B) ------------------------------------------

    GPIO_PORTB_ODR_R |= 0x20; // enable open drain select for RED_LED_MASK
    GPIO_PORTB_DIR_R |= 0x20;   // make bit5 an output
    GPIO_PORTB_DR2R_R |= 0x20;  // set drive strength to 2mA
    GPIO_PORTB_DEN_R |= 0x20;   // enable bit5 for digital
    GPIO_PORTB_AFSEL_R |= 0x20; // select auxilary function for bit 5
    GPIO_PORTB_PCTL_R = GPIO_PCTL_PB5_M0PWM3; // enable PWM on bit 5

    GPIO_PORTE_ODR_R |= 0x30; // enable open drain select for RED, BLUE, and GREEN
    GPIO_PORTE_DIR_R |= 0x30;   // make bits 4 and 5 outputs
    GPIO_PORTE_DR2R_R |= 0x30;  // set drive strength to 2mA
    GPIO_PORTE_DEN_R |= 0x30;   // enable bits 4 and 5 for digital
    GPIO_PORTE_AFSEL_R |= 0x30; // select auxilary function for bits 4 and 5
    GPIO_PORTE_PCTL_R = GPIO_PCTL_PE4_M0PWM4 | GPIO_PCTL_PE5_M0PWM5; // enable PWM on bits 4 and 5

//-------------------------- Configure PWM Module0 to Drive RGM Backlight -------------------------------------

    // RED   on M0PWM3 (PB5), M0PWM1b
    // BLUE  on M0PWM4 (PE4), M0PWM2a
    // GREEN on M0PWM5 (PE5), M0PWM2b

    __asm(" NOP");                                   // wait 1 clock
    __asm(" NOP");                                   // wait 1 clock
    __asm(" NOP");                                   // wait 1 clock

    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state
    PWM0_1_CTL_R = 0;                                // turn-off PWM0 generator 1
    PWM0_2_CTL_R = 0;                                // turn-off PWM0 generator 2

    PWM0_1_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
    // output 3 on PWM0, gen 1b, cmpb
    PWM0_2_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_ONE;
    // output 4 on PWM0, gen 2a, cmpa
    PWM0_2_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_ONE;
    // output 5 on PWM0, gen 2b, cmpb

    // set period to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz
    PWM0_1_LOAD_R = 1024;
    PWM0_2_LOAD_R = 1024;

    //Below line of code commented out is non-inverted when set to 0 and inverted when set to 1
    //PWM0_INVERT_R = PWM_INVERT_PWM3INV | PWM_INVERT_PWM4INV | PWM_INVERT_PWM5INV;

    // invert outputs for duty cycle increases with increasing compare values
    PWM0_1_CMPB_R = 0;                               // red off (0=always low, 1023=always high)
    PWM0_2_CMPB_R = 0;                               // green off
    PWM0_2_CMPA_R = 0;                               // blue off

    PWM0_1_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 1
    PWM0_2_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM0 generator 2
    PWM0_ENABLE_R = PWM_ENABLE_PWM3EN | PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN;// enable outputs

    // Configure A0 and ~CS for graphics LCD
    GPIO_PORTB_DIR_R |= 0x42;  // make bits 1 and 6 outputs
    GPIO_PORTB_DR2R_R |= 0x42; // set drive strength to 2mA
    GPIO_PORTB_DEN_R |= 0x42;  // enable bits 1 and 6 for digital

    // Configure SSI2 pins for SPI configuration
    GPIO_PORTB_DIR_R |= 0x90;                        // make bits 4 and 7 outputs
    GPIO_PORTB_DR2R_R |= 0x90;                       // set drive strength to 2mA
    GPIO_PORTB_AFSEL_R |= 0x90;                      // select alternative functions for MOSI, SCLK pins
    GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB7_SSI2TX | GPIO_PCTL_PB4_SSI2CLK; // map alt fns to SSI2
    GPIO_PORTB_DEN_R |= 0x90;                        // enable digital operation on TX, CLK pins

    // Configure the SSI2 as a SPI master, mode 3, 8bit operation, 1 MHz bit rate
    SSI2_CR1_R &= ~SSI_CR1_SSE;                      // turn off SSI2 to allow re-configuration
    SSI2_CR1_R = 0;                                  // select master mode
    SSI2_CC_R = 0;                                   // select system clock as the clock source
    SSI2_CPSR_R = 40;                                // set bit rate to 1 MHz (if SR=0 in CR0)
    SSI2_CR0_R = SSI_CR0_SPH | SSI_CR0_SPO | SSI_CR0_FRF_MOTO | SSI_CR0_DSS_8; // set SR=0, mode 3 (SPH=1, SPO=1), 8-bit
    SSI2_CR1_R |= SSI_CR1_SSE;                       // turn on SSI2

//------------------------------------ Configure FREQ_IN for frequency counter ------------------------------------------

    GPIO_PORTD_AFSEL_R |= 0x40;                      // select alternative functions for FREQ_IN pin
    GPIO_PORTD_PCTL_R &= ~GPIO_PCTL_PD6_M;           // map alt fns to FREQ_IN
    GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD6_WT5CCP0;
    GPIO_PORTD_DEN_R |= 0x40;                        // enable bit 6 for digital input

//    //Configure Wide Timer 5 as counter
//    if (timeMode)
//        setTimerMode();
//    else
    setCounterMode();

//----------------------------------------- Configure UART0 Pins --------------------------------------------------------

    GPIO_PORTA_DIR_R |= 2;                           // enable output on UART0 TX pin: default, adde
    GPIO_PORTA_DEN_R |= 3;                           // enable digital on UART0 pins: default, added for clarity
    GPIO_PORTA_AFSEL_R |= 3;                         // use peripheral to drive PA0, PA1: default, added for clarity
    GPIO_PORTA_PCTL_R &= 0xFFFFFF00;                 // set fields for PA0 and PA1 to zero
    GPIO_PORTA_PCTL_R = GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                         // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to 115200 baud, 8N1 format (must be 3 clocks from clock enable and config writes)
    UART0_CTL_R = 0;                                 // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                  // use system clock (40 MHz)
    UART0_IBRD_R = 21;                               // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART0_FBRD_R = 45;                               // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN; // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN; // enable TX, RX, and module
    UART0_IM_R = UART_IM_RXIM;                       // turn-on RX interrupt
    NVIC_EN0_R |= 1 << (INT_UART0-16);               // turn-on vector number 21, or interrupt number 5, (UART0)

//--------------------------------------- Configure Timer 1 as the time base ----------------------------------------

    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_TAILR_R = 0x3D0900;                        // set load value to 4,000,000
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    NVIC_EN0_R |= 1 << (INT_TIMER1A-16);             // turn-on vector number 37, or interrupt 21, (TIMER1A)
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer

//------------------------------------------  Configure ADC Pins ----------------------------------------------------------

    // Configure AN0 as an analog input
    GPIO_PORTE_AFSEL_R |= 0x08;                      // select alternative functions for AN0 (PE3)
    GPIO_PORTE_DEN_R &= ~0x08;                       // turn off digital operation on pin PE3
    GPIO_PORTE_AMSEL_R |= 0x08;                      // turn on analog operation on pin PE3

    // Configure ADC
    ADC0_CC_R = ADC_CC_CS_SYSPLL;                    // select PLL as the time base (not needed, since default value)
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3;                // disable sample sequencer 3 (SS3) for programming
    ADC0_EMUX_R = ADC_EMUX_EM3_PROCESSOR;            // select SS3 bit in ADCPSSI as trigger
    ADC0_SSMUX3_R = 0;                               // set first sample to AN0
    ADC0_SSCTL3_R = ADC_SSCTL3_END0;                 // mark first sample as the end
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN3;                 // enable SS3 for operation

}
/*********************************************************************************************************/
//                                        Start of Interrupts
/*********************************************************************************************************/

// Frequency counter service publishing latest frequency measurements every second
void Timer1Isr()
{
    if (periodicMode == true)
    {
        WTIMER5_TAV_R = 0;                           // reset counter for next period
        getMeasurement();                            // get rgb triplet measurement and display
    }

    TIMER1_ICR_R = TIMER_ICR_TATOCINT;               // clear interrupt flag
}

/*********************************************************************************************************/
//                                      Start of Main Function
/*********************************************************************************************************/

int main(void){

    // Initialize hardware
    initHw();

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
    GREEN_LED = 1;
    waitMicrosecond(1000000);
    GREEN_LED = 0;

    //Print Main Menu
    printMenu();
    putsUart0("\r\n");

    //stay in while loop until program is exited to get next input from the terminal
    while(true)
    {
        getMenuCmd();

        tokenFlag = false;       //reset flag for tokenizing new cmd string

        //Instructs the hardware to calibrate the white color balance and displays
        //the duty cycle information when complete
        if(isCommand("calibrate", 105) ==  true && ok != false)
        {
            //turn off periodic mode for calibration command
            if(periodicMode == true)
            {
                periodicMode = false;   //turn off Timer1Isr
                putsUart0("Periodic mode disabled.\r\n");
            }
            calibrateMode = true;
            testMode = false;
            calibrateLed();
            printTest = false;
            setRgbColor(0,0,0);     //turn off LEDs when finished with test
        }
        //Stores the current color as color reference N (N = 0..15)
        else if(isCommand("color", 100) == true && ok != false)
        {
            bool flag = false;
            //turn deltaMode off to print values when using trigger.
            if(deltaMode == true)
            {
                deltaMode = false;
                flag = true;
            }
            //take measurement and store value
            getMeasurement();
            colorRedValue[colorIndex] = ledRed;
            colorGreenValue[colorIndex] = ledGreen;
            colorBlueValue[colorIndex] = ledBlue;
            colorValid[colorIndex] = true;
            //turn deltaMode back on if it was in that state when entering the command
            if(flag == true)
            {
                deltaMode = true;
            }

            snprintf(buffer, sizeof(buffer), "color %u stored.\r\n", colorIndex);
            putsUart0(buffer);
            putsUart0("\r\n");

        }
        //Erases color reference N (N = 0..15)
        else if(isCommand("erase", 101) == true && ok != false)
        {
            //delete color from user input
            if(colorValid[colorIndex] ==  true)
            {
                colorValid[colorIndex] = false;
                colorRedValue[colorIndex] = 0;
                colorGreenValue[colorIndex] = 0;
                colorBlueValue[colorIndex] = 0;

                //print raw results in comparison
                snprintf(buffer, sizeof(buffer), "color %u erased.\r\n", colorIndex);
                putsUart0(buffer);
                putsUart0("\r\n");
            }
            else
            {
                putsUart0("No color stored at position to erase.\r\n");
            }
        }
        //Configures the hardware to send an RGB triplet in 8-bit calibrated format
        //every 0.1 x T seconds, where T = 0..255 or off
        else if(isCommand("periodic", 102) == true && ok != false)
        {
            periodicMode = false;

            if(validCalibration == false)
            {
                putsUart0("Calibration needs to be completed before entering periodic mode.\r\n");
            }
            else
            {
                 periodicT();
            }
        }
        //Configures the hardware to send an RGB triplet when the RMS average
        //of the RGB triplet vs the long-term average (IIR filtered, alpha = 0.9)
        //changes by more than D, where D = 0..255 or off
        else if(isCommand("delta", 103) == true && ok != false)
        {
            deltaMode = true;
        }
        //Configures the hardware to send an RGB triplet when the Euclidean
        //distance (error) between a sample and one of the color reference (R,G,B)
        //is less than E, where E = 0..255 or off
        else if(isCommand("match", 104) == true && ok != false)
        {
            if(matchValue == 0)
            {
                matchMode = false;
            }
            else if (matchValue < 256)
            {
                matchMode = true;
            }
        }
        //Configures the hardware to send an RGB triplet immediately
        else if(isCommand("trigger", 0) == true && ok != false)
        {

            deltaMode = false;
            //turn off periodic mode when using trigger command
            if(periodicMode == true)
            {
                periodicMode = false;   //turn off Timer1Isr
                putsUart0("Periodic mode disabled.\r\n");
            }

            getMeasurement();
        }
        //Configures the hardware to send an RGB triplet when the PB is pressed
        else if(isCommand("button", 0) == true && ok != false)
        {
            //turn off periodic mode when using button command
            if(periodicMode == true)
            {
                periodicMode = false;   //turn off Timer1Isr
                putsUart0("Periodic mode disabled.\r\n");
            }
            deltaMode = false;
            // Wait for PB press
            putsUart0("Press Push Button to Continue.\r\n");
            waitPbPress();
            getMeasurement();
        }
        //Manipulate green status LED
        else if(isCommand("led", 0) == true && ok != false)
        {
            //Disable the green status LED
            if(isCommand("off", 0) == true && ok != false)
            {
                GREEN_LED = 0;
                sampleLed = false;
            }
            //Enable the green status LED
            else if(isCommand("on", 0) == true && ok != false)
            {
                GREEN_LED = 1;
                sampleLed = false;
            }
            //Blinks the green status LED for each sample taken
            else if(isCommand("sample", 0) == true && ok != false)
            {
                sampleLed = true;
            }
            else
            {
                ok = false;
            }
        }
        //Drives up the LED from a DC of 0 to 255 on red, green, and blue LEDs
        //separately and outputs the uncalibrated 12-bit light intensity in tabular form
        else if(isCommand("test", 106) == true && ok != false)
        {
            //turn off periodic mode for test command
            if(periodicMode == true)
            {
                periodicMode = false;   //turn off Timer1Isr
                putsUart0("Periodic mode disabled.\r\n");
            }
            calibrateMode = false;
            testMode = true;
            testLED();              //perform test of r,g,b LEDs
            printTest = false;
            setRgbColor(0,0,0);     //turn off LEDs when finished with test
        }
        //Re-print Main Menu
        else if(isCommand("menu", 0) == true && ok != false)
        {
            periodicMode = false;   //turn off Timer1Isr
            putsUart0("\r\n");
            printMenu();
            putsUart0("\r");
        }
        //Command used to exit program
        else if(isCommand("exit", 0) == true && ok != false)
        {
            putsUart0("Exiting Program ... ");
            //turn off LEDs before exiting program.
            setRgbColor(0,0,0);
            exit(0);
        }
        else if(isCommand("set", 0) == true && ok != false)
        {
            setTriplet();
        }
        //If no command recognized print statement below
        else
        {
            putsUart0("Command Not Recognized.\r\n");
            putsUart0("\r\n");
            ok = true;
        }
    ok = true;
    }
}
/*********************************************************************************************************/
//                                      Start of Functions List
/*********************************************************************************************************/
// Approximate busy waiting (in units of microseconds), given a 40 MHz system clock
void waitMicrosecond(uint32_t us)
{
    __asm("WMS_LOOP0:   MOV  R1, #6");          // 1
    __asm("WMS_LOOP1:   SUB  R1, #1");          // 6
    __asm("             CBZ  R1, WMS_DONE1");   // 5+1*3
    __asm("             NOP");                  // 5
    __asm("             NOP");                  // 5
    __asm("             B    WMS_LOOP1");       // 5*2 (speculative, so P=1)
    __asm("WMS_DONE1:   SUB  R0, #1");          // 1
    __asm("             CBZ  R0, WMS_DONE0");   // 1
    __asm("             NOP");                  // 1
    __asm("             B    WMS_LOOP0");       // 1*2 (speculative, so P=1)
    __asm("WMS_DONE0:");                        // ---
                                                // 40 clocks/us + error
}
// Blocking function that writes a serial character when the UART buffer is not full
void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}
// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i;
    for (i = 0; i < strlen(str); i++)
      putcUart0(str[i]);
}
// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    while (UART0_FR_R & UART_FR_RXFE);               // wait if uart0 rx fifo empty
    return UART0_DR_R & 0xFF;                        // get character from fifo
}
// Blocking function that returns with serial data and stores in a character array
void getsUart0(char* str, uint8_t maxChars)
{
    int count = 0;
    char temp;
    bool flag = false;

    //use flag to skip getcUart0() function call if key pressed to stop display of period T values to monitor
    //if false do not call getcUart0 and if true set flag to false so next char can be retrieved from user.
    if(characterTypedFlag == false){
        //call function getcUart0 and store first typed character in a "temporary" variable
        temp = getcUart0();
    }
    else
    {
        temp = str[count];
        characterTypedFlag = false;
    }
    flag = true;
    //loop used to determine which characters are stored char* str. Variable char temp used for logical comparisons
    while(temp != '\0' && count <= maxChars)
    {
          //call function getcUart0 if the value of count is greater than zero but less than maxChars
          if((count >= 0 && (count < maxChars)) && flag == false)
          {
              temp = getcUart0();
          }
          //if the integer value of count equals the integer value of maxChars
          //then store '\0' in str and temp. Finally increment the value of count by 1.
          else if(count == (maxChars))
          {
              str[count] = '\0';
              temp = '\0';
              count++;
          }
          //if backspace entered into terminal and the count is greater than 1 then decrement count by 1.
          if(temp == '\b' && count != 0)
          {
              count--;
          }
          //if return or enter key typed into terminal then store '\0' into both str and temp variables.
          else if(temp == '\r')
          {
              str[count] = '\0';
              temp = '\0';
          }
          //filter out any capital letters and store those that are not capitalized in str and increment count.
          else if(' ' <= temp)
          {
              //if a capital letter is typed into the terminal then convert to lower case and increment count.
              if('A' <= temp && temp <= 'Z')
              {
                 str[count] = temp + 32;            //converts capital letter to lower case
                 count++;
              }
              else
              {
              str[count] = temp;                   //if character already lower case store value in str and increment count by one
              count++;
              }
          }
          flag = false;
    }
    strcpy(cmd,str);
}
//Function to tokenize strings
void tokenizeString()
{
    int count = 0;
    int field = 0;
    char temp;
    bool delim = true;

    temp = cmd[count];

    while(temp != '\0' && count < MAX_CHARS)
    {
        //Only comparing against lower case as all capital letters
        //are converted to lower case before entering function.
        if('a' <= temp && temp <= 'z')
        {
            if(delim == true)
            {
                pos[field] = count;       //sets the position from the transition of delimiter to alpha.....
                type[field] = 'A';        //A is Alpha
                fieldCount++;             //Count of number of Alphanumeric tokens detected
                field++;
                delim = false;            //Flag variable
            }
            count++;                      //traverse
            temp = cmd[count];            //store next position
        }
        //Only verifying if value is a number between 0 and 9. Might add capability
        //to detect math operators later.
        else if(('0' <= temp && temp <= '9') || ',' == temp ||  temp == '.') //Code executes for numerics same as alpha
        {
            if(delim == true)
            {
                pos[field] = count;       //sets the position from the transition of delimiter to numeric.....
                type[field] = 'N';        //set token as a number
                fieldCount++;             //count of alphanumeric tokens detected
                field++;
                delim = false;            //flag variable
            }
            count++;
            temp = cmd[count];
        }
        //Anything else is being considered a delimiter as of now. Will change
        //detected delimiter and inserted null value in place
        else
        {
            delim = true;
            cmd[count] = '\0';
            count++;
            temp = cmd[count];
        }
    }
}
//Set Red,Green,and Blue LED Colors
void setRgbColor(uint16_t red, uint16_t green, uint16_t blue)
{
    PWM0_1_CMPB_R = red;      //set value recorded for red
    PWM0_2_CMPA_R = blue;     //set value recorded for blue
    PWM0_2_CMPB_R = green;    //set value recorded for green
}
//Function Used to Determine if Correct Command Entered
bool isCommand(const char* str, uint8_t args)
{
    bool validCmd = false;
    uint8_t i = 0;
    uint16_t temp = 0;

    //flag so string is only tokenized once per entered command
    if(tokenFlag != true)
    {
        tokenizeString();       //pass cmd character array into the string tokenize function
        tokenFlag = true;
    }

    //while variable 'i' is less than the number of field counts from tokenized string
    //and the boolean variable, validCmd, is false, if token type is an alpha character then compare
    //the character array command with the tokenized string return from the getString function.
    //if they are the same set validCmd to true and return value. Otherwise validCmd is not a match and
    //the logic remains false.
    while(i < fieldCount)
    {
        if((type[i] ==  'A' || type[i] == 'N') && validCmd != true)
        {
            if(strcmp(str, getString(i)) == 0)
            {
                validCmd = true;
            }
        }
        //store any valid integer for color command
        else if(validCmd == true && args == 100)
        {
            if(type[i] == 'N')
            {
                temp = getValue(i);
                if(temp < 16)
                {
                    colorIndex = temp;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("Invalid entry.\r\n");
                    break;
                }
            }
            else
            {
                ok = false;
                putsUart0("Invalid entry.\r\n");
                break;
            }
        }
        //store any valid integer for erase command
        else if(validCmd == true && args == 101)
        {
            if(type[i] == 'N')
            {
                temp = getValue(i);
                if(temp < 16)
                {
                    colorIndex = temp;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("Invalid entry.\r\n");
                    break;
                }
            }
            else
            {
                ok = false;
                putsUart0("Invalid entry.\r\n");
                break;
            }
        }
        //store any valid integer for periodic command
        else if(validCmd == true && args == 102)
        {
            if(type[i] == 'N')
            {
                temp = getValue(i);
                if(temp <= 255)
                {
                    periodicValue = temp;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("Invalid entry.\r\n");
                    break;
                }
            }
            else if(type[i] == 'A')
            {
                if(strcmp("off", getString(i)) == 0)
                {
                    periodicValue = 0;
                    ok = true;
                    break;
                }
                else if(strcmp("max", getString(i)) == 0)
                {
                    periodicValue = 255;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("Invalid entry.\r\n");
                    break;
                }
            }
        }
        //store any valid integer for delta command
        else if(validCmd == true && args == 103)
        {
            if(type[i] == 'N')
            {
                temp = getValue(i);
                if(temp == 0)
                {
                    int i;
                    //turn off delta D mode
                    deltaMode = true;

                    for (i = 0; i < 16; i++)
                    {
                        methodValues[i] = 0;
                    }
                    break;
                }
                else if(temp <= 255)
                {
                    //sum = 0.0;
                    deltaValue = temp;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("No change to delta D due to invalid entry.\r\n");
                    break;
                }
            }
            else if(type[i] == 'A')
            {
                if(strcmp("off", getString(i)) == 0)
                {
                    int i;
                    deltaMode = false;
//                    methodResult = 0.0;
//                    difference = 0;
//                    index = 0;
//
//                    for (i = 0; i < 16; i++)
//                    {
//                        methodValues[i] = 0;
//                    }
                    ok = true;
                    break;
                }
                else if(strcmp("max", getString(i)) == 0)
                {
                    deltaMode = true;
                    deltaValue = DELTA_MAX;
                    snprintf(buffer, sizeof(buffer), "%u.0\0",DELTA_MAX);
                    methodResult = atof(buffer);
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("No change to delta D due to invalid entry.\r\n");
                    break;
                }
            }
        }
        //store any valid integer for match command
        else if(validCmd == true && args == 104)
        {
            if(type[i] == 'N')
            {
                temp = getValue(i);
                if(temp <= 255)
                {
                    matchValue = temp;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("Invalid match command.\r\n");
                    break;
                }
            }
            else if(type[i] == 'A')
            {
                if(strcmp("off", getString(i)) == 0)
                {
                    matchValue = 0;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("Invalid match command.\r\n");
                    break;
                }
            }
        }
        //store any valid integer for calibration command
        else if(validCmd == true && args == 105)
        {
            if(type[i] == 'N')
            {
                temp = getValue(i);
                if(temp <= 4095)
                {
                    threshold = temp;
                    ok = true;
                    break;
                }
                else
                {
                    ok = false;
                    putsUart0("Invalid calibration command.\r\n");
                    break;
                }
            }
            else
            {
                ok = false;
                putsUart0("Invalid calibration command.\r\n");
                break;
            }
        }
        //store any valid test command
        else if(validCmd == true && args == 106)
        {
            if(strcmp("print", getString(i)) == 0)
            {
                printTest = true;
                ok = true;
                break;
            }
        }
        i++;
    }

    return validCmd;
}
//Function to return a token as a string
char* getString(uint8_t count)
{
    int i = 0;
    count = pos[count];

    //store string at beginning of position
    //in an array to be returned.
    while(cmd[count] != '\0')
    {
        buffer[i] =  cmd[count];

        i++;
        count++;
    }

    buffer[i++] = '\0';        //add null character to end of char array

    return buffer;
}
//Function to return a token as an integer
uint16_t getValue(uint16_t count)
{
    uint16_t num = 0;
    int i = 0;

    count = pos[count];

    //store string in array to be converted to an
    //integer and returned to parent function.
    while(cmd[count] != '\0')
    {
        buffer[i] =  cmd[count];
        i++;
        count++;
    }

    buffer[i++] = '\0';       //add null character to end of char array

    //check if number entered by user is signed.
    //If it is negative then set num = 0
    i = -1;
    i = atoi(buffer);

    if(i < 0)
    {
        num = 0;
    }
    else
    {
        num = atoi(buffer);
    }

    return num;
}
//Function to read ADC0 using SS3
int16_t readAdc0Ss3()
{
    ADC0_PSSI_R |= ADC_PSSI_SS3;                     // set start bit
    while (ADC0_ACTSS_R & ADC_ACTSS_BUSY);           // wait until SS3 is not busy
    return ADC0_SSFIFO3_R;                           // get single result from the FIFO
}
//Return LED Value for Passing to setRgbColor()
uint16_t getRgbValue(char * buffer)
{
    uint16_t led;

    //fetch value from 0 ---> 1023 and store in buffer. Convert string to integer and return
    //the integer back to parent function.
    getsUart0(buffer, MAX_CHARS);
    led = atoi(buffer);

    return led;
}
//Function used to return a Cumulative Average of raw results
uint16_t readRawResult()
{
    uint16_t raw;

    raw = readAdc0Ss3();            // Read Light Sensor PE3(AN0) Pin

    return raw;
}
//function to test led functionality
void testLED()
{
    int i;
    uint16_t num = 0;
    char buffer[MAX_CHARS];

    setRgbColor(0,0,0);

    putsUart0("wait.... ");
    putsUart0("\r\n");

    //Ramp RED LED
    for (i = 0; i < 1024; i++)
    {
         setRgbColor(i, 0, 0);
         waitMicrosecond(RAMP_SPEED);
         num = readRawResult();

         if(testMode == true)
         {
             redLedCal[i] = num;
         }
         else if(calibrateMode == true && num <= threshold)
         {
             rgbLeds[0] = i;
         }
         else if(calibrateMode == true && num > threshold)
         {
             break;
         }

         if(printTest == true)
         {
             //no spaces between the 4 numbers as these are copy and pasted
             //into excel for plotting purposes.
             snprintf(buffer, sizeof(buffer), "%d,0,0,%u\r\n",i,num);
             putsUart0(buffer);
             putsUart0("\r");
         }
    }

    setRgbColor(0,0,0);

    //Ramp GREEN LED
    for (i = 0; i < 1024; i++)
    {
         setRgbColor(0, i, 0);
         waitMicrosecond(RAMP_SPEED);
         num = readRawResult();

         if(testMode == true)
         {
             greenLedCal[i] = num;
         }
         else if(calibrateMode == true && num <= threshold)
         {
             rgbLeds[1] = i;
         }
         else if(calibrateMode == true && num > threshold)
         {
             break;
         }

         if(printTest == true)
         {
             //no spaces between the 4 numbers as these are copy and pasted
             //into excel for plotting purposes.
              snprintf(buffer, sizeof(buffer), "0,%d,0,%u\r\n",i,num);
              putsUart0(buffer);
              putsUart0("\r");
         }
    }

    setRgbColor(0,0,0);

    //Ramp from off to blue
    for (i = 0; i < 1024; i++)
    {
        setRgbColor(0, 0, i);
        waitMicrosecond(RAMP_SPEED);
        num = readRawResult();

        if(testMode == true)
        {
            blueLedCal[i] = num;
        }
        else if(calibrateMode == true && num <= threshold)
        {
            rgbLeds[2] = i;
        }
        else if(calibrateMode == true && num > threshold)
        {
            break;
        }

        if(printTest ==  true)
        {
             snprintf(buffer, sizeof(buffer), "0,0,%d,%u\r\n",i,num);
             putsUart0(buffer);
             putsUart0("\r");
        }
    }

    putsUart0("\r\n");
    validTest = true;
    testMode = false;
}
//function to calibrate instrument
void calibrateLed()
{
    bool OK = true;

    if(threshold > 4095)
    {
        putsUart0("Threshold Value Outside of Range (0 to 4095).\r\n");
        OK = false;
    }
    else
    {
        testLED();
    }
    //Set threshold values for R,G, and B if no errors detected.
    if(OK == true )
    {
        validCalibration = true;
        //setRgbColor(redLED, greenLED, blueLED);
        snprintf(buffer, sizeof(buffer), "(PWMr: %u,PWMg: %u,PWMb: %u)\r\n", rgbLeds[0], rgbLeds[1], rgbLeds[2]);
        putsUart0(buffer);
        putsUart0("\r\n");
    }
    calibrateMode = false;
}
// Blocking function that returns only when SW1 is pressed
void waitPbPress()
{
    //wait until push button pressed
    while(GPIO_PORTF_DATA_R & PUSH_BUTTON_MASK);
    //wait specified time
    waitMicrosecond(10000);
}
//Function to call when needing to measure for trigger and button commands
void getMeasurement()
{
    char buffer[MAX_CHARS];
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

        if(deltaMode == true)
        {
            deltaD();
        }

        //if match mode is turned on calculate the Euclidean Distance between measured value
        //and stored colors. If eColor < matchValue then display matched color on terminal
        if(matchMode == true)
        {
            for(colorIndex = 0; colorIndex < TOTAL_COLORS; colorIndex++)
            {
                if(colorValid[colorIndex] == true)
                {
                    euclidNorm();

                    if(eColor < matchValue)
                    {
                        snprintf(buffer, sizeof(buffer), "color %u\r\n", colorIndex);
                        putsUart0(buffer);
                        putsUart0("\r\n");
                    }
                }
            }
        }

        if(sampleLed == true)
        {
             GREEN_LED = 1;
        }
        waitMicrosecond(10000);

        //If in delta mode and change in r,g,b is greater than deltaValue then print new values
        if(deltaMode == true && difference > deltaValue)
        {
            //print raw results in comparison
            snprintf(buffer, sizeof(buffer), "(r: %u,g: %u,b: %u).\r\n", ledRed, ledGreen, ledBlue);
            putsUart0(buffer);
            putsUart0("\r\n");
        }
        //if not in delta mode print measured values
        else if(deltaMode == false)
        {
            //print raw results in comparison
            snprintf(buffer, sizeof(buffer), "(r: %u,g: %u,b: %u).\r\n", ledRed, ledGreen, ledBlue);
            putsUart0(buffer);
            putsUart0("\r\n");
        }
        if(sampleLed == true)
        {
            GREEN_LED = 0;
        }
    }
    else
    {
        putsUart0("No valid calibration performed. Please perform calibration.\r\n");
    }
}
//Function to set triplet
void setTriplet()
{
    char buffer[MAX_CHARS];

    //gets user input for manual setting of Red LED value
    putsUart0("Enter Red LED value (from 0 to 1023): ");
    ledRed = getRgbValue(buffer);
    putsUart0("\r");
    getcUart0();
    //gets user input for manual setting of Green LED value
    putsUart0("Enter Green LED value (from 0 to 1023): ");
    ledGreen = getRgbValue(buffer);
    putsUart0("\r");
    getcUart0();
    //gets user input for manual setting of Blue LED value
    putsUart0("Enter Blue LED value (from 0 to 1023): ");
    ledBlue = getRgbValue(buffer);
    putsUart0("\r\n");
    getcUart0();

    setRgbColor(ledRed, ledGreen, ledBlue);
}
//return measured value in the range of 0... 255 using
// m = (m/th)*(tMax-tMin), where th = threshold, tMax
//and tMin is target max and min, and m = measured value
int normalizeRgbColor(int measurement)
{
    int temp;
    float numerator;
    float denominator;
    float ratio;
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
//function to set the period between measurements
void periodicT()
{
    //multiply fc = 40MHz by resolution in units of 0.1s (4,000,000 = 40MHz*0.1)
    loadValue = periodicValue * 4000000;

    //if value of str is '0' or 'off' then deactivate periodic T
    if(periodicValue == 0)
    {
        periodicMode = false;
        putsUart0("periodic T function is OFF\r\n");
    }
    //if value of str is between 1... 255 then store loadValue in TIMER1_TAILR_R,
    //set timeMode to false, periodicMode to true, and turn on Wide5Timer as a counter.
    else if(periodicValue >= 1 && periodicValue <= 255)
    {
        TIMER1_TAILR_R = loadValue;
        periodicMode = true;
        setCounterMode();
    }
}
//function to set the delta value for displaying measured values when greater than D
void deltaD()
{
    float avg = 0.0;

    methodResult = sqrt((ledRed * ledRed) + (ledGreen * ledGreen) + (ledBlue * ledBlue));
    sum -= methodValues[index];
    sum += methodResult;
    methodValues[index] = methodResult;
    index = (index + 1) % 16;

    avg = 0.9*(sum/16) + (0.1 * methodResult);
    difference = abs((int)methodResult - (int)avg);
}
//function used to calculate the Euclidean distance
void euclidNorm()
{
    uint16_t colorDiffRed;
    uint16_t colorDiffBlue;
    uint16_t colorDiffGreen;
    float red;
    float green;
    float blue;

    //calculate (r - ri)^2
    colorDiffRed= ledRed - colorRedValue[colorIndex];
    colorDiffRed = colorDiffRed * colorDiffRed;
    snprintf(buffer, sizeof(buffer), "%u.0\0",colorDiffRed);
    red = atof(buffer);

    //calculate (g - gi)^2
    colorDiffGreen = ledGreen - colorGreenValue[colorIndex];
    colorDiffGreen = colorDiffGreen * colorDiffGreen;
    snprintf(buffer, sizeof(buffer), "%u.0\0",colorDiffGreen);
    green = atof(buffer);

    //calculate (b - bi)^2
    colorDiffBlue = ledBlue - colorBlueValue[colorIndex];
    colorDiffBlue = colorDiffBlue * colorDiffBlue;
    snprintf(buffer, sizeof(buffer), "%u.0\0",colorDiffBlue);
    blue = atof(buffer);

    eColor= sqrt (red + green + blue);       //calculate square root of above differences
}
//function handles input of user command
void getMenuCmd()
{
    //When periodic mode = true use if statement to temporarily interrupt
    //display of periodic measurements to get next command to perform,
    //else proceed as usual and retrieve next command from user
    if(periodicMode == true)
    {
        while (UART0_FR_R & UART_FR_RXFE);               //wait if uart0 rx fifo empty
        periodicMode = false;                            //turn off periodMode while getting input command
        c = UART0_DR_R & 0xFF;                           //get character from fifo
        putsUart0("\b ");
        cmd[0] = c;                                      //store user interrupt character at index = 0 of cmd
        characterTypedFlag = true;                       //set flag for character typed
        putsUart0("\r\n");                               //return to next line on terminal to distinguish rest of typed command from interrupt character
        putsUart0("Enter Command\r\n");
        putsUart0("-------------\r\n");
        putcUart0(c);                                    //display initially typed interrupt character
        getsUart0(cmd, MAX_CHARS);                       //get new command and display rest of command string
        putsUart0("\r\n");
        getcUart0();
        periodicMode = true;                             //turn on period mode to continue printing
      }
      else
      {
          putsUart0("\r\n");
          putsUart0("Enter Command\r\n");
          putsUart0("-------------\r\n");
          getsUart0(cmd, MAX_CHARS);
          putsUart0("\r\n");
          getcUart0();
       }
}
//prints contents of main menu
void printMenu()
{
    putsUart0("###########################################################\r\n");
    putsUart0("                             MENU                          \r\n");
    putsUart0("###########################################################\r\n");
    putsUart0("\r\n");
    putsUart0("   Calibration Functions                   CLI Commands\r\n");
    putsUart0("-----------------------------------------------------------\r\n");
    putsUart0("    1) Calibrate            calibrate T (T = 0... 4095)\r\n");
    putsUart0("    2) Color N                    color N (N = 0... 15)\r\n");
    putsUart0("    3) erase N                    erase N (N = 0... 15)\r\n");
    putsUart0("\r\n");
    putsUart0("   Sampling Functions                      CLI Commands\r\n");
    putsUart0("-----------------------------------------------------------\r\n");
    putsUart0("    4) periodic T             periodic T (T = 1... 255)\r\n");
    putsUart0("    5) delta D                   delta D (D = 0... 255)\r\n");
    putsUart0("    6) match E                   match E (E = 0... 255)\r\n");
    putsUart0("    7) trigger                                  trigger\r\n");
    putsUart0("    8) button                                    button\r\n");
    putsUart0("\r\n");
    putsUart0("   User Interface Functions                CLI Commands\r\n");
    putsUart0("----------------------------------------------------------\r\n");
    putsUart0("    9) led off                                  led off\r\n");
    putsUart0("   10) led on                                    led on\r\n");
    putsUart0("   11) led sample                            led sample\r\n");
    putsUart0("   12) test                          test P (P = Print)\r\n");
    putsUart0("                                  (P is optional input)\r\n");
    putsUart0("\r\n");
    putsUart0("   Miscellaneous Functions                 CLI Commands\r\n");
    putsUart0("----------------------------------------------------------\r\n");
    putsUart0("   13) Re-print Menu                               menu\r\n");
    putsUart0("   14) Exit Program                                exit\r\n");
    putsUart0("   15) Set LEDS                                     set\r\n");
    putsUart0("\r");
}
