/*
 * Lab4.c
 *
 * Created: 11/26/2018 2:10:05 PM
 * Authors: Tharun Suresh and Oliver Jin
 */ 

#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>

#include "defines.h"
#include "uart.h"

#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

/* state definitions */
#define IDLE 0
#define LEADING_Pulse 1
#define LEADING_Space 2
#define Addr_Pulse 3
#define Addr_Space 4
#define Addr_Inv_Pulse 5
#define Addr_Inv_Space 6
#define Command_Pulse 7
#define Command_Space 8
#define Command_Inv_Pulse 9
#define Command_Inv_Space 10
#define FINAL_Pulse 11
#define REPEAT_Pulse 12

/*state values*/
#define LEADING_Pulse_Duration ((4000 <= clock) && (9500 >= clock))
#define LEADING_Space_Duration ((4000 <= clock) && (5000 >= clock))
#define Logic1_Pulse_Duration ((400 <= clock) && (800 >= clock))
#define Logic0_Pulse_Duration ((400 <= clock) && (800 >= clock))
#define Logic1_Space_Duration ((1400 <= clock) && (1800 >= clock))
#define Logic0_Space_Duration ((400 <= clock) && (800 >= clock))
#define Final_Pulse_Duration  ((400 <= clock) && (800 >= clock))
#define Repeat_Pulse_Duration ((1800 <= clock) && (2600 >= clock))
#define Repeat_Space_Duration ((2100 <= clock) && (2400 >= clock))
#define distortion ((500 <= clock) && (14000 >= clock))

/*initialize variables*/
volatile int clock = 0;
volatile int state = 0;
volatile uint8_t buffer[4];
volatile uint8_t pointer = 7;
volatile int data;
volatile int address;

#define IR_dataPin (1 << 6)

void init(){
	MCUCR &= ~(1<<PUD); /*pullup enable for input*/
	DDRB &= ~IR_dataPin; /*pin input*/
	PORTB |= IR_dataPin; 
	
	/*interrupt enable*/
	sei();
	PCICR |= (1<<PCIE0);
	PCMSK0 |= (1<<PCINT6); 

	/*clock normal mode*/
	TCCR1A = 0x00;
	TCCR1B = 0x01;
	TCCR1C = 0x00;
}

void invalid(){
	cli(); /*disable interrupts prior to message*/
	//_delay_ms(100);
	fprintf(stdout, "INVALID state:%u clock:%u\n",state,clock); /*invalid message*/
	state = 0; /*reset state machine*/
	sei(); /*enable interrupts again*/
}

int main(void){
	init();
	uart_init();
	stdout = stdin = &uart_str;
	
	while (1){	
	} 
	return 0;
}

