// BevoBlaster.c
// Runs on LM4F120/TM4C123
// Zahid Hossain and Arjun Patel

/* This example accompanies the books
	"Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
	ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2019

	"Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
	ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2019

	Copyright 2019 by Jonathan W. Valvano, valvano@mail.utexas.edu
	You may use, edit, run or distribute this file
	as long as the above copyright notice remains
	THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
	OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
	VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
	OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
	For more information about my classes, my research, and my books, see
	http://users.ece.utexas.edu/~valvano/
 */
/* ******* Hardware I/O connections ********
	PD2 = Slide Pot
	PE4 = Left Button
	PE5 = Right Button
	PF1-3 = LED Heartbeat (RBG)

	Backlight (pin 10) connected to +3.3 V
	MISO (pin 9) unconnected
	SCK (pin 8) connected to PA2 (SSI0Clk)
	MOSI (pin 7) connected to PA5 (SSI0Tx)
	TFT_CS (pin 6) connected to PA3 (SSI0Fss)
	CARD_CS (pin 5) unconnected
	Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
	RESET (pin 3) connected to PA7 (GPIO)
	VCC (pin 2) connected to +3.3 V
	Gnd (pin 1) connected to ground
*/

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "Print.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"

void DisableInterrupts(void);
void EnableInterrupts(void);
void Heartbeat_Init(void);
void Buttons_Init(void);
void SysTick_Init(void);
void Delay10ms(uint8_t count);
uint32_t Convert(uint32_t input);

struct player{
	int16_t y;
	const unsigned short *sprite;
	const unsigned short *empty;
	int16_t w;
	int16_t h;
}; typedef struct player player_t;

static player_t bevo_player;

struct bullet{
	int16_t x;
	int16_t y;
	const unsigned short *sprite;
	const unsigned short *empty;
	int16_t w;
	int16_t h;
	uint8_t needsDraw;
}; typedef struct bullet bullet_t;

static bullet_t horns;
static bullet_t charge; //McConaughey

//Fade screen from black to desired color
void screenGradient(uint16_t maxColor){
	uint16_t B,G,R,Bmax,Gmax,Rmax;
	Bmax = (maxColor & 0xF800) >> 11;
	Gmax = (maxColor & 0x07E0) >> 5;
	Rmax = (maxColor & 0x001F);
	uint8_t levels = 20;
	for(uint8_t i=0;i<=levels;i++){
		B = Bmax * i / levels;
		G = Gmax * i / levels;
		R = Rmax * i / levels;
		uint16_t color = (B<<11)|(G<<5)|(R);
		ST7735_FillScreen(color);
		Delay10ms(1);
	}
	ST7735_FillScreen(maxColor);
}

//Burnt Orange > BGR = 0-87-191 = 0-21-24 = 00000 010101 11000 = 0x02B8
#define BurntO 0x02B8

typedef enum {English,Spanish} language_t;
language_t gameLang;
uint8_t button;
uint8_t inGame = 0; //1 if playing, 0 if not
void startScreen(void){
	screenGradient(BurntO);
	
	uint8_t baseX = 16;
	uint8_t baseY = 50;
	uint8_t size = 4;
	uint8_t offset = size * 6;
	ST7735_DrawChar(baseX,baseY,'B',0xFFFF,BurntO,size);
	Delay10ms(3);
	ST7735_DrawChar(baseX + offset,baseY,'E',0xFFFF,BurntO,size);
	Delay10ms(3);
	ST7735_DrawChar(baseX + 2*offset,baseY,'V',0xFFFF,BurntO,size);
	Delay10ms(3);
	ST7735_DrawChar(baseX + 3*offset,baseY,'O',0xFFFF,BurntO,size);
	Delay10ms(3);
	ST7735_DrawString(7,9,"BLASTER",0xFFFF);
	Delay10ms(10);

	ST7735_DrawString(3,12,"Select Language",0xFFF0);
	ST7735_DrawString(2,15,"English",0x0FFF);
	ST7735_DrawString(13,15,"Spanish",0x0FFF);
	Delay10ms(2);
	ST7735_FillRect(25,133,15,15,0xFFFF);
	ST7735_FillRect(90,133,15,15,0xFFFF);

	button = 0;
	while(button == 0){};
	if(button == 1){gameLang = English;}
	if(button == 2){gameLang = Spanish;}
	button = 0;
	ST7735_FillScreen(BurntO);
	if(gameLang == English){ST7735_DrawString(2,15,"English",0x0FFF);}
	if(gameLang == Spanish){ST7735_DrawString(13,15,"Spanish",0x0FFF);}
	Delay10ms(10);
}


