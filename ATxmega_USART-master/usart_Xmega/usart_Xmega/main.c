/*
 * atxmega_test.cpp
 *
 * Created: 11-08-2018 17:35:43
 * Author : Puneet Shrivas
 */ 

#define F_CPU 2000000UL
#define CPU_PRESCALER 1
#define PI 3.14159
#define NUM_BYTES 4
#include <avr/io.h>
#include <util/delay.h>
#include "USART.h"
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "spi_driver.h"
#define USART USARTC0
#define Fmclk 25000000			  //crystal frequency on AD development board
/*! \brief SPI master module on PORT C. */
SPI_Master_t spiMasterC;

/*! \brief SPI slave module on PORT D. */
SPI_Slave_t spiSlaveD;

/*! \brief SPI Data packet */
SPI_DataPacket_t dataPacket;

/*! \brief Test data to send from master. */
uint8_t masterSendData[NUM_BYTES];

/*! \brief Data received from slave. */
uint8_t masterReceivedData[NUM_BYTES];

/*! \brief Result of the test. */
bool success = true;

PORT_t *ssPort = &PORTC;					//instantiation for SPI

volatile int pixelCount=0,timeCount[2]={0,0};
int interruptPeriod = ((((F_CPU)/(CPU_PRESCALER*1000000))*532)-1);
int led =1;
USART_data_t USART_data;
void SetClock0()
{
//////////////////////////////////////////////////////////////////////////
//Compare Match A timer													//
//////////////////////////////////////////////////////////////////////////
// 	PMIC_EnableLowLevel();
// 	TCC0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_FRQ_gc;
// 	TCC0.INTCTRLB = (uint8_t) TC_CCAINTLVL_LO_gc;
// 	TCC0.PER =UINT16_MAX;
// 	TCC0_CCAH = ((TEMP>>8) & 0x00FF);
// 	TCC0_CCAL = (TEMP & 0x00FF);
// 	TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
//////////////////////////////////////////////////////////////////////////
//Overflow timer														//
//////////////////////////////////////////////////////////////////////////
	PMIC_EnableHighLevel();		//Enable interrupts : High level for timer
	TCC0.CTRLA = TC_CLKSEL_DIV1_gc;						 //Set Prescaler 1
	TCC0.CTRLB= TC_WGMODE_NORMAL_gc;	   //Wave generation mode : Normal
	TCC0.INTCTRLA = TC_OVFINTLVL_HI_gc;		   //Enable overflow interrupt
	TCC0.PER = interruptPeriod;		                   //Initialize Period  
}
void SetClock1()
{	
	TCC1.PER =0xFF;											  //Set period 
	TCC1.CTRLA = TC_CLKSEL_DIV1_gc;						 //Set Prescaler 1
}

void SetUsart()
{
	PORTC.DIRSET   = PIN3_bm;							//Set TX as output
	PORTC.DIRCLR   = PIN2_bm;					   		 //Set RX as input
	USART_InterruptDriver_Initialize(&USART_data, &USART, USART_DREINTLVL_LO_gc);	
	USART_Format_Set(USART_data.usart, USART_CHSIZE_8BIT_gc,USART_PMODE_DISABLED_gc, false);	
	USART_RxdInterruptLevel_Set(USART_data.usart, USART_RXCINTLVL_LO_gc);
	USART_Baudrate_Set(&USART, 12 , 0);				 //gives baudrate 9600
	USART_Rx_Enable(USART_data.usart);
	USART_Tx_Enable(USART_data.usart);
	PMIC.CTRL |= PMIC_LOLVLEX_bm;	//Enable low level interrupt for USART
}
void USARTsend(uint8_t data)
{
	sei();							
	USART_TXBuffer_PutByte(&USART_data, data);		
	cli();
}
void USARTsend16(uint16_t data)
{
	uint8_t MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	uint16_t LSdata = (data & 0x00FF);			//filter out LS
	sei();
	USART_TXBuffer_PutByte(&USART_data, MSdata);
	USART_TXBuffer_PutByte(&USART_data, LSdata);
	cli();
}

void SPI_Master_init()
{
	 /*  Hardware setup:
	 *    - Connect PC4 to PD4 (SS)
	 *    - Connect PC5 to PD5 (MOSI)
	 *    - Connect PC6 to PD6 (MISO)
	 *    - Connect PC7 to PD7 (SCK)
	 */
	PORTC.DIRSET = PIN4_bm;					    //Set SS as output
	PORTC.DIRSET = PIN5_bm;						//MOSI as output
	PORTC.DIRSET = PIN7_bm;						//SCK as output
	PORTC.PIN4CTRL = PORT_OPC_WIREDANDPULL_gc;  //Set PullUp at SS 
	PORTC.OUTSET = PIN4_bm;						//Set SS high for no Slave
	SPI_MasterInit(&spiMasterC,&SPIC,&PORTC,false,SPI_MODE_2_gc,SPI_INTLVL_OFF_gc,true,SPI_PRESCALER_DIV4_gc); //Initialize device as master 
}

