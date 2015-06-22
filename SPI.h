#ifndef SPI
#define SPI
//ifndef, define and endif are all used to ensure that there are no multiple definitions



void Init_SPI(void);
void SPI_send(uint8_t stuff);
uint8_t SPI_receive(void);

#endif