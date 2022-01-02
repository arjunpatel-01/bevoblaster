// dac.h
// This software configures DAC output
// Runs on TM4C123
// Program written by: Zahid Hossain - Arjun Patel 
// Lab number: 6
// Hardware connections:
// PA2 = Key0, PA3 = Key1, PA4 = Key2
// PB0-PB5 = 6-Bit DAC
// PD3 = TExAS Display meter input

#ifndef DAC_H
#define DAC_H
#include <stdint.h>
// Header files contain the prototypes for public functions
// this file explains what the module does

// **************DAC_Init*********************
// Initialize 6-bit DAC, called once 
// Input: none
// Output: none
void DAC_Init(void);


// **************DAC_Out*********************
// output to DAC
// Input: 6-bit data, 0 to 63
// Input=n is converted to n*3.3V/64
// Output: none
void DAC_Out(uint32_t data);

#endif