int8_t enemyHealth[3] = {3,4,6}; //mike, sammy, reveille
int16_t enemyPos = 80;
int16_t prevEnemyPos;
uint16_t enemySpeed;
uint8_t currentEnemy; //0 = mike, 1 = sammy, 2 = reveille
uint8_t chargeReady; //0 if not ready, 1 if ready
int main(void){

	DisableInterrupts();
	PLL_Init(Bus80MHz);
	Heartbeat_Init();
	SysTick_Init();
	ADC_Init();
	Sound_Init();
	Buttons_Init();
	Random_Init(1);
	ST7735_InitR(INITR_REDTAB);
	ST7735_FillScreen(0);
	EnableInterrupts();

	startScreen();

	bevo_player.y=0;
	bevo_player.w=30;
	bevo_player.h=30;
	bevo_player.sprite = bevo;
	bevo_player.empty = clear30;

	horns.needsDraw = 0;
	horns.sprite = hookem;
	horns.empty = clear10;
	horns.x = 0; //launch position is from x=30

	charge.needsDraw = 0;
	charge.sprite = mcconaughey;
	charge.empty = clear20;
	charge.x = 0;

	ST7735_FillScreen(0xFFFF);
	inGame = 1;
	chargeReady = 0;
	while(inGame){
		static const unsigned short *enemy;
		if(enemyHealth[0] > 0){
			enemy = mikeTheTiger; enemySpeed = 500; currentEnemy = 0;
		}else if(enemyHealth[1] > 0){
			enemy = sammyTheOwl; enemySpeed = 300; currentEnemy = 1;
		}else if(enemyHealth[2] > 0){
			enemy = reveille; enemySpeed = 100; currentEnemy = 2;
		}
		if(enemyHealth[2] <= 0){inGame = 0;}
		
		if(horns.needsDraw){
			ST7735_DrawBitmap(horns.x,horns.y,horns.sprite,10,10);
			ST7735_DrawBitmap(horns.x-10,horns.y,horns.empty,10,10);
		}
		if(charge.needsDraw){
			ST7735_DrawBitmap(charge.x,charge.y,charge.sprite,20,20);
			ST7735_DrawBitmap(charge.x-20,charge.y,charge.empty,20,20);
		}

		//Draw Enemy
		ST7735_DrawBitmap(97,prevEnemyPos,clear30,30,30);
		ST7735_DrawBitmap(97,enemyPos,enemy,30,30);
		//Draw Bevo
		ST7735_DrawBitmap(0, bevo_player.y, bevo_player.sprite, bevo_player.w, bevo_player.h);
		ST7735_DrawBitmap(0, bevo_player.y-30, bevo_player.empty, bevo_player.w, bevo_player.h);
		ST7735_DrawBitmap(0, bevo_player.y-60, bevo_player.empty, bevo_player.w, bevo_player.h);
		ST7735_DrawBitmap(0, bevo_player.y+30, bevo_player.empty, bevo_player.w, bevo_player.h);
		ST7735_DrawBitmap(0, bevo_player.y+60, bevo_player.empty, bevo_player.w, bevo_player.h);

		//Enemy Health #
		ST7735_SetCursor(10,0);
		if(enemyHealth[currentEnemy]>=0){LCD_OutDec(enemyHealth[currentEnemy]);}

		//Blaster Status Rectangles
		//Green = Ready to Fire, Red = Not Ready
		if(horns.needsDraw == 1){
			ST7735_FillRect(40,155,20,20,0x001F);
		} else {
			ST7735_FillRect(40,155,20,20,0x07E0);
		}
		if(charge.needsDraw == 0 && chargeReady == 1){
			ST7735_FillRect(68,155,20,20,0x07E0);
		} else {
			ST7735_FillRect(68,155,20,20,0x001F);
		}

	}

	ST7735_FillScreen(0);
	Delay10ms(3);
	Sound_Alright3X();
	ST7735_SetCursor(1,1);
	if(gameLang == English){
		ST7735_OutString("You Won!!!");
		Delay10ms(3);
		ST7735_SetCursor(1,4);
		ST7735_OutString("Press reset button");
		ST7735_SetCursor(1,5);
		ST7735_OutString("to play again");
	}else{
		ST7735_OutString("T\xA3 Ganaste!!!");
		Delay10ms(3);
		ST7735_SetCursor(1,4);
		ST7735_OutString("Presione el bot\xA2n");
		ST7735_SetCursor(1,5);
		ST7735_OutString("de reiniciar para");
		ST7735_SetCursor(1, 6);
		ST7735_OutString("volver a jugar");
	}
	while(1){};
}

