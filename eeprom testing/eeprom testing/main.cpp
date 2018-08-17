/*
 * eeprom testing.cpp
 *
 * Created: 27-06-2018 00:09:01
 * Author : Puneet Shrivas
 */ 


#define I2C_BAUD 400000UL 
#define F_CPU 14745600
#define Prescaler 1
#define BAUDRATE 9600			  //Baud rate for UART
#define BAUD_PRESCALLER (((F_CPU/(BAUDRATE * 16UL))) - 1) //Predefined formula from data sheet
#define MAX_ADDR 131072
#define HALF_ADDR 65536
#define TIMER1_PRESCALER 1
int TEMP = ((((F_CPU)/(TIMER1_PRESCALER*1000000))*560.5)-1);			//Counter Cycles for required time560.5
#define _BV(bit) (1<<(bit))
#define MAX_ITER	200
#define PAGE_SIZE 128
#include <avr/io.h>			//definitions have to be before inclusions
#include <util/twi.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
uint32_t write_addr=0;
uint8_t twst;
volatile uint32_t cont=0;
/*volatile*/ uint8_t byte[200];
static uint8_t eeprom_addr = 0b10100110;	/* E2 E1 E0 = 0 0 0 */
void UART_send(uint8_t data)
{
	while(!(UCSRA & (1<<UDRE)));										//wait for buffer to be emptied
	UDR=data;
}
void led()
{
	DDRA=(1<<PINA0);
	PORTA=(1<<PINA0);
}
void ioinit(void)
{

  /* initialize TWI clock: 100 kHz clock, TWPS = 0 => prescaler = 1 */
#if defined(TWPS0)
  /* has prescaler (mega128 & newer) */
  TWSR = 0;
#endif

#if F_CPU < 3600000UL
  TWBR = 10;			/* smallest TWBR value, see note [5] */
#else
  TWBR = (F_CPU / I2C_BAUD - 16) / (2*Prescaler);
#endif
}


int eeprom_read_bytes_part(uint32_t eeaddr, int len,/*volatile */uint8_t *buf)
{
  uint8_t sla, twcr, n = 0;
  int rv = 0;
  
  ///* Added code for handling the two halves of the EEPROM
  if(eeaddr >= HALF_ADDR)
  {
    eeaddr -= HALF_ADDR;
    eeprom_addr |= 0x08;
  }
  else
  {
    eeprom_addr &= ~0x08;
  }
  
  /* patch high bits of EEPROM address into SLA */
  sla = eeprom_addr;

  /*
   * Note [8]
   * First cycle: master transmitter mode
   */
 restart:
  if (n++ >= MAX_ITER)
    return -1;
 begin:

  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_REP_START:	 	/* OK, but should not happen */
    case TW_START:
      break;

    case TW_MT_ARB_LOST:	/* Note [9] */
      { goto begin;}

    default:
      return -1;		/* error: not in start condition */
      /* NB: do /not/ send stop condition */
    }

  /* Note [10] */
  /* send SLA+W */
  TWDR = sla | TW_WRITE;
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_SLA_ACK:
      break;

    case TW_MT_SLA_NACK:	/* nack during select: device busy writing */
      /* Note [11] */
      goto restart;

    case TW_MT_ARB_LOST:	/* re-arbitrate */
     {  goto begin;}

    default:
      goto error;		/* must send stop condition */
    }

  TWDR = (eeaddr>>8);		/* high 8 bits of addr */
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;

    case TW_MT_DATA_NACK: 

    case TW_MT_ARB_LOST:
      { goto begin;}

    default:
      goto error;		/* must send stop condition */
    }

  TWDR = eeaddr;		/* low 8 bits of addr */
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;

    case TW_MT_DATA_NACK:
     {  goto quit;}

    case TW_MT_ARB_LOST:
      { goto begin;}

    default:
      goto error;		/* must send stop condition */
    }

  /*
   * Note [12]
   * Next cycle(s): master receiver mode
   */
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send (rep.) start condition */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_START:		/* OK, but should not happen */
    case TW_REP_START:
      break;

    case TW_MT_ARB_LOST:
      { goto begin;}

    default:
      goto error;
    }

  /* send SLA+R */
  TWDR = sla | TW_READ;
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MR_SLA_ACK:
      break;

    case TW_MR_SLA_NACK:
      goto quit;

    case TW_MR_ARB_LOST:
     {  goto begin;}

    default:
      goto error;
    }

  for (twcr = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);	len > 0;len--)
    {
      if (len == 1)
	twcr = _BV(TWINT) | _BV(TWEN); /* send NAK this time */
      TWCR = twcr;		/* clear int to start transmission */
      while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
      switch ((twst = TW_STATUS))
	{
	case TW_MR_DATA_NACK:
	  len = 0;		/* force end of loop */
				/* FALLTHROUGH */
	case TW_MR_DATA_ACK:
	  *buf++ = TWDR;
	  rv++;
	  break;

	default:
	  goto error;
	}
    }
 quit:
  /* Note [14] */
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */

  return rv;

 error:
   
  rv = -1;
  goto quit;
}