void SPI_send8(uint8_t data)
{
	SPI_MasterSSLow(ssPort, PIN4_bm);   //Set Slave Select Low	
	SPI_MasterTransceiveByte(&spiMasterC, data); 
	SPI_MasterSSHigh(ssPort, PIN4_bm);	//Set Slave Select High
}
uint8_t SPI_receive8()
{
	SPI_MasterCreateDataPacket(&dataPacket,masterSendData,masterReceivedData,NUM_BYTES,&PORTC,PIN4_bm);
    SPI_MasterTransceivePacket(&spiMasterC, &dataPacket);
	return masterReceivedData[0];
	PORTF_OUTTGL = PIN4_bm;
}
void  SPI_send16(uint16_t data)    			
{
	uint8_t MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	uint8_t LSdata = (data & 0x00FF);			//filter out LS
	SPI_MasterSSLow(ssPort, PIN4_bm);   //Set Slave Select Low
	PORTA_OUTCLR = PIN0_bm;
	SPI_MasterTransceiveByte(&spiMasterC, MSdata); 
	SPI_MasterTransceiveByte(&spiMasterC, LSdata);
	SPI_MasterSSHigh(ssPort, PIN4_bm);	//Set Slave Select High
	PORTA_OUTSET = PIN0_bm;
//		PORTF_OUTTGL = PIN4_bm;
		//_delay_ms(100);
}
void Set_AD9833(float frequency, unsigned int phase)
{
	long FreqReg = (((float)frequency)*pow(2,28))/(float)Fmclk;	  //Calculate frequency to be sent to AD9833
	int MSB = (int)((FreqReg &  0xFFFC000) >> 14);		   //Extract first 14 bits of FreqReg and place them at last 14 bits of MSB
	int LSB = (int)((FreqReg & 0x3FFF));				  //Extract last 14 bits of FreqReg and place them at last 14 bits of MSB
	MSB|=0x4000;										  //Set D14,D15 = (1,0) for using FREQ0 registers, MSB has all 16 bits set
	LSB|=0x4000;     									  //Set D14,D15 = (1,0) for using FREQ0 registers, LSB has all 16 bits set
	SPI_send16(0x2100);								  //define waveform and set reset bit
	SPI_send16(LSB);									  //Write LSBs
	SPI_send16(MSB);									  //Write MSBs
	phase&=0x0FFF;
	phase|=0xC000;
	//SPI_write16(0xC000);								  //Mode selection for writing to phase register bit, selection of PHASE0 register (Needs to be fixed)
	SPI_send16(phase);
	SPI_send16(0x2000);
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
// 	PORTF_DIRSET=PIN7_bm;
// 	PORTF_DIRSET=PIN2_bm;
// 	PORTF_DIRSET=PIN4_bm;
//  	SetClock0();	
// 	SetClock1();
 //	SetUsart();
   // USARTsend('c');
   //sei();
   	PORTA_DIRSET = PIN0_bm;
	PORTF_DIRSET = PIN0_bm|PIN1_bm|PIN2_bm|PIN3_bm|PIN4_bm|PIN5_bm|PIN6_bm|PIN7_bm;
	PORTA_OUTSET = PIN0_bm;
	SPI_Master_init();
//SPI_send16(0x100);
//SPI_send16(0x2100);
// for (uint8_t i =1; i<255;i++)
// {
// 	SPI_send8(i);
// }
//SPI_send16(0x4000);
//SPI_send16(0xC000);
//SPI_send16(0x2000);
	
SPI_send16(0x100);
Set_AD9833(2300,0);
// 	TCC0.CNT=0;
// 	sei();
// 	//TCC1.CNT=0;
// 	while (pixelCount<1000);
// 	cli();
// 	PORTF_OUTTGL=PIN7_bm;
// 	_delay_ms(532);                                       
// 	PORTF_OUTTGL=PIN7_bm;
// 	pixelCount=0;
// 	led=2;
// 	sei();
// 	while (pixelCount<1000);
// 	cli();
// 	while(1)
// 	{
// 		Set_AD9833(3000,0);
// 		_delay_ms(1000);
// 	}
}
// void SPI_receive(uint8_t)
// {
// 	
// }


ISR(TCC0_OVF_vect)
{
	if(led==1) PORTF_OUTTGL=PIN4_bm;
	else if(led==2) PORTF_OUTTGL=PIN2_bm;
	pixelCount++;
	if (pixelCount<2)
	{
		timeCount[pixelCount]=TCC1_CNT;
	}
}

ISR(USARTC0_DRE_vect)
{
	USART_DataRegEmpty(&USART_data);
}