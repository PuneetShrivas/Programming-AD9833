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
#define TIMER1_PRESCALER 1
#define TIMER0_PRESCALER 0
#define PI 3.14159
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <avr/interrupt.h>

int TEMP = ((((F_CPU)/(TIMER1_PRESCALER*1000000))*552)-1);			//Counter Cycles for required time
int TICKS = 65536-TEMP;												//Value for TCNT1 to implement timing by overflow
	
volatile float global_frequency=0;										//Volatile global variables for use in interrupt service routine
volatile int cont=0;
volatile unsigned int prev_phase=0;
volatile unsigned int next_phase=0;
volatile int contprev = 0;
volatile int contnext = 0;
volatile float timeprev = 1;
void SPI_init(void)
{
	DDRB=(1<<PINB7)|(1<<PINB5)|(1<<PINB0);								//sets SCK, MOSI,SS and PINB0 as output (F sync at Pinb0)
	PORTB=(1<<PINB0)|(1<<PINB4);										//F sync High, SS is set high
	SPCR=(1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<SPIE)|(1<<SPI2X);				//Enable SPI, set master, pre-scaler = 2, SPI Mode:2
}

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

void led(int i)
{
	switch (i)
	{
		case 0 : PORTA|=(1<<PINA0);break;
		case 1 : PORTA|=(1<<PINA1);break;
		case 2 : PORTA|=(1<<PINA2);break;
	}
}

void SPI_transfer(uint8_t data)
{
	SPDR=data;
	while(!(SPSR&(1<<SPIF))); 											//wait for data transfer and receiving
}

