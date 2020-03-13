/*
 * Project Code
	Displays String "User Temperature Setting: " + userTemp
	Uses two buttons to increase or decrease userTemp by 1
	
 */ 
#ifndef F_CPU
#define F_CPU 8000000UL //define microcontroller clock speed
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "lcd_io.c"
#include "lcd_io.h"
#include "DHT.c"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "DHT.h"

//global variables
volatile unsigned char TimerFlag = 0;
unsigned char user1 = 32;	
unsigned char user2 = 0x07 + '0';	
unsigned char user3 = 0x05 + '0';
unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;
int8_t temperature_int = 0;
int8_t humidity_int = 0;
unsigned char sensor1; 
unsigned char sensor2; 
unsigned char sensor3;

enum userTempStates{SM_Init, SM_select} userTempState;
enum fanStates{SM_off, SM_on} fanState;

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}
void TimerOff() {
	TCCR1B = 0x00;
}
void TimerISR() {
	TimerFlag = 1;
}
ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--;
	if(_avr_timer_cntcurr == 0) {
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
void WaterChar() { // print raindrop
	LCD_WriteCommand(64);
	LCD_WriteData(0);
	LCD_WriteData(4);
	LCD_WriteData(10);
	LCD_WriteData(17);
	LCD_WriteData(25);
	LCD_WriteData(21);
	LCD_WriteData(14);
	LCD_WriteData(0);
	LCD_WriteCommand(0xB8); // row on the LCD
	LCD_WriteData(5); // column of LCD
	delay_ms(10);
}
void SunChar() { //print sun
	LCD_WriteCommand(104);
	LCD_WriteData(4);
	LCD_WriteData(21);
	LCD_WriteData(14);
	LCD_WriteData(14);
	LCD_WriteData(14);
	LCD_WriteData(14);
	LCD_WriteData(21);
	LCD_WriteData(4);
	LCD_WriteCommand(0xB8);
	LCD_WriteData(5);
	delay_ms(10);
}
void LCD_print(uint16_t temp, unsigned char cur_count) {
	sensor1 = 48;
	sensor2 = 48;
	sensor3 = 48;
	uint16_t tempF = (temp * (9.0 / 5)) + 32;
	
	LCD_Cursor(cur_count);
	if(tempF >= 100 ) {
		sensor1 = 49; 
		temp -= 100;
	} else {
		sensor1 = ' ';
	}
	
	LCD_WriteData(sensor1);
	cur_count++;

	LCD_Cursor(cur_count);
	while(tempF >= 10 ) {
		sensor2++;
		tempF -= 10;
	}
	
	LCD_WriteData(sensor2);
	cur_count++;	
	sensor3 += tempF;  
	
	LCD_Cursor(cur_count);
	LCD_WriteData(sensor3);
	LCD_Cursor(0);
	
}
void TickFanState() {

	switch(fanState) {
		case SM_off :
		if(user1 == sensor1 && user2 == sensor2 && user3 <= sensor3) { 
			fanState = SM_on;
			} else if(user1 == sensor1 && user2 <= sensor2) {
			fanState = SM_on;
			} else if(user1 == ' ' && sensor1 == (0x01 + '0')) { 
			fanState = SM_on;
			} else {
			fanState = SM_off;
		}
		break;
		
		case SM_on :
		if(user1 == sensor1 && user2 == sensor2 && sensor3 < user3) {
			fanState = SM_off;
			} else if(user1 == sensor1 && sensor2 < user2) { 
			fanState = SM_off;
			} else if(user1 == (0x01 + '0') && sensor1 == ' ') {
			fanState = SM_off;
			} else {
			fanState = SM_on;
			}
		break;
	}
		
		switch(fanState) {
			case SM_off :
				PORTD &= ~(1 << PIND5);
				WaterChar();
			break;
			
			case SM_on :
				PORTD |= (1 << PIND5);
				SunChar();
			break;
	}
			LCD_DisplayString(1, "User Temp:");
			LCD_WriteData(user1);
			LCD_WriteData(user2);
			LCD_WriteData(user3);
			if(fanState == SM_on) {
				SunChar();
			}

			

}


void TickTempState() {
	switch(userTempState) {
		case SM_Init :
			if((~PINA & 0x04) == 0x04) {
				userTempState = SM_select;
			} else {
				userTempState = SM_Init;
			}
			break;
		case SM_select :
			if((~PINA & 0x01) == 0x01) {
				userTempState = SM_select;
				
				if(user1 == (0x01 + '0') && user2 == (0x02 + '0') && user3 == (0x05 + '0')) { // wont exceed 125
					user1 = (0x01 + '0'); user2 = (0x02 + '0'); user3 = (0x05 + '0');
					
				} else if(user2 == (0x09 + '0') && user3 == (0x09 + '0')) {// go from 99 to 100
					user1 = (0x01 + '0'); user2 = (0x00 + '0'); user3 = (0x00 + '0');
					
				} else if(user1 == ' ' && user2 <= (0x09 + '0') && user3 >= (0x09 + '0')) { // go from 80 to 90 
					user1 = ' '; user2++; user3 = (0x00 + '0');
					
				} else if(user1 == (0x01 + '0') && user2 <= (0x09 + '0') && user3 >= (0x09 + '0')) {
					user1 = (0x01 + '0'); user2++; user3 = (0x00 + '0');
					
				} else {
					user3++; // regular increment
				}
				
			} else if((~PINA & 0x02) == 0x02) {
				userTempState = SM_select;
				
				if(user1 == ' ' && user2 == (0x06 + '0') && user3 == (0x00 + '0')) { // wont go below 75
					user1 = ' '; user2 = (0x06 + '0'); user3 = (0x00 + '0');
					
				} else if(user1 == (0x01 + '0') && user2 == (0x00 +'0') && user3 == (0x00 +'0')) {//go from 100 to 99
					user1 = ' '; user2 = (0x09 + '0'); user3 = (0x09 + '0');
					
				} else if(user3 == (0x00 + '0')) {  // go from 90 to 89 for example
					user2--; user3 = (0x09 + '0');
					
				} else {
					user3--; // regular decrement
				}
				
			} else if((~PINA & 0x04) == 0x04) {
				userTempState = SM_Init;
			}
			break;
		default : 
			userTempState = SM_Init;
			break;
	}	
}

int main(void)
{
	DDRA = 0xF0; PORTA = 0x0F; 
	DDRC = 0xFF; PORTC = 0x00; 
	DDRD = 0xFF; PORTD = 0x00;

	LCD_init();
	LCD_ClearScreen();
	
	_delay_ms(1000);
	userTempState = SM_Init;
	fanState = SM_off;
	

	TimerSet(100);
	TimerOn();
	while (1)
	{
			if (dht_GetTempUtil(&temperature_int, &humidity_int) != -1) {
				LCD_print(temperature_int, 27);
			}
			
			else {LCD_DisplayString (1, "error");}

			_delay_ms(1500);
		
		TickTempState();
		TickFanState();
		while(!TimerFlag) {}
		TimerFlag = 0;
	}
	return 0;
}

