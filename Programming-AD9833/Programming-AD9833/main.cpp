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
	DDRB|=(1<<PINB7)|(1<<PINB5)|(1<<PINB4);         //sets SCK, MOSI and PINB0 as output (Fsync at SS)
	PORTB|=(1<<PINB7)|(1<<PINB4);					//SCK and Fsync High
	SPCR|=(1<<SPE)|(1<<MSTR)|(1<<CPOL);				//Enable SPI, set master, prescaler = 4, SPI Mode:2
}

void SPI_write16 (uint16_t data)    	// 	send a 16bit word and use fsync
{
	unsigned char MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	unsigned char LSdata = (data & 0x00FF);			//filter out LS

	PORTB &= ~(1<<PB4);						// 	Fsync Low --> begin frame
	
	SPDR = MSdata;							// 	send First 8 MS of data
	while (!(SPSR & (1<<SPIF)));			//	while busy

	SPDR = LSdata;							// 	send Last 8 LS of data
	while (!(SPSR & (1<<SPIF)));			//	while busy

	PORTB |= (1<<PB4);						// 	Fsync High --> End of frame
}

void Set_AD9833(float frequency, int mode)
{
	unsigned long FreqReg = (frequency*pow(2,28))/Fmclk;  //Calculate frequency to be sent to AD9833
	int MSB = (int)((FreqReg & 0xFFFC000) >> 14);		  //Extract first 14 bits of FreqReg and place them at last 14 bits of MSB
	int LSB = (int)((FreqReg & 0x3FFF));				  //Extract last 14 bits of FreqReg and place them at last 14 bits of MSB	
	MSB|=0x4000;										  //Set D14,D15 = (1,0) for using FREQ0 registers, MSB has all 16 bits set
	LSB|=0x4000;										  //Set D14,D15 = (1,0) for using FREQ0 registers, LSB has all 16 bits set
	switch(mode)									      //Reset AD9833 and choose mode 
	{
		case 0 : SPI_write16(0x2100); break;			  //Sine
		case 1 : SPI_write16(0x2168); break;			  //Square with same frequency
		case 2 : SPI_write16(0x2102); break;		      //Triangle
		default: return;
	}
	SPI_write16(MSB);									  //Write MSBs
	SPI_write16(LSB);									  //Write LSBs
	//SPI_write16(0xC000);								  //Mode selection for writing to phase register bit, selection of PHASE0 register (Needs to be fixed)
	switch(mode)									      //unReset AD9833 and choose mode
	{
		case 0 : SPI_write16(0x2000); break;			  //Sine
		case 1 : SPI_write16(0x2068); break;			  //Square with same frequency
		case 2 : SPI_write16(0x2002); break;		      //Triangle
		default: return;
	}
}

int main(void)
{
	SPI_init();
	Set_AD9833(1000,SINE);	
    while (1) 
    {
    }
}