void UART_write16(unsigned short data)
{
	unsigned char MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	unsigned char LSdata = (data & 0x00FF);			//filter out LS
	UART_send(MSdata);
	UART_send(LSdata);
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

void Set_AD9833(float frequency, unsigned int phase)
{
	long FreqReg = (frequency*pow(2,28))/(float)Fmclk;	  //Calculate frequency to be sent to AD9833
	int MSB = (int)((FreqReg &  0xFFFC000) >> 14);		  //Extract first 14 bits of FreqReg and place them at last 14 bits of MSB
	int LSB = (int)((FreqReg & 0x3FFF));				  //Extract last 14 bits of FreqReg and place them at last 14 bits of MSB	
	MSB|=0x4000;										  //Set D14,D15 = (1,0) for using FREQ0 registers, MSB has all 16 bits set
	LSB|=0x4000;     									  //Set D14,D15 = (1,0) for using FREQ0 registers, LSB has all 16 bits set
	SPI_write16(0x2100);								  //define waveform and set reset bit
	SPI_write16(LSB);									  //Write LSBs
	SPI_write16(MSB);									  //Write MSBs
	phase&=0x0FFF;
	phase|=0xC000;
	//SPI_write16(0xC000);								  //Mode selection for writing to phase register bit, selection of PHASE0 register (Needs to be fixed)
	SPI_write16(phase);
	SPI_write16(0x2000);                                                                                                                                                                                                                                                                                             
}

unsigned int getphase(float pphase,float freq, float time)
{
	time/=1000000;
	pphase/=2048/PI;
	float ph=((fmod(time,(1/freq))*2*PI*freq)+pphase)*2048/PI;
	return (unsigned int) ph;
}

int main(void)
{
	UART_init();
	SPI_init();
	DDRA=(1<<PINA0)|(1<<PINA1)|(1<<PINA2);			//output pins for LEDs
	TCCR1A=0;
	PORTA=0;
	
	SPI_write16(0x100);
		//test timer
	{
	//////////////////////////////////////////////////////////////////////////						
	// 	TCNT0=0;																													
	// 	next_phase = getphase(prev_phase,global_frequency,532);
	// 	//add frequency retrieval function here
	// 	Set_AD9833(global_frequency,next_phase);
	// 	prev_phase=next_phase;													
	// 	cont=TCNT0;					
	// 	PORTA=0;
	// 	UART_write16(cont);										
	// 	//160.590278				
	// 	//325.520833 us with phase correction											
	//////////////////////////////////////////////////////////////////////////
	}

// 	while(1)
// 	{
// 		TCCR0=(1<<CS00)|(1<<CS01);
// 		TCNT0=0;
// 		next_phase=getphase(prev_phase,2000,100000);
// 		Set_AD9833(2000,next_phase);
// 		cont=TCNT0 ;
// 		UART_write16(cont);
// 	}
// }
	//color conversion from RGB to Y/RY/BY 
	int R=0,G=255,B=0;
	float Y = 16.0 + (.003906 * ((65.738 * R) + (129.057 * G) + (25.064 * B)));
	float RY = 128.0 + (.003906 * ((112.439 * R) + (-94.154 * G) + (-18.285 * B)));
	float BY = 128.0 + (.003906 * ((-37.945 * R) + (-74.494 * G) + (112.439 * B)));
	//frequency calculation and documented values
	float freqY  =  1500 + (Y * 3.1372549);			//1757.2549(red)	1954.90196(green)	1628.62745(blue)	
	float freqRY =  1500 + (RY * 3.1372549);		//2252.94118(red)  1606.66667(green)	1845.09804(blue)
	float freqBY =  1500 + (BY * 3.1372549);		//1782.35294(red)	1669.41177(green)	2252.94118(blue)|
	
	SPI_write16(0x100);								//Reset AD9833 
	
	//VIS Code
	{//leader tone
	Set_AD9833(1900,0);
	_delay_ms(300);
	//break
	Set_AD9833(1200,0);
	_delay_ms(10);
	//leader
	Set_AD9833(1900,0);
	_delay_ms(300);
	//VIS start bit
	Set_AD9833(1200,0);
	_delay_ms(30);
	//PD90 VIS code = 99d = 0b1100011
	//bit 0=1
	Set_AD9833(1100,0);
	_delay_ms(30);
	//bit 1=1
	Set_AD9833(1100,0);
	_delay_ms(30);
	//bit 2=0
	Set_AD9833(1300,0);
	_delay_ms(30);
	//bit 3=0
	Set_AD9833(1300,0);
	_delay_ms(30);
	//bit 4=0
	Set_AD9833(1300,0);
	_delay_ms(30);
	//bit 5=1
	Set_AD9833(1100,0);
	_delay_ms(30);
	//bit 6=1
	Set_AD9833(1100,0);
	_delay_ms(30);
	//Parity bit
	Set_AD9833(1100,0);
	_delay_ms(30);
	//stop bit
	Set_AD9833(1200,0);
	_delay_ms(30);
	}
	
	//image data
	for (int i=1;i<=5;i++)
	{
	//Sync Pulse
	Set_AD9833(1200,0);
	_delay_ms(19);
	_delay_us(839);
	//Porch
	Set_AD9833(1500,0);
	_delay_ms(1);
	_delay_us(920);
	//Color transmission	
	//single color using interrupts
	
	{
		
	//Y Scan odd line
	cont=0;	
	global_frequency=freqY;	
	sei();
	TCCR1B|=(1<<CS10);
	TIMSK|=(1<<TOIE1);
	TCNT1=65534;
	while(cont<2);
	cli();
	TIMSK&=~(1<<OCIE1A);
	TCCR1B=0x00;
	
	//R-Y Scan average
	cont=0;
	global_frequency=freqRY;
	sei();
	TCCR1B|=(1<<CS10);
	TIMSK|=(1<<TOIE1);
	TCNT1=TICKS;
	while(cont<320);
	cli();
	TIMSK&=~(1<<OCIE1A);
	TCCR1B=0x00;
	
	//B-Y Scan average
	cont=0;
	global_frequency=freqBY;
	sei();
	TCCR1B|=(1<<CS10);
	TIMSK|=(1<<TOIE1);
	TCNT1=TICKS;
	while(cont<320);
	cli();
	TIMSK&=~(1<<OCIE1A);
	TCCR1B=0x00;
	
	//Y Scan even line
	cont=0; 
	global_frequency=freqY;
	sei();
	TCCR1B|=(1<<CS10);
	TIMSK|=(1<<TOIE1);
	TCNT1=TICKS;
	while(cont<321);
	cli();
	TIMSK&=~(1<<OCIE1A);
	TCCR1B=0x00;
	}
	{
// 		//Y Scan odd line
// 		for (int j=1;j<=8;j++)
// 		{
// 			Set_AD9833(1757.2549);
// 			_delay_us(10479.409722); //532*20-160.590278
// 			Set_AD9833(1954.90196);
// 			 _delay_us(10479.409722);
// 		}
// 		//R-Y Scan average
// 		for (int j=1;j<=8;j++)
// 		{
// 			Set_AD9833(2252.94118);
// 			 _delay_us(10479.409722);
// 			Set_AD9833(1606.66667);
// 			 _delay_us(10479.409722);
// 		}
// 		//B-Y Scan average
// 		for (int j=1;j<=8;j++)
// 		{
// 			Set_AD9833(1782.35294); _delay_us(10479.409722);
// 			Set_AD9833(1669.41177); _delay_us(10479.409722);
// 		}
// 		//Y Scan even line
// 		for (int j=1;j<=8;j++)
// 		{
// 			Set_AD9833(1757.2549); _delay_us(10479.409722);
// 			Set_AD9833(1954.90196); _delay_us(10479.409722);
// 		}
// 		//Y Scan odd line
// 		Set_AD9833(freqY,0); 
// 		_delay_us(170079.41);
// 
// 		//R-Y Scan average
// 		Set_AD9833(freqRY,0); 
// 		_delay_us(170079.41);
// 
// 		//B-Y Scan average
// 		Set_AD9833(freqBY,0); 
// 		_delay_us(170079.41);
// 
// 		//Y Scan even line
// 		Set_AD9833(freqY,0);
// 		_delay_us(170079.41);
}
}	
    Set_AD9833(0x00,0);

	while(1)
	{		
	}
}

ISR(TIMER1_OVF_vect)
{
// 	contprev=contnext;
// 	contnext=TCNT0;
// 	timeprev = (contnext-contprev)*TIMER0_PRESCALER/F_CPU;
	TCNT1=TICKS;
	next_phase = getphase(prev_phase,global_frequency,532);
	cont++;
	//add frequency retrieval function here
	Set_AD9833(global_frequency,next_phase);
	prev_phase=next_phase;
}
	
 EMPTY_INTERRUPT(SPI_STC_vect) //to prevent reset on Empty SPI interrupt 