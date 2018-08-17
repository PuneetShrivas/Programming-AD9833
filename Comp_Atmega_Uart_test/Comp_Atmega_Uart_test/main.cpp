/*
 * Comp_Atmega_Uart_test.cpp
 *
 * Created: 25-06-2018 21:01:06
 * Author : Puneet Shrivas
 */ 
#define F_CPU 14745600
#include <avr/io.h>
#include <util/delay.h>

#define BAUDRATE 115200			  //Baud rate for UART
#define BAUD_PRESCALLER (((F_CPU/(BAUDRATE * 16UL))) - 1) //Predefined formula from data sheet
void UART_init(void)
{
	UBRRH = (unsigned char)(BAUD_PRESCALLER>>8);						//UART initialization from data-sheet
	UBRRL = (unsigned char)BAUD_PRESCALLER;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);							// Set frame format: 8data, 2stop bit
}

void UART_send(unsigned char data)
{
	while(!(UCSRA & (1<<UDRE)));										//wait for buffer to be emptied
	UDR=data;
}

void UART_write16(unsigned short data)
{
	unsigned char MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	unsigned char LSdata = (data & 0x00FF);			//filter out LS
	UART_send(MSdata);
	UART_send(LSdata);
}
unsigned char UART_Receive( void )
{
	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)) )
	;
	/* Get and return received data from buffer */
	return UDR;
}

int main(void)
{	int datacount=0;
	DDRA = (1<<PINA0)|(1<<PINA1)|(1<<PINA2);
	UART_init();
	UART_send('a');
	//UART_write16(0x00
	
}

