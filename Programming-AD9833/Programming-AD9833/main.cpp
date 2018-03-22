/*
 * Programming-AD9833.cpp
 *
 * Created: 19-03-2018 20:55:19
 * Author : Puneet Shrivas
 */ 

#define F_CPU 14745600
#define Fmclk 25000000
#define SINE 0x2100               //mode 0
#define SQUARE 0x2168			  //mode 1
#define TRIANGLE 0x2102			  //mode 2
#include <avr/io.h>
#include <avr/delay.h>
#include <math.h>

void SPI_init(void)
{
	DDRB=(1<<PINB7)|(1<<PINB5)|(1<<PINB0);         //sets SCK, MOSI and PINB0 as output (Fsync at Pinb0
	DDRA=(1<<PINA0)|(1<<PINA1)|(1<<PINA2);
	PORTA=0;
	PORTB|=(1<<PINB7)|(1<<PINB0);					//SCK and Fsync High
	SPCR=(1<<SPE)|(1<<MSTR)|(1<<CPOL)|(SPR1)|(SPR0)|(1<<SPIE);				//Enable SPI, set master, prescaler = 4, SPI Mode:2
}
void led(int i)
{
	switch (i)
	{
		case 0 : PORTA|=(1<<PINA0);
		_delay_ms(5000);
		PORTA = 0; break;
		case 1 : PORTA|=(1<<PINA1);
		_delay_ms(5000);
		PORTA = 0; break;
		case 2 : PORTA|=(1<<PINA2);
		_delay_ms(5000);
		PORTA = 0; break;
	}
}
void SPI_transfer(uint8_t data)
{
	led(0);
	
	SPDR=data;
	_delay_us(5);
	asm volatile("nop");
	while(!(SPSR&(1<<SPIF))) {;/*wait for data transfer and recieving*/}
	led(1);
}
void SPI_write16 (unsigned short data)    	// 	send a 16bit word and use fsync
{  led(1);
	unsigned char MSdata=0;
	unsigned char LSdata=0;
	MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	LSdata = (data & 0x00FF);			//filter out LS
	PORTB &= ~(1<<PB0);						// 	Fsync Low --> begin frame
	led(2);
	SPI_transfer(MSdata);
	SPI_transfer(LSdata);
// 	SPDR = MSdata;							// 	send First 8 MS of data
// 	while (!(SPSR & (1<<SPIF)));			//	while busy
// 	SPDR = LSdata;							// 	send Last 8 LS of data
// 	while (!(SPSR & (1<<SPIF)));			//	while busy
led(0);
	PORTB |= (1<<PB0);						// 	Fsync High --> End of frame
}

void Set_AD9833(float frequency, int mode)
{
	long FreqReg = (frequency*pow(2,28))/Fmclk;  //Calculate frequency to be sent to AD9833
	int MSB = (int)((FreqReg & 0xFFFC000) >> 14);		  //Extract first 14 bits of FreqReg and place them at last 14 bits of MSB
	int LSB = (int)((FreqReg & 0x3FFF));				  //Extract last 14 bits of FreqReg and place them at last 14 bits of MSB	
	MSB|=0x4000;										  //Set D14,D15 = (1,0) for using FREQ0 registers, MSB has all 16 bits set
	LSB|=0x4000; led(1);										  //Set D14,D15 = (1,0) for using FREQ0 registers, LSB has all 16 bits set
// 	switch(mode)									      //Reset AD9833 and choose mode 
// 	{
// 		case 0 : SPI_write16(0x2100); break;			  //Sine
// 		case 1 : SPI_write16(0x2128); break;			  //Square with same frequency
// 		case 2 : SPI_write16(0x2102); break;		      //Triangle
// 		default: return;
// 	}
	SPI_write16(0x2100);led(0);
	SPI_write16(MSB);led(1);									  //Write MSBs
	SPI_write16(LSB);led(2);									  //Write LSBs
	SPI_write16(0xC000);								  //Mode selection for writing to phase register bit, selection of PHASE0 register (Needs to be fixed)
// 	switch(mode)									      //unReset AD9833 and choose mode
// 	{
// 		case 0 : SPI_write16(0x2000); break;			  //Sine
// 		case 1 : SPI_write16(0x2028); break;			  //Square with same frequency
// 		case 2 : SPI_write16(0x2002); break;		      //Triangle
// 		default: return;
// 	}
	SPI_write16(0x2002);
}

int main(void)
{
	SPI_init();
//	SPI_transfer(1);
	SPI_write16(0x100);led(2);
	_delay_ms(500);
	SPI_write16(0x100);led(2);
	
	
    while (1) 
    {Set_AD9833(4000,0);	
// 	_delay_ms(1000);
// 	Set_AD9833(3000,0);	
// 	_delay_ms(1000);
    }
}

