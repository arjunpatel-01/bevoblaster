// ADC.c
// Runs on LM4F120/TM4C123
// Provide functions that initialize ADC0
// Last Modified: 1/17/2020
// Student names: Zahid Hossain, Arjun Patel

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"

// ADC initialization function 
// Input: none
// Output: none
// measures from PD2, analog channel 5
void ADC_Init(void){ 
  SYSCTL_RCGCADC_R |= 0x01; //activate ADC0
  SYSCTL_RCGCGPIO_R |= 0x08; //activate PortD
  for(volatile uint8_t i=0;i<5;i++){uint16_t delay = SYSCTL_RCGCADC_R;} //Delay for clock stabilization
  GPIO_PORTD_DIR_R &= ~(0x04); //PD2 input (Channel 5 analog input)
  GPIO_PORTD_AFSEL_R |= 0x04; //enable Alt Function on PD2
  GPIO_PORTD_DEN_R &= ~(0x04); //disable digital on PD2
  GPIO_PORTD_AMSEL_R |= 0x04; //enable analog on PD2
  ADC0_PC_R = 0x01; //Conversion speed = 125KHz
  ADC0_SSPRI_R = 0x0123; //Sequencer 3 is highest priority (0x0)
  ADC0_ACTSS_R &= ~(0x08); //disable seq 3
  ADC0_EMUX_R &= ~(0xF000); //seq 3 use software start trigger
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R & ~(0x0F)) + 5; //Set channel to Ain5 (PD2)
  ADC0_SSCTL3_R = 0x06; //no TS0 D0, yes IE0 (enable raw interrupt flag) END0 (only perform single sample conversion)
  ADC0_IM_R &= ~(0x08); //disable seq 3 interrupts
  ADC0_SAC_R = 4;   // 32-point averaging
  ADC0_ACTSS_R |= 0x08; //enable seq 3
}

//------------ADC_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion
// measures from PD2, analog channel 5
uint32_t ADC_In(void){  
  uint32_t data;
  ADC0_PSSI_R = 0x08; //start ADC seq 3
  while((ADC0_RIS_R & 0x08)==0){}; //check raw interrupt status for seq 3
  data = ADC0_SSFIFO3_R & 0xFFF; //pull 12-bit sample result from seq 3 FIF0
  ADC0_ISC_R = 0x08; //Clear flag for seq 3, writing 1 clears ISC and RIS flags
  return data; // remove this, replace with real code
}


