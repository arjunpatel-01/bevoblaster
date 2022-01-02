// dac.c
// This software configures DAC output
// Runs on TM4C123
// Program written by: Zahid Hossain - Arjun Patel 
// Lab number: 6
// Hardware connections:
// PA2 = Key0, PA3 = Key1, PA4 = Key2
// PB0-PB5 = 6-Bit DAC
// PD3 = TExAS Display meter input

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
// Code files contain the actual implemenation for public functions
// this file also contains an private functions and private data

// **************DAC_Init*********************
// Initialize 6-bit DAC, called once 
// Input: none
// Output: none
void DAC_Init(void){
    SYSCTL_RCGCGPIO_R |= 0x02;
    volatile uint8_t clock_stabilize_delay = 100; //arbitrary delay for clock stabilization
    GPIO_PORTB_DEN_R |= 0x3F;
    GPIO_PORTB_DIR_R |= 0x3F; //set bits output
}

// **************DAC_Out*********************
// output to DAC
// Input: 6-bit data, 0 to 63
// Input=n is converted to n*3.3V/64
// Output: none
void DAC_Out(uint32_t data){
    GPIO_PORTB_DATA_R = data;
}
