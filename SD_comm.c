////////////////////////////////////////////////////////////////////////////////
//SD Card Functions/////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "include.h"

//This function uses these codes to determine what to send to the SD card.
/*
#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_IF_COND			 8
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define SD_SEND_OP_COND			 41
#define APP_CMD					 55
#define READ_OCR				 58
#define CRC_ON_OFF               59
*/
//  Sends 6 bytes of commands to the SD card and receives an 8 bit error code
//from the SD card.  If the code is "0000 0001", then no errors have occurred.
//If the is_init variable is set to 1, then the function assumes that this is part of the
//SD card initialization process to set the SD card to SPI mode.
uint8_t Send_SD_command(uint8_t command_number, uint32_t arg, uint8_t is_init){
	uint8_t error_codes = 0;

	if (is_init == 1){
		PORTB |= DDB2; //Puts PB2 on high, turns on SS line.
		_delay_ms(50);
	}
	else{
		PORTB |= DDB2; //Puts PB2 on high, turns on SS line.
	}
	
	SPI_send(command_number | 0b01000000);
	//This code sends 8 bits at a time through MOSI.
	SPI_send(arg >> 24);
	SPI_send(arg >> 16);
	SPI_send(arg >> 8);
	SPI_send(arg);
	
	//If command 8 is sent, then use this at the end of the command instead.
	if (command_number == 8 ){
		SPI_send(0x87); //1000 0111
	}
	else{
		SPI_send(0x95); //1001 0101
	}
	
	// Waits for a response other than 1111 1111.
	//The 1's are because the MISO line is pulled to high.
	while((error_codes = SPI_receive()) == 0xff);
	//error_codes = SPI_receive();
	if (is_init == 1){
		_delay_ms(50);
		PORTB &= ~(1<<DDB2); //Turns off SS line
	}
	else{
		PORTB &= ~(1<<DDB2); //Turns off SS line
	}
	
	return error_codes;
}

//This command is to be used with command 8 or 58.
//These commands return a 40 bit error code, but are packaged in 64 bits
//because it's easier to package.
uint64_t Send_SD_command_long(uint8_t command_number, uint32_t arg){
	uint8_t purple[5];
	uint64_t error_codes = 0;
	
	PORTB |= DDB2; //Puts PB2 on high, turns on SS line.
	SPI_send(command_number | 0b01000000);
	//This code sends 8 bits at a time through MOSI.
	SPI_send(arg >> 24);
	SPI_send(arg >> 16);
	SPI_send(arg >> 8);
	SPI_send(arg);
	
	//If command 8 is sent, then use this at the end of the command instead.
	if (command_number == 8 ){
		SPI_send(0x87); //1000 0111
	}
	else{
		SPI_send(0x95); //1001 0101
	}
	
	// Waits for a response other than 1111 1111.
	//The 1's are because the MISO line is pulled to high.
	
	while((purple[0] = SPI_receive()) == 0xff);
	purple[1] = SPI_receive();
	purple[2] = SPI_receive();
	purple[3] = SPI_receive();
	purple[4] = SPI_receive();
	uint64_t push_bit = 1;
	for (int8_t i=0; i<5; i++){
		for (uint8_t loop_bit=7; ((loop_bit < 8)||(loop_bit < 254)); loop_bit--){
			push_bit = 1;
			//Left this debug code because it's fun to watch
			//printNumber(loop_bit);
			//print64BitNumber(error_codes);
			//newLine();
			if(bit_is_set(purple[i],loop_bit)){
				error_codes |= (push_bit << (loop_bit+(i*8)));
			}
			else{
				error_codes &= ~(push_bit <<(loop_bit+(i*8)));
			}
		}
	}
	PORTB &= ~(1<<DDB2); //Turns off SS line
	return error_codes;
}



//This function is run to initialize the SD card.
void Init_SD_Card(void){
	//Setting all the variables to 0 prevents warnings about uninitialized variables.
	// Otherwise the variables are filled with whatever was there before.
	uint8_t first_command = 0;
	uint64_t second_command = 0;
	uint8_t third_command = 0;
	uint8_t fourth_command = 0;
	uint64_t fifth_command = 0;
	//Reset the SD card, puts SD card in SPI mode.
	//All sequential receive commands should be put into while loops so the
	//  uController will wait for the correct signal to be sent back before
	//  sending the next one.

	//The While loopa waits for a receive signal, the pullup resistor makes it stay at
	//1 until a command is sent from the SD card.
	while (0xFF == (first_command = Send_SD_command(0,0,1)));
	
	//Asks whether the SD card supports the voltage in the arguments.
	//It returns a 40 bit statement, but only the first 8 bits really
	//matter anyway.
	while (0xFF == (second_command = Send_SD_command_long(8,0x000001AA)));

	//00000000 00000000 00000001 10101010
	//The following commands work with standard capacity SD cards.
	while (0xFF == (third_command = Send_SD_command(55, 0,0)));

	while (0xFF == (fourth_command = Send_SD_command(41, 0x40000000,0)));

	while (0xFF == (fifth_command = Send_SD_command_long(58, 0)));
	_delay_ms(30);
	
}


//This function reads a 512 byte sector from the SD card starting at an offset of
//	the address given.  Then it puts all of the bytes in a 512 byte uint8_t array.
//	The void pointer needs to be malloced before hand and passed to it.  This function
//  will not malloc, so it's easier to manage and free sectors loaded into ATMega memory.
//
void read_one_block(uint32_t address, void* hold_sector){
	uint8_t error_stuff;
	uint16_t i;
	
	address = address << 9;
	
	
	// The command READ_SINGLE_BLOCK is represented as 17.
	while ( 0x00 != (error_stuff = Send_SD_command(17, address, 0)));//{printBinaryByte(error_stuff);newLine();}
	//Command 17 will send a reply of 0000 0000 if no errors occur while reading data.
	if (error_stuff != 0b00000000){
		flashPrintf(READ_BLOCK_ERROR);
		printBinaryByte(error_stuff);
		newLine();
	}
	//Wait until the correct start sequence is received from SPI.
	//FE signifies the beginning of a data packet.
	while (0xFE != SPI_receive());
	
	char *ptr = (char*) hold_sector;
	for (i = 0; i<512; i++){
		//This and "++ptr" are used to traverse the struct
		*ptr = SPI_receive();
		++ptr;
	}

}
