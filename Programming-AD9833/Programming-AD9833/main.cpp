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
#include <util/atomic.h>

int TEMP = ((((F_CPU)/(TIMER1_PRESCALER*1000000))*560.5)-1);			//Counter Cycles for required time557
int TICKS = 65535-TEMP;												//Value for TCNT1 to implement timing by overflow
	
volatile float global_frequency=0;										//Volatile global variables for use in interrupt service routine
volatile float prev_freq = 0;
volatile int cont=0;
int cont_copy=0;
volatile unsigned int prev_phase=0;
volatile unsigned int next_phase=0;
volatile int contprev = 0;
volatile int contnext = 0;
volatile int t=0;
volatile int compare = 0;
volatile int notSet = 0;
void SPI_init(void)
{
	DDRB=(1<<PINB7)|(1<<PINB5)|(1<<PINB0);								//sets SCK, MOSI,SS and PINB0 as output (F sync at Pinb0)
	PORTB=(1<<PINB0)|(1<<PINB4);										//F sync High, SS is set high
	SPCR=(1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<SPIE)|(1<<SPI2X);				//Enable SPI, set master, pre scaler = 2, SPI Mode:2
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
	int MSB = (int)((FreqReg &  0xFFFC000) >> 14);		   //Extract first 14 bits of FreqReg and place them at last 14 bits of MSB
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

//color conversion from RGB to Y/RY/BY
int R1=0,G1=0,B1=0;
float Y1 = 16.0 + (.003906 * ((65.738 * R1) + (129.057 * G1) + (25.064 * B1)));
float RY1 = 128.0 + (.003906 * ((112.439 * R1) + (-94.154 * G1) + (-18.285 * B1)));
float BY1 = 128.0 + (.003906 * ((-37.945 * R1) + (-74.494 * G1) + (112.439 * B1)));
//frequency calculation and documented values
volatile float freqY1  =  1500 + (Y1 * 3.1372549);			//1757.2549(red)	1954.90196(green)	1628.62745(blue)
volatile float freqRY1 =   1500 + (RY1 * 3.1372549);		//2252.94118(red)  1606.66667(green)	1845.09804(blue)
volatile float freqBY1 =  1500 + (BY1 * 3.1372549);		//1782.35294(red)	1669.41177(green)	2252.94118(blue)

int R2=255,G2=255,B2=255;
float Y2 = 16.0 + (.003906 * ((65.738 * R2) + (129.057 * G2) + (25.064 * B2)));
float RY2 = 128.0 + (.003906 * ((112.439 * R2) + (-94.154 * G2) + (-18.285 * B2)));
float BY2 = 128.0 + (.003906 * ((-37.945 * R2) + (-74.494 * G2) + (112.439 * B2)));
//frequency calculation and documented values
volatile float freqY2  =  1500 + (Y2  * 3.1372549);			//1757.2549(red)	1954.90196(green)	1628.62745(blue)
volatile float freqRY2 =  1500 + (RY2 * 3.1372549);		//2252.94118(red)  1606.66667(green)	1845.09804(blue)
volatile float freqBY2 =  1500 + (BY2 * 3.1372549);		//1782.35294(red)	1669.41177(green)	2252.94118(blue)

int main(void)
{
	UART_init();
	SPI_init();
	DDRA=(1<<PINA0)|(1<<PINA1)|(1<<PINA2);			//output pins for LEDs
	TCCR1A=0;
	PORTA=0;

	//test timers
	
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
	// 	TCCR1B|=(1<<CS10);
	// 	for(int i=1;i<=10;i++)
	// 	{
	// 		TCNT1=0;
	// 		//next_phase=getphase(prev_phase,2000,100000);
	// 		Set_AD9833(2000,next_phase);
	// 		cont=TCNT1 ;
	// 		UART_write16(cont);
	// 	}
	// 		
	// 	for(int i=1;i<5;i++)
	// 	{
	// 		j=i;
	// 	cont=0;
	// 	contnext=0;
	// 	contprev=0;
	// 	TCNT0=0;
	// 	sei();
	// 	TCCR1B|=(1<<CS10);
	// 	TIMSK|=(1<<OCIE1A);
	// 	TCNT1=0;
	// 	OCR1A=TEMP;
	// 	//TCNT1=65534;
	// 	do
	// 	{
	// 		ATOMIC_BLOCK(ATOMIC_FORCEON)
	// 		{
	// 			cont_copy=cont;
	// 		}
	// 	} while (cont_copy<2);
	// 	cli();
	// 	TIMSK&=~(1<<OCR1A);
	// 	TCCR1B=0x00;
	// 	UART_send(contprev);
	// 	UART_send(contnext);
	// 	UART_send(j);
	// // 	UART_send(contprev);
	// // 	UART_send(contnext);
	// 	}
	//////////////////////////////////////////////////////////////////////////
	

	SPI_write16(0x100);								//Reset AD9833 

	//VIS Code
// 	{//leader tone
// 	_delay_ms(500);
// 	Set_AD9833(1900,0);
// 	_delay_ms(300);
// 	//break
// 	Set_AD9833(1200,0);
// 	_delay_ms(10);
// 	//leader
// 	Set_AD9833(1900,0);
// 	_delay_ms(300);
// 	//VIS start bit
// 	Set_AD9833(1200,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//PD90 VIS code = 99d = 0b1100011
// 	//bit 0=1
// 	Set_AD9833(1100,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//bit 1=1
// 	Set_AD9833(1100,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//bit 2=0
// 	Set_AD9833(1300,0);
// 	_delay_ms(29);  _delay_us(839);
// 	//bit 3=0
// 	Set_AD9833(1300,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//bit 4=0
// 	Set_AD9833(1300,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//bit 5=1
// 	Set_AD9833(1100,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//bit 6=1
// 	Set_AD9833(1100,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//Parity bit
// 	Set_AD9833(1300,0);
// 	_delay_ms(29);	_delay_us(839);
// 	//stop bit
// 	Set_AD9833(1200,0);
// 	_delay_ms(29);	_delay_us(839); 			
// 	}
// 
// 	//image data
// 	for (int i=1;i<=128;i++)
// 	{
// 	//Sync Pulse
// 	Set_AD9833(1200,0);
// 	_delay_ms(19);	_delay_us(840);		//Time in protocol minus programming time of Set_AD9833()
// 	
// 	//Porch
// 	Set_AD9833(1500,0);
// 	_delay_ms(1);	_delay_us(919);		//Time in protocol minus programming time of Set_AD9833()
// 
// 	//Color transmission	
// 	cont=1;								// variable for maintaining count of pixels
// 	global_frequency=freqY1;			//initialization for first pixel
// 	sei();				
// 	TCCR1B=0;		
// 	TCCR1B|=(1<<CS10)|(1<<WGM12);
// 	TIMSK|=(1<<OCIE1A);
// 	OCR1A=TEMP;
// 	TCNT1=TEMP-1; 
// 	while(cont<=1280);					// wait loop for interrupts  to complete
// 	cli();
// 	TIMSK&=~(1<<OCIE1A);
// 	TCCR1B=0x00;
// 	PORTA=0;
// 	
// 	//added delay for straightening image
// // 	_delay_ms(1);
// 	//_delay_us(100);
// 
// 	//color be delay 
// 	{// 	
// 
// 
// // // 		//Y Scan odd line
// // // 		for (int j=1;j<=8;j++)
// // // 		{
// // // 			Set_AD9833(1757.2549);
// // // 			_delay_us(10479.409722); //532*20-160.590278
// // // 			Set_AD9833(1954.90196);
// // // 			 _delay_us(10479.409722);
// // // 		}
// // // 		//R-Y Scan average
// // // 		for (int j=1;j<=8;j++)
// // // 		{
// // // 			Set_AD9833(2252.94118);
// // // 			 _delay_us(10479.409722);
// // // 			Set_AD9833(1606.66667);
// // // 			 _delay_us(10479.409722);
// // // 		}
// // // 		//B-Y Scan average
// // // 		for (int j=1;j<=8;j++)
// // // 		{
// // // 			Set_AD9833(1782.35294); _delay_us(10479.409722);
// // // 			Set_AD9833(1669.41177); _delay_us(10479.409722);
// // // 		}
// // // 		//Y Scan even line
// // // 		for (int j=1;j<=8;j++)
// // // 		{
// // // 			Set_AD9833(1757.2549); _delay_us(10479.409722);
// // // 			Set_AD9833(1954.90196); _delay_us(10479.409722);
// // // 		}
// 		//Y Scan odd line
// // 		Set_AD9833(freqY1,0); 
// // 		_delay_us(170079.41);
// // 
// // 		//R-Y Scan average
// // 		Set_AD9833(freqRY1,0); 
// // 		_delay_us(170079.41);
// // 
// // 		//B-Y Scan average
// // 		Set_AD9833(freqBY1,0); 
// // 		_delay_us(170079.41);
// // 
// // 		//Y Scan even line
// // 		Set_AD9833(freqY1,0);
// // 		_delay_us(170079.41);
// 
// }
// 
// 	}
// 
// Set_AD9833(0x00,0);
Set_AD9833(2300,0);
	while(1)
	{
	
	}

}

ISR(TIMER1_COMPA_vect)
{
	if(compare==0)
	{
		Set_AD9833(global_frequency,next_phase);
		notSet=0;
	}
	else
	{
		notSet++;
	}
//	Set_AD9833(global_frequency,next_phase);	          
	prev_phase=next_phase;
	prev_freq = global_frequency;	
// 	if(cont==319) global_frequency = freqRY1;
// 	else if(cont==639) global_frequency = freqBY1;
// 	else if(cont==959) global_frequency = freqY1;
	if(((cont-2)%20)==0) 
	{
		t = (cont-2)/20;
		if((t%2)==0)
		{
			if(t<15) global_frequency = freqY1;
			else if(t<31) global_frequency = freqRY1;
			else if(t<47) global_frequency = freqBY1;
			else if(t<63) global_frequency = freqY1;
		}
		else if((t%2)==1)
		{
			if(t<16) global_frequency = freqY2;
			else if(t<32) global_frequency = freqRY2;
			else if(t<48) global_frequency = freqBY2;
			else if(t<64) global_frequency = freqY2;
		}
	}
	next_phase = getphase(prev_phase,prev_freq,(532*notSet));		//calculation of phase to be added in new wave
	cont++;
	if (global_frequency==prev_freq) compare=1;
	else compare =0;
	if (cont==1280) compare=0;

}
	
 EMPTY_INTERRUPT(SPI_STC_vect) //to prevent reset on Empty SPI interrupt 