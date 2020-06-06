# Programmable Colorimeter

Project goal is to design a device that can measure, learn, and recognize a given color in the visible portion of the light spectrum.

The intensity of reflected light from a given target color is used to measure its color when the RGB LEDs are on.

## Hardware:

Two 10x2 headers are used to connect a Texas Instruments TM4C123GH6PM microcontroller to a PC board. An RGB LED, used for illumination, and a TEPT5600 ambient light sensor were then soldered to the PC board. After soldering the RGB LED and TEPT5600 sensor to the PC board they are then wired to pins on the microcontroller. 

Pin connections for different components connected to the microcontroller are shown in the table below:

| Component | Pin |
| :----: | :----: |
| UART0 RX | PA0 |
| UART0 TX | PA1 |
| Green Status LED | PF3 |
| Push Button | PF4 |
| Red LED (external) | PB5 |
| Blue LED (external) | PE4 |
| Green LED (external) | PE5 |
| TEPT5600 | PE3 |


## Software:

Teraterm is used as a virtual COM port to interface with the microcontroller, over UART0, allowing for transmition/reception of information between the user and device.

## Project Steps:

1. Write code for getsUart0 function.

2. Write function to tokenize string.

3. Create the getValue function.

4. Code function for getString.

5. Create function for isCommand.

   Function isCommand returns a boolean by parsing each string entered 

6. Illuminate r, g, and b LEDs from PWM 0-->1023 for test purposes.

   Drive up RGB LED from duty cycle of 0 to 1023 on each r, g, and b LED seperately 

7. Calibrate r, g, and b LEDs.

   Instructs the hardware to calibrate the white color balance and displays the duty cycle information when complete.
   
   Ramp up red LED, then green LED, and finally blue LED from 0 to 1023 PWM values. Plot measured values Find the point at which each of the RGB values crosses a user defined threshold.

8. Trigger Command.

   Configures the hardware to send an RGB triplet immediately.
   
   * Red LED set to PWM<sub>R</sub>
   * Measure light or R<sub>value</sub>
   * Green LED set to PWM<sub>G</sub>
   * Measure light or G<sub>value</sub>
   * Blue LED set to PWM<sub>B</sub>
   * Measure light or B<sub>value</sub>
   
   Then display RGB triplet values on terminal. Display an "error" message if device is not calibrated yet.

9. Support for periodic T command

   Configures the hardware to send an RGB triplet in 8-bit calibrated format every 0.1 x T seconds, where T = 0..255 or off. Want a resolution in units of 0.1 seconds and uses Timer1Isr function to achieve this. For example:
   
   * periodic 1 --> 100 milliseconds
   * periodic 100 --> 10 seconds
   
   Timer1Isr performs the following steps:
   
   * Read Wide Timer 5 (WT5) value
   * Store value in a global variable
   * Clear WT5 value
   * Clear interrupt flag for WT5

10. LED ON|OFF|Sample commands

    Manipulates green status LED by:
    
    * ON - Disabling the green status LED
    * OFF - Enabling the green status LED
    * Sample - Blinks the green status LED for each sample taken

11. color N command

    Stores the current color as color reference N (N = 0..15).

12. erase N command

    Erases color reference N (N = 0..15).

13. match E command

    Configures the hardware to send an RGB triplet when the Euclidean distance (error) between a sample and one of the color reference (R,G,B) is less than E, where E = 0..255 or off.

14. delta D command

    Configures the hardware to send an RGB triplet when the RMS average of the RGB triplet vs the long-term average (IIR filtered, alpha = 0.9) changes by more than D, where D = 0..255 or off.
