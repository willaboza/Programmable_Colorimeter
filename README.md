# Programmable Colorimeter

## Overview:

Project goal is to design a device that can measure, learn, and recognize a given color in the visible portion of the light spectrum.

The intensity of reflected light from a given target color is used to measure its color when the RGB LEDs are on.

## Hardware:

Two 10x2 headers are used to connect a Texas Instruments TM4C123GH6PM microcontroller to a PC board. An RGB LED, used for illumination, and a TEPT5600 ambient light sensor were then soldered to the PC board. After soldering the RGB LED and TEPT5600 sensor to the PC board they are then wired to pins on the microcontroller. 

## Software:

Teraterm is used as a virtual COM port to interface with the microcontroller, over UART0, allowing for transmition/reception of information between the user and device.

## Project Steps:

1. Write code for getsUart0 function.

2. Write function to tokenize string.

3. Create the getValue function.

4. Code function for getString.

5. Create function for isCommand.

6. Illuminate r, g, and b LEDs from PWM 0-->1023 for test purposes.

7. Calibrate r, g, and b LEDs.

8. Trigger Command.

9. Support for periodic T command.

10. LED ON|OFF|Sample command

11. color N command

12. erase N command

13. match E command

14. delta D command
