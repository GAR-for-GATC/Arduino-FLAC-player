#ifndef USART
#define USART
//ifndef, define and endif are all used to ensure that there are no multiple definitions


#define USART_BAUDRATE 9600
#define F_CPU 16000000
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

void Init_USART(void);

void transmitByte(uint8_t my_byte);
uint8_t receiveByte(void);
void USART_Buffer_Flush(void);

void printBinaryByte(uint8_t byte);
void print32BitNumber(uint32_t bits);
void print64BitNumber(uint64_t bits);

void newLine(void);
void printString(const char *myString);
void flashPrintf(const char *my_string);

void printNumber(uint8_t n);
void printNumber16(uint16_t n, uint8_t print_leading_zeros);
void printHex(uint16_t number, uint8_t *leading_zeros, uint8_t *endianess);
void printHex32(uint32_t number, uint8_t *leading_zeros, uint8_t *endianess);

void display_Block(uint32_t *block_address_offset, void* the_block);
char formatChar(uint8_t *purple);

uint8_t change_to_upper(uint8_t lower);
uint8_t is_lower(uint8_t my_char);


#endif