ISR(PCINT0_vect, ISR_BLOCK){
	clock = TCNT1; /*timer set to variable*/
	TCNT1 = 0; /*reset timer*/
	if (clock > 10000)
	{
	state = IDLE;
	}

	if (clock>250)
	{
	/*state machine*/
	switch(state){

		case IDLE: /*state 0*/
			state++; /*increment state*/
			break;

		case LEADING_Pulse:	/*state 1*/
			if (LEADING_Pulse_Duration){
				state++;	/*if leading pulse check for leading space or repeat space*/
			} 
			else{
				invalid();	/*print invalid message*/
			}
			break;

		case LEADING_Space: /*state 2*/
			if (LEADING_Space_Duration){
				state++;		/*if leading space check for address pulse*/
			}
			else if(Repeat_Space_Duration){
				state = FINAL_Pulse;	/*if repeat space go to state 11*/
			}
			else{
				invalid();
			}
			break;
		
		case Addr_Pulse: /*state 3*/
			if (Logic1_Pulse_Duration){ /* if logic 1 pulse, increment state*/
				state++;
			}
			else{
				invalid();
			}

		case Addr_Space: /*state 4*/
			if(Logic1_Space_Duration){
				if(pointer == 0){	/* if the buffer is full*/
				
					pointer = 7; /*reset pointer*/
					state++;
				}
				else{
					buffer[0] |= (1<<pointer); /*put 1 in buffer*/
					pointer--; /*decrement state and pointer*/
					state--;
				}
			}
			else if(Logic0_Space_Duration){ /*if 0*/
				if(pointer == 0){ /*if buffer is full*/
					pointer = 7; /*reset pointer*/
					state++;
				}
				else{
					buffer[0] &= ~(1<<pointer); /*buffer=0*/
					pointer--; /*decrement state and pointer*/
					state--;
				}
			}
			else{
				invalid();
			}
			break;
		
		case Addr_Inv_Pulse: /*state 5*/
			if(Logic1_Pulse_Duration){ /* if logic 1 pulse measured increment state*/
				state++;
			}
			else{
				invalid();
			}
			break;

		case Addr_Inv_Space: /*state 6*/
			if(Logic1_Space_Duration){ /* if logic 1 space*/
				if(pointer == 0){ /* if buffer is full*/
					pointer = 7; /*reset pointer and increment state*/
					state++;
				}
				else{
					buffer[1] |= (1<<pointer); /*put 1 in buffer*/
					pointer--; /*decrement state and pointer*/
					state--;
				}
			}
			else if(Logic0_Space_Duration){ /*if it's a 0*/
				if(pointer == 0){ /*if buffer full, reset pointer, go to next state*/
					pointer = 7;
					state++;
				}
				else{
					buffer[1] &= ~(1<<pointer); // puts 0 in buffer
					pointer--; /*decrement state and pointer*/
					state--;
				}
			}
			else{
				invalid();
			}
			break;

		case Command_Pulse: /*state 7*/
			if(Logic1_Pulse_Duration){ /*if pulse, increment state*/
				state++;
			}
			else{
				invalid();
			}
			break;


		case Command_Space: /*state 8*/
			if(Logic1_Space_Duration){ /*if space == logic 1*/
				if(pointer == 0){ /*if buffer full, reset pointer, go to next state*/
					pointer = 7;
					state++;
				}
				else{
					buffer[2] |= (1<<pointer); /*put 1 in buffer*/
					pointer--; /*decrement state and pointer*/
					state--;
				}
			}
			else if(Logic0_Space_Duration){ /*if space == logic 0*/
				if(pointer == 0){ /*if buffer full, reset pointer, go to next state*/
					pointer = 7;
					state++;
				}
				else{
					buffer[2] &= ~(1<<pointer); /*put 0 in buffer*/
					pointer--; /*decrement state and pointer*/
					state--;
				}
			}
			else{
				invalid();
			}
			break;

		case Command_Inv_Pulse: /*state 9*/
			if(Logic1_Pulse_Duration){
				state++;
			}
			else
			{
				invalid();
			}
			break;

		case Command_Inv_Space: /*state 10*/
			if(Logic1_Space_Duration){
				if(pointer == 0){
					pointer = 7;
					state++;
				}
				else
				{
					buffer[3] |= (1<<pointer);
					pointer--;
					state--;
				}
			}
			else if(Logic0_Space_Duration){
				if(pointer == 0){
					pointer = 7;
					state++;
				}
				else{
					buffer[3] &= ~(1<<pointer);
					pointer--;
					state--;
				}
			}
			else{
				invalid();
			}
			break;

		case FINAL_Pulse: /*state 11*/
			if(Final_Pulse_Duration || Repeat_Space_Duration){
				if(buffer[0] == ~buffer[1] && buffer[2] == ~buffer[3]){  /*if the data and address inverses are valid*/
					cli(); /*everything's good so printing time. disable interrupts*/
					_delay_ms(100);

					address = buffer[0]; /*set address and data from buffer*/
					data = buffer[2];
				
					fprintf(stdout, "Address: %x, Data: %x\n", address, data); /*print address and data*/
				
					sei(); /*enable interrupts again*/
					state = 0;
				}
			}
			if(distortion){ /*if distorted, reset state machine*/
				state = 0;
			}
			else{
				invalid();
			}
			break;

		

	}
}
}
	