void GPIOPortE_Handler(void){
	button = (GPIO_PORTE_RIS_R & 0x30) >> 4; //1 = left, 2 = right
	if(inGame == 1){
		if(button == 1 && horns.x == 0){
			Sound_HookEmHorns();
			horns.needsDraw = 1;
			horns.x = 30;
			horns.y = bevo_player.y-10;
			button = 0;
		}
		if(button == 2 && chargeReady && charge.x == 0){
			Sound_Alright3X();
			charge.needsDraw = 1;
			charge.x = 30;
			charge.y = bevo_player.y-10;
			button = 0;
			chargeReady = 0;
		}
	}
	GPIO_PORTE_ICR_R = 0x30; //acknowledge flags, last stepS
}


uint32_t Convert(uint32_t input){
	//ADC Input maps from 5 to 196
	return (47*input/1000 + 5);
}

void SysTick_Handler(void){
	static uint16_t tickCount;
	if(tickCount % 800 == 0){chargeReady = 1;}//charge ready every 8 seconds
	if(tickCount % enemySpeed == 0){
		prevEnemyPos = enemyPos;
		enemyPos = 29+((Random()%130)+1);//new random enemy position
	}
	tickCount ++;
	//Bevo map 5 -> 159, 196 -> 30
	bevo_player.y = (0-Convert(ADC_In())*675/1000)+162;
	if(horns.x != 0){horns.x += 1;}
	if((horns.y-10 >= enemyPos-30 && horns.y-10 <= enemyPos)||(horns.y >= enemyPos-30 && horns.y <= enemyPos)){
		if(horns.x >= 99){
			enemyHealth[currentEnemy] -= 1;
			ST7735_DrawBitmap(horns.x,horns.y,horns.empty,10,10); //to clean up fragments
			horns.x = 0;
			horns.needsDraw = 0;
		}
	} else if(horns.x >= 130){
		horns.x = 0;
		horns.needsDraw = 0;
	}
	if(charge.x != 0){charge.x += 1;}
	if((charge.y-20 >= enemyPos-30 && charge.y-20 <= enemyPos)||(charge.y >= enemyPos-30 && charge.y <= enemyPos)){
		if(charge.x >= 97){
			enemyHealth[currentEnemy] -= 3;
			ST7735_DrawBitmap(charge.x,charge.y,charge.empty,20,20); //to clean up fragments
			charge.x = 0;
			charge.needsDraw = 0;
		}
	} else if(charge.x >= 125){
		charge.x = 0;
		charge.needsDraw = 0;
	}
}

void Heartbeat_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x20;
	uint8_t volatile clockdelay = 100;
	GPIO_PORTF_DEN_R |= 0x0E;
	GPIO_PORTF_DIR_R |= 0x0E;
}

void Buttons_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x10;
	uint8_t volatile clockdelay = 100;
	GPIO_PORTE_DIR_R &= ~(0x30);
	GPIO_PORTE_DEN_R |= 0x30;
	GPIO_PORTE_PDR_R |= 0x30;
	GPIO_PORTE_IS_R &= ~(0x30); //Edge Sensitive
	GPIO_PORTE_IBE_R &= ~(0x30); //Don't interrupt on both rise & fall
	GPIO_PORTE_IEV_R &= ~(0x30); //Interrupt on falling edge
	GPIO_PORTE_ICR_R = 0x30; //clear flag 4 & 5
	GPIO_PORTE_IM_R |= 0x30; //arm interrupt on PE4-5
	NVIC_PRI1_R = (NVIC_PRI1_R & ~(0x7<<5)) | (0x5<<5); //priority = 5 (bits 7:5 controls interrupt 4)
	NVIC_EN0_R |= 0x1 << 4; //enable interrupt 4 in NVIC (PortE Handler)
}

#define refreshRate (80000000/100)
void SysTick_Init(void){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = refreshRate;
	NVIC_ST_CURRENT_R = 0;
	NVIC_PRI3_R = (NVIC_PRI3_R & 0x1FFFFFFF) | 0xE0000000; //Priority = 3 (bits 31:29)
	NVIC_EN0_R |= (0x1 << 15);
	NVIC_ST_CTRL_R = 0x07;
}

void Delay10ms(uint8_t count){
	SYSCTL_RCGCTIMER_R |= 0x01; //TIMER0 on
	TIMER0_CTL_R = 0x00000000; //Disable
	TIMER0_CFG_R = 0x00000000; //32-Bit mode
	TIMER0_TAMR_R = 0x00000002; //periodic count down
	TIMER0_TAPR_R = 0; //bus clock resolution
	TIMER0_ICR_R = 0x00000001; //clear flag
	TIMER0_IMR_R = 0x00000000; //disarm interrupts
	TIMER0_CTL_R = 0x00000001; //Enable
	for(uint8_t i = 0;i<count;i++){
		TIMER0_TAILR_R = 800000 - 1; //10ms delay
		while(TIMER0_TAR_R > 0){};
	}
}
