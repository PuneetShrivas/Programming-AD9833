/*
 * Programming-AD9833.cpp
 *
 * Created: 19-03-2018 20:55:19
 * Author : Puneet Shrivas
 */ 

#define F_CPU 14745600
#include <avr/io.h>
#include <avr/delay.h>

void SPI_init(void)
{
	DDRB|=(1<<PINB7)|(1<<PINB5)|(1<<PINB0); //sets SCK, MOSI and PINB0 as output (Fsync at PINB0)
	PORTB|=(1<<PINB7)|(1<<PINB0);  //SCK and Fsync High
	SPCR|=(1<<SPE)|(1<<MSTR)|(1<<CPOL); //Enable SPI, set master, prescaler = 4, SPI Mode:2
}

void SPI_Transmit(char data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF))){;/*wait for data transfer and recieving*/}
}

void SPI_write16 (char data)
{
	
}	
int main(void)
{
    
    while (1) 
    {
    }
}