int eeprom_write_page(uint32_t eeaddr, int len, uint8_t *buf)
{
  uint8_t sla, n = 0;
  int rv = 0;
  uint16_t endaddr;
  
  ///* Added code for handling the two halves of the EEPROM
  if(eeaddr >= HALF_ADDR)
  {
    eeaddr -= HALF_ADDR;
    eeprom_addr |= 0x08;
  }
  else
    eeprom_addr &= ~0x08;

  if (eeaddr + len < (eeaddr | (PAGE_SIZE - 1)))
    endaddr = eeaddr + len;
  else
    endaddr = (eeaddr | (PAGE_SIZE - 1)) + 1;
  len = endaddr - eeaddr;

  /* patch high bits of EEPROM address into SLA */
  sla = eeprom_addr;

 restart:
  if (n++ >= MAX_ITER)
    return -1;
 begin:

  /* Note [15] */
  TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_REP_START:		/* OK, but should not happen */
    case TW_START:
      break;
    case TW_MT_ARB_LOST:
      goto begin;
    default:
      return -1;		/* error: not in start condition */
      /* NB: do /not/ send stop condition */
    }

  /* send SLA+W */
  TWDR = sla | TW_WRITE;
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_SLA_ACK:
      break;

    case TW_MT_SLA_NACK:	/* nack during select: device busy writing */
      goto restart;

    case TW_MT_ARB_LOST:	/* re-arbitrate */
      goto begin;

    default:
      goto error;		/* must send stop condition */
    }
	
	
  TWDR = (eeaddr>>8);		/* high 8 bits of addr */
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;

    case TW_MT_DATA_NACK:
      goto quit;

    case TW_MT_ARB_LOST:
      goto begin;

    default:
      goto error;		/* must send stop condition */
    }


  TWDR = eeaddr;		/* low 8 bits of addr */
  TWCR = _BV(TWINT) | _BV(TWEN); /* clear interrupt to start transmission */
  while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
  switch ((twst = TW_STATUS))
    {
    case TW_MT_DATA_ACK:
      break;

    case TW_MT_DATA_NACK:
      goto quit;

    case TW_MT_ARB_LOST:
      goto begin;

    default:
      goto error;		/* must send stop condition */
    }

  for (; len > 0; len--)
    {
      TWDR = *buf++;
      TWCR = _BV(TWINT) | _BV(TWEN); /* start transmission */
      while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
      switch ((twst = TW_STATUS))
	{
	case TW_MT_DATA_NACK:
	  goto error;		/* device write protected -- Note [16] */

	case TW_MT_DATA_ACK:
	  rv++;
	  break;

	default:
	  goto error;
	}
    }
 quit:
  TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN); /* send stop condition */

  return rv;

 error:
  rv = -1;
  goto quit;
}

int eeprom_write_bytes(uint32_t eeaddr, int len, uint8_t *buf)
{
  int rv, total;

  total = 0;
  do
    {
      rv = eeprom_write_page(eeaddr, len, buf);
      if (rv == -1)
        return -1;
      eeaddr += rv;
      len -= rv;
      buf += rv;
      total += rv;
    }
  while (len > 0);

  return total;
}

void error(void)
{
	led();	
  exit(0);
}

