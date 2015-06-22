////////////////////////////////////////////////////////////////////////////////
//SPI Functions/////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "include.h"

void Init_SPI(void){
	//SS is PB2, MOSI is PB3, MISO is PB4, SCK is PB5
	// init SS pin as output.
	// init MOSI pin as output.
	// init MISO pin as input.
	// init SCLK pin as output.
	DDRB = (1<<PB2)|(1<<PB3)|(1<<PB5);
	
	//set spi clock between 100 and 400 kHz
	//Write to the SPI data register to start the SPI clock generator.
	//SPE - Enable SPI, MSTR - makes ATMEGA master
	// At 16,000,000 clock, setting SPR1 will divide the clock by
	//64.  This should set the SPI clock to 250,000.
	// SPIE enables interrupts.
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1);//|(1<<SPIE);
	//enable global interrupts
	//sei();
	
	//These should all be 0 anyway, set them to 0 just in case.
	SPSR = 0b00000000;
	
	//Added this to prevent errors.
	_delay_ms(10);
}


//This sends stuff, 8 bits at a time, through MOSI.
void SPI_send(uint8_t stuff){
	SPDR = stuff;
	while (!(SPSR &(1 << SPIF)));
}


//This function receives stuff from MISO
uint8_t SPI_receive(void){
	//I'm not sure why this line works, but
	//without it MISO doesn't work.
	SPDR = 0xff; //1111 1111
	while ( !(SPSR &(1 << SPIF)));
	return SPDR;
}













