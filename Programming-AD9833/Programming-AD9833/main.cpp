/*
 * Programming-AD9833.cpp
 *
 * Created: 19-03-2018 20:55:19
 * Author : Puneet Shrivas
 */ 

#define F_CPU 14745600
#define Fmclk 25000000
#define SINE 0x2100               //mode 0
#define SQUARE 0x2128			  //mode 1
#define TRIANGLE 0x2102			  //mode 2
#include <avr/io.h>
#include <avr/delay.h>
#include <math.h>
#define BAUDRATE 9600 //Baud rate for UART
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1) //Predefined formula from datasheet

void SPI_init(void)
{
	DDRB=(1<<PINB7)|(1<<PINB5)|(1<<PINB0)|(1<PINB4);         //sets SCK, MOSI,SS and PINB0 as output (Fsync at Pinb0)
	DDRA=(1<<PINA0)|(1<<PINA1)|(1<<PINA2);
	PORTA=0;
	PORTB=(1<<PINB0)|(1<<PINB4);					//Fsync High, SS is set high
	SPCR=(1<<SPE)|(1<<MSTR)|(1<<CPOL)/*|(1<<SPR1)|(1<<SPR0)*/|(1<<SPIE);				//Enable SPI, set master, prescaler = 4, SPI Mode:2
}

void UART_init(void)
{
	UBRRH = (uint8_t)(BAUD_PRESCALLER>>8);
	UBRRL = (uint8_t)BAUD_PRESCALLER;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0); // Set frame format: 8data, 2stop bit
}

void UART_send(uint8_t data)
	 {
		 while(!(UCSRA & (1<<UDRE)));
		 UDR=data;
	 }

void led(int i)
{
	switch (i)
	{
		case 0 : PORTA|=(1<<PINA0);
		_delay_ms(500);
		PORTA = 0; break;
		case 1 : PORTA|=(1<<PINA1);
		_delay_ms(500);
		PORTA = 0; break;
		case 2 : PORTA|=(1<<PINA2);
		_delay_ms(500);
		PORTA = 0; break;
	}
}
void SPI_transfer(uint8_t data)
{
	//led(0);
	PORTB|=(1<<PINB4);
	UART_send(data);
	SPDR=data;
	while(!(SPSR&(1<<SPIF))) {;/*wait for data transfer and recieving*/}
	//led(1);
}
void SPI_write16 (unsigned short data)    	// 	send a 16bit word and use fsync
{  
	unsigned char MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	unsigned char LSdata = (data & 0x00FF);			//filter out LS
	//UART_send(MSdata);
	//UART_send(LSdata);
	PORTB &= ~(1<<PINB0);						// 	Fsync Low --> begin frame
	SPI_transfer(MSdata);
	SPI_transfer(LSdata);
	PORTB |= (1<<PINB0);						// 	Fsync High --> End of frame
}

void Set_AD9833(float frequency)
{
	long FreqReg = (frequency*pow(2,28))/Fmclk;  //Calculate frequency to be sent to AD9833
	int MSB = (int)((FreqReg &  0xFFFC000) >> 14);		  //Extract first 14 bits of FreqReg and place them at last 14 bits of MSB
	int LSB = (int)((FreqReg & 0x3FFF));				  //Extract last 14 bits of FreqReg and place them at last 14 bits of MSB	
	MSB|=0x4000;										  //Set D14,D15 = (1,0) for using FREQ0 registers, MSB has all 16 bits set
	LSB|=0x4000; /*led(1);*/										  //Set D14,D15 = (1,0) for using FREQ0 registers, LSB has all 16 bits set
	SPI_write16(0x2100);/*led(0);*/
	SPI_write16(MSB);/*led(1);*/									  //Write MSBs
	SPI_write16(LSB);/*led(2);*/									  //Write LSBs
	SPI_write16(0xC000);								  //Mode selection for writing to phase register bit, selection of PHASE0 register (Needs to be fixed)
	SPI_write16(0x2000);
}

int main(void)
{
	UART_init();
	SPI_init();
	SPI_write16(0x100);

    while (1) 
    {
		Set_AD9833(1000);
		_delay_ms(1000);
		Set_AD9833(2000);
		_delay_ms(1000);
		Set_AD9833(3000);
		_delay_ms(1000);				
    }
}