int eeprom_read_bytes(uint32_t eeaddr, int len, /*volatile*/ uint8_t *buf)
{
	if((eeaddr < HALF_ADDR) && ((eeaddr + len) > HALF_ADDR))
	{
		int first = HALF_ADDR - eeaddr;
		eeprom_read_bytes_part(eeaddr, first, buf);
		return eeprom_read_bytes_part(HALF_ADDR, len - first , buf + first);
	}
	
	return eeprom_read_bytes_part(eeaddr, len, buf);
}

void UART_init(void)
{
	UBRRH = (unsigned char)(BAUD_PRESCALLER>>8);						//UART initialization from data-sheet
	UBRRL = (unsigned char)BAUD_PRESCALLER;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);							// Set frame format: 8data, 2stop bit
}


unsigned char UART_Receive( void )
{
	/* Wait for data to be received */
	while ( !(UCSRA & (1<<RXC)) )
	;
	/* Get and return received data from buffer */
	return UDR;
}

void UART_write16(unsigned short data)
{
	unsigned char MSdata = ((data>>8) & 0x00FF);  	//filter out MS
	unsigned char LSdata = (data & 0x00FF);			//filter out LS
	UART_send(MSdata);
	_delay_us(500);
	UART_send(LSdata);
}

int main(void)
{	

	ioinit();
	UART_init();
	uint8_t  output[2], data[200]; 
	//UART_send(6);
	int count=0;
// 	byte[0]=17; byte[1]=13;
// 	TCCR1B|=(1<<CS10);
// 	TCNT1=0;
// 	eeprom_read_bytes(write_addr, 2 , data);
// 	count = TCNT1;
// 	UART_write16(count);
// 	UART_send(data[0]);
	//UART_send(data[1]);
// for (int i=0;i<225;i++)
// {	
// 	byte[0]=i;
// 	eeprom_write_bytes(eeprom_addr+i,2,byte);
// }
// for (int i=12;i<=226;i++)
// {	
// 	
// 	eeprom_read_bytes(eeprom_addr+i,1,data);
// 	UART_send(data[0]);
// 	//USART_Flush();
// 	_delay_ms(100);
// 	UART_send(i);
// 	_delay_ms(100);
// }
UART_send(0);
for (write_addr=0;write_addr<81920;write_addr++)
{
	byte[0]=UART_Receive();
	eeprom_write_bytes(write_addr, 1, byte);
	eeprom_read_bytes(write_addr,1,data);
// 	eeprom_read_bytes(write_addr, 1, data);
	UART_send(data[0]);
}
UART_send(1);
	//write_addr+=0;
	
	
// 	for (uint32_t i=0;i<200;i++)
// 	{	
// 		eeprom_read_bytes(write_addr+(2*i),2,data);
// 		UART_send(data[0]);
// 		_delay_ms(300);
// 		UART_send(data[1]);
// 		_delay_ms(300);
// /*		_delay_ms(1);*/
// 	}
// 	UART_send(01);
// for ( int i=0; i<5;i++)
// {
// 	output[0]=0; output[1]=0;
// 	int data=0;
// 	int check2 = eeprom_read_bytes(eeprom_addr+(i*2),2, output);
// 	data|=(output[);
// 	data|=output[0]<<81];
// 	UART_write16(data);
// }
//  	UART_send(output[0]);
//  	UART_send(output[1]);
// 	 UART_send(byte[0]);
// 	 UART_send(byte[1]);
//     //UART_send('a');
	/* Replace with your application code */
//     while (1) 
//     {
// 		PORTA^=(1<<PINA0);
// 		_delay_ms(5000);
//     }
// 	sei();
// 	TCCR1B=0;
// 	TCCR1B|=(1<<CS10)|(1<<WGM12);
// 	TIMSK|=(1<<OCIE1A);
// 	OCR1A=TEMP;
// 	TCNT1=TEMP-1;
// 	while(cont<=100);					// wait loop for interrupts  to complete
// 	cli();
// 	TIMSK&=~(1<<OCIE1A);
// 	TCCR1B=0x00;
}

// ISR(TIMER1_COMPA_vect)
// {
// 	byte[0]=0; byte[1]=0;
// 	eeprom_read_bytes(write_addr+cont,1,byte);
// 	cont++;
// 	UART_send(byte[0]);
// }