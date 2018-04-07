/*
 * Programming-AD9833.cpp
 *
 * Created: 19-03-2018 20:55:19
 * Author : Puneet Shrivas
 */ 

#define F_CPU 14745600			 
#define Fmclk 25000000			  //crystal frequency on AD development board
#define SINE 0x2100               //mode 0
#define SQUARE 0x2128			  //mode 1
#define TRIANGLE 0x2102			  //mode 2
#define BAUDRATE 9600			  //Baud rate for UART
#define BAUD_PRESCALLER (((F_CPU/(BAUDRATE * 16UL))) - 1) //Predefined formula from data sheet
#define TIMER1_PRESCALER 64
#include <avr/io.h>
#include <avr/delay.h>
#include <math.h>
#include <avr/interrupt.h>

void SPI_init(void)
{
	DDRB=(1<<PINB7)|(1<<PINB5)|(1<<PINB0);         //sets SCK, MOSI,SS and PINB0 as output (F sync at Pinb0)
	PORTB=(1<<PINB0)|(1<<PINB4);					//F sync High, SS is set high
	SPCR=(1<<SPE)|(1<<MSTR)|(1<<CPOL)/*|(1<<SPR1)|(1<<SPR0)*/|(1<<SPIE)|(1<<SPI2X);				//Enable SPI, set master, pre-scaler = 2, SPI Mode:2
}

void UART_init(void)
{
	UBRRH = (unsigned char)(BAUD_PRESCALLER>>8);
	UBRRL = (unsigned char)BAUD_PRESCALLER;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0); // Set frame format: 8data, 2stop bit
}

void UART_send(unsigned char data)
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
	PORTB|=(1<<PINB4);							//set SS pin high every time 
	//UART_send(data);							//check data sent to AD
	SPDR=data;
	while(!(SPSR&(1<<SPIF))) {;/*wait for data transfer and recieving*/} //Error is possible here
}

void SPI_write16 (unsigned short data)    			//send a 16bit word and use f sync
{  
	unsigned char MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	unsigned char LSdata = (data & 0x00FF);			//filter out LS
	PORTB &= ~(1<<PINB0);						    //F sync Low --> begin frame
	SPI_transfer(MSdata);							
	SPI_transfer(LSdata);
	PORTB |= (1<<PINB0);						    //F sync High --> End of frame
}

void Set_AD9833(float frequency)
{
	long FreqReg = (frequency*pow(2,28))/(float)Fmclk;  //Calculate frequency to be sent to AD9833
	int MSB = (int)((FreqReg &  0xFFFC000) >> 14);		  //Extract first 14 bits of FreqReg and place them at last 14 bits of MSB
	int LSB = (int)((FreqReg & 0x3FFF));				  //Extract last 14 bits of FreqReg and place them at last 14 bits of MSB	
	MSB|=0x4000;										  //Set D14,D15 = (1,0) for using FREQ0 registers, MSB has all 16 bits set
	LSB|=0x4000;     									  //Set D14,D15 = (1,0) for using FREQ0 registers, LSB has all 16 bits set
	SPI_write16(0x2100);								  //define waveform and set reset bit
	SPI_write16(LSB);									  //Write LSBs
	SPI_write16(MSB);									  //Write MSBs
	SPI_write16(0xC000);								  //Mode selection for writing to phase register bit, selection of PHASE0 register (Needs to be fixed)
	SPI_write16(0x2100);
}

int main(void)
{
	UART_init();
	SPI_init();
	DDRA=(1<<PINA0)|(1<<PINA1)|(1<<PINA2);
	PORTA=0;
	
// 	//test timer
// 	float count;
// 	TCCR0|=(1<<CS10)|(1<<CS11);
// 	TCNT0=0;
// 	Set_AD9833(0x00);
// 	count=TCNT0;
// 	UART_send(count);
// 	
// 	float micros = ((count+1)*TIMER1_PRESCALER/F_CPU)*1000000;
// 	float millis=micros*1000;
// 	float linetime =532*320;
	
	//color yellow
	int R=255,G=255,B=0;
	float Y = 16.0 + (.003906 * ((65.738 * R) + (129.057 * G) + (25.064 * B)));
	float RY = 128.0 + (.003906 * ((112.439 * R) + (-94.154 * G) + (-18.285 * B)));
	float BY = 128.0 + (.003906 * ((-37.945 * R) + (-74.494 * G) + (112.439 * B)));
	float freqY  =  1500 + (Y * 3.1372549);
	float freqRY =  1500 + (RY * 3.1372549);
	float freqBY =  1500 + (BY * 3.1372549);
	
	SPI_write16(0x100);							//Reset AD9833 
	
	/*VIS CODE*/
	//leader tone
	Set_AD9833(1900);
	_delay_ms(300);
	//break
	Set_AD9833(1200);
	_delay_ms(10);
	//leader
	Set_AD9833(1900);
	_delay_ms(300);
	//VIS start bit
	Set_AD9833(1200);
	_delay_ms(30);
	//PD90 VIS code = 99d = 0b1100011
	//bit 0=1
	Set_AD9833(1100);
	_delay_ms(30);
	//bit 1=1
	Set_AD9833(1100);
	_delay_ms(30);
	//bit 2=0
	Set_AD9833(1300);
	_delay_ms(30);
	//bit 3=0
	Set_AD9833(1300);
	_delay_ms(30);
	//bit 4=0
	Set_AD9833(1300);
	_delay_ms(30);
	//bit 5=1
	Set_AD9833(1100);
	_delay_ms(30);
	//bit 6=1
	Set_AD9833(1100);
	_delay_ms(30);
	//Parity bit
	Set_AD9833(1100);
	_delay_ms(30);
	//stop bit
	Set_AD9833(1200);
	_delay_ms(30);
	
	
	for (int i=1;i<=256/* -16 ? */;i++)
	{
		
		//Sync Pulse
		Set_AD9833(1200);
		_delay_ms(20);
		//Porch
		Set_AD9833(1500);
		_delay_ms(2.080);
		
		//Color transmission
		//Y Scan odd line 
		Set_AD9833(freqY);
		_delay_us(484.8686);
		//R-Y Scan average
		Set_AD9833(freqRY);
		_delay_us(484.8686);
		//B-Y Scan average
		Set_AD9833(freqBY);
		_delay_us(484.8686);
		//Y Scan even line
		Set_AD9833(freqY);
		_delay_us(484.8686);
	}
	
	while (1) 
    {
    }
}

