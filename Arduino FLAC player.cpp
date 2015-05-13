//This code is really messy and isn't cleaned up at all.  It's uploaded to Git
//  as a copy in case something happens. 


//Used on ATMega328p on breadboard.

#define USART_BAUDRATE 9600
#define F_CPU 16000000
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/power.h>


/*-----These are all conventions added with acr/io.h--------//
#define _BV(bit) (1 << (bit))
#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit)) //This checks if a bit is set.
#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit))) //checks if bit is clear
#define loop_until_bit_is_set(sfr, bit) do { } while (bit_is_clear(sfr, bit))
#define loop_until_bit_is_clear(sfr, bit) do { } while (bit_is_set(sfr, bit))
*/

uint8_t Send_SD_command(uint8_t command_number, uint32_t arg);
uint64_t Send_SD_command_long(uint8_t command_number, uint32_t arg);
void Init_SPI(void);
void SPI_send(uint8_t stuff);
uint8_t SPI_receive(void);
void Init_SD_Card(void);
void Init_USART(void);
void transmitByte(uint8_t my_byte);
void printBinaryByte(uint8_t byte);
void printString(const char myString[]);
void newLine(void);
void print32BitNumber(uint32_t bits);
//void printMultipleof8Bits(uint_least8_t my_bits);
void printNumber(uint8_t n);
void print64BitNumber(uint64_t bits);
//void printMultipleof8Bits(uint_least64_t *green);

////////////////////////////////////////////////////////////////////////////////
//Main Program//////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main(void){
	uint8_t b = 0;
	uint8_t c = 0;
	uint32_t d = 787878;
	uint64_t error_codes = 0;
	Init_USART();
	print64BitNumber(error_codes);
	newLine();
	b = 10;
	printBinaryByte(b);
	newLine();
	printBinaryByte(b);
	newLine();
	//uint64_t apple = *d;
	//printMultipleof8Bits(apple);
	Init_SPI();
	Init_SD_Card();
	//c = Send_SD_command(0,0);
	//printBinaryByte(c);
	//Init_SPI();
	//error_codes = Send_SD_command_long(0,0);
	//print64BitNumber(error_codes);
	//print32BitNumber(d);
	//newLine();
	//Init_SPI();	
	//_delay_ms(500);
	//c = Send_SD_command(0,0);
	//printBinaryByte(c);
	//newLine();
	//Init_SD_Card();
	
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
//USART Functions///////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Init_USART(void)
{
	clock_prescale_set(clock_div_1);
	UCSR0B = (1<<RXEN0)|(1<<TXEN0); //Enables the USART transmitter and receiver
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00)|(1<<USBS0); //tells it to send 8bit characters (setting both USCZ01 and UCSZ00 to one)
	//now it has 2 stop bits.

	UBRR0H = (BAUD_PRESCALE >> 8); //loads the upper 8 bits into the high byte of the UBRR register
	UBRR0L = BAUD_PRESCALE; //loads the lower 8 bits
}

void printBinaryByte(uint8_t byte){
	
	//This code is really efficient.  Instead of
	//using large ints to loop, it uses small uint8_t's.
	for (uint8_t bit=7; bit<255; bit--){
		if(bit_is_set(byte,bit)){
			transmitByte('1');
		}
		else{
			transmitByte('0');
		}
	}
}
//uint8_t is used for 8bit chars
void transmitByte(uint8_t my_byte){
	//This can print 8bit ints to the serial monitor, but sometimes prints chars???
	//fifth bit, if UDRE0is empty, then the transmit buffer is empty and ready to receive new
	//data.
	do{}while(bit_is_clear(UCSR0A, UDRE0));
	//UDR0 is the transmit register.
	UDR0 = my_byte;
}
void newLine(void){
	printString("\r\n");
}
//Prints 64 bit number.
void print64BitNumber(uint64_t bits){
	printBinaryByte(bits >> 56);
	printBinaryByte(bits >> 48);
	printBinaryByte(bits >> 40);
	printBinaryByte(bits >> 32);
	printBinaryByte(bits >> 24);
	printBinaryByte(bits >> 16);
	printBinaryByte(bits >> 8);
	printBinaryByte(bits);
}
void printString(const char myString[]){
	uint8_t i = 0;
	while(myString[i]){
		while ((UCSR0A &(1<<UDRE0)) == 0){}//do nothing until transmission flag is set
		UDR0 = myString[i]; // stick Chars in the register.  They gets sent.
		i++;
	}
}


//Prints 32 bit number.
void print32BitNumber(uint32_t bits){
	printBinaryByte(bits >> 24);
	printBinaryByte(bits >> 16);
	printBinaryByte(bits >> 8);
	printBinaryByte(bits);
}

/*
//prints a data type of the pointer passed to it.  To use, pass
//a pointer or whatever you want to print. Only prints up to 64 bit
//numbers.
void printMultipleof8Bits(uint_least64_t *green){
	for (int purple = sizeof(green); purple<0; purple--){
		printBinaryByte(green >> (purple*8));
	}
}
*/
void printNumber(uint8_t n){//This function Prints a number to the serial monitor
	//Algorithm to convert 8 bit binary to 3 digit decimal.
	//N=16(n1)+1(n0)
	//N=n1(1*10+6*1)+n0(1*1)
	//N=10(n1)+1(6(n1)+1(n0))
	//Also: N = 100(d2)+10(d1)+1(d0)
	//Then make: a1 = n1 and a0 = (6(n1)+1(n0))
	//Then get rid of the carries since n0 can be from 0-15, etc.
	//c1 = a0/10 This is the number of 10's to carry over
	//d0 = a0%10 This is the decimal that's outputted.
	//c2 = (a1+c1)/10
	//d1 = (a1+c1)%10
	//d2 = c2
	uint8_t d2, d1, q;
	uint16_t d0;
	//0xF is 15
	//00010000 this is 16 (n)
	//d0 = 00010000 & 00001111, d0 = 00000000
	d0 = n & 0xF;
	//If you AND n then the bits in the original number show in d0,d1 and d2
	//d1 = (00010000 >> 4) same as 00000001, & 00001111, d1 = 00000001
	//this sees if there's anything in the second 4 bits
	d1 = (n>>4) & 0xF;
	d2 = (n>>8);
	//this sets d2 to 0.
	
	d0 = 6*(d2+d1)+d0;
	q = d0/10;
	d0 = d0%10;
	
	d1 = q + 5*d2 + d1;
	if (d1!=0){
		q = d1/10;
		d1 = d1%10;
		
		d2 = q+2*d2;
		if (d2!=0){
			transmitByte(d2 + '0');
			//remember to add '0' because its an ASCII char
		}
		transmitByte(d1 + '0');
	}
	transmitByte(d0 + '0');
	
}
////////////////////////////////////////////////////////////////////////////////
//petite FAT, and Read\Write Functions//////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/* The File Allocation Table is a file system.  It's lightweight, but has low 
performance, reliability or scalability.  Better ones that should be considered
in the future would be FAT-FTL, YAFFS2 and JFFS2.  FAT was made for floppy disks
in 1977.  
A FAT file system has these partitions in the following order: A boot sector, reserved
  sectors, FAT #1, FAT #2, a root directory (not in FAT32), the part with all the
  data.  The size of each sector is specified in the boot sector and usually is
  512 bytes. There are 2 FATs in case one of them gets corrupted.  The size of the
  FATs depends on the filesystem used and the total disk space. Root is usually 512
  bytes large.  FAT32 puts its root directory in the data area.  Each partition has 
  its own boot sector.  
The configuration can be disk is located at the start of the boot sector, starting
  from offset 0x0B.  An offset is 00-0F bytes (16 bytes) large. The 51 bytes starting
  at 0x0B is called the Disk Parameter Block (DPB), and tells the size of the sector, # of
  reserved sectors, # of FATS, and the size of root.
The 1st 3 bytes in boot are jump commands in 80x86 code around the DPB.  It's similar to 
  BIOS in that it can interrupt and do stuff.  The 8 bytes after the jump commands are 
  the vendor name of the disk.  The following list is how FAT32 is separated and what's
  in it.:
Offset | Description                                                                                | Size
-----------------------------------------------------------------------------------------------------------
0x000  | Intel 80x86 jump instruction	                                                            | 3
0x003  | OEM name (not the volume name, see offset 0x02B)	                                        | 8
0x00B  | Sector size in bytes	                                                                    | 2
0x00D  | Number of sectors per cluster	                                                            | 1
0x00E  | Reserved sectors (including the boot sector)	                                            | 2
0x010  | Number of FATs	                                                                            | 1
0x011  | Number of directory entries in the root directory (N.A. for FAT32)	                        | 2
0x013  | Total number of sectors on the disk/partition, or zero for disks/partitions bigger than    | 2
       |  32 MiB (the field at offset 0x020 should then be used)	                                |                           
0x015  | Media descriptor (bit #2 holds whether the disk is removable)	                            | 1
0x016  | Number of sectors used for one FAT table (N.A. for FAT32)	                                | 2
0x018  | Number of sectors per track (cylinder), CHS addressing	                                    | 2
0x01a  | Number of heads, CHS addressing	                                                        | 2
0x01c  | Number of hidden sectors, which is the number of sectors before the boot sector            | 4
       |  (this field may be set to zero, though)	                                                |          
0x020  | Total number of sectors on the disk/partition, if this is above 32 MiB                     | 4
       |  (only valid if the field at offset 0x013 is zero)	                                        |                   
0x024  | FAT32 only: Number sectors in the one FAT, replaces the field at offset 0x016	            | 4 
0x028  | FAT32 only: Flags for FAT mirroring & active FAT	                                        | 2
0x02a  | FAT32 only: File system version number	                                                    | 2
0x02c  | FAT32 only: Cluster number for the root directory, typically 2	                            | 4
0x030  | FAT32 only: Sector number for the FSInfo structure, typically 1	                        | 2
0x032  | FAT32 only: Sector number for a backup copy of the boot sector, typically 6	            | 2
0x034  | FAT32 only: Reserved for future expansion	                                                | 12
0x040  | Drive number (this field is at offset 0x024 in FAT12/FAT16)	                            | 1
0x041  | Current head (internal to DOS; this field is at offset 0x025 in FAT12/FAT16)	            | 1
0x042  | Boot signature, the value 0x29 indicates the three next fields are valid                   | 1
       |  (this field is at offset 0x026 in FAT12/FAT16)	                                        | 
0x043  | Volume ID (serial number; this field is at offset 0x027 in FAT12/FAT16)	                | 4
0x047  | Volume label (this field is at offset 0x02b in FAT12/FAT16)	                            | 11
0x052  | File system type (this field is at offset 0x036 in FAT12/FAT16)	                        | 8
0x08a  | Boot code (starts at offset 0x03e in FAT12/FAT16, and is 448 bytes)	                    | 327
0x1fe  | Boot sector signature, must be 0x55AA	                                                    | 2

There is a difference between logical and physical sectors. FAT uses logical sectors.  If you use linux
and look at CHS addressing (Cylinder, Head, Sector), logical sectors need to be converted to physical 
sectors.  Using a Logical based system you need to add an offset to the data areas of the logical sector.

FAT uses a "hard disk", or "fixed disk" format.  The information about the partitions are located in the
  Master Boot Record (MBR) and is 512 bytes large.
  
The partition table holds a bunch of lists, and contains a value that indicates the 
  type of file system in use and sets a flag that tells whether the partition is 
  active.
This is the layout of the Partition Table:
Offset | Description                                                        | Size
-----------------------------------------------------------------------------------
0x00  | 0x80 if active (bootable), 0 otherwise	                            | 1
0x01  | start of the partition in CHS-addressing	                        | 3
0x04  | type of the partition, see below	                                | 1
0x05  | end of the partition in CHS-addressing	                            | 3
0x08  | relative offset to the partition in sectors (LBA)	                | 4
0x0C  | size of the partition in sectors	                                | 4

Different flags that show what kind of file system is in use.  The partition record 
  should always hold the relative linear offset to the start of the partition. For 
  use in embedded devices, the types 4, 6 and 14 may all be seen as equivalent.
Partition Type | Description
-----------------------------------------------------------------------------------
0              | empty / unused
1              | FAT12
4              | FAT16 for partitions <= 32 MiB
5              | extended partition
6              | FAT16 for partitions > 32 MiB
11             | FAT32 for partitions <= 2 GiB
12             | Same as type 11 (FAT32), but using LBA addressing, which removes size constraints
14             | Same as type 6 (FAT16), but using LBA addressing
15             | Same as type 5, but using LBA addressing
*/


/* SD commands 17 and 18 reads single or multiple blocks respectively.
After the command is sent for read, the SD card sends a response, then data,
the length of which is set by a previous command, SD command 16.  The Data Response
Token is 8 bits long.  Blocks can be anywhere from 1 to 512 bytes large.  The data 
block is followed by a 16 bit CRC code, generated by the formula: y=(x^16)+(x^12)+(x^5)+1.
  In a multiple block read, there are multiple data blocks with CRC codes and
the ends by sending the stop command, command 12.
  If the read function fails, then the SD card sends a 1 byte error message, with 
the first 4 bits are 0.
*/
//Using the code found here for read, write and format commands.
//http://elm-chan.org/fsw/ff/00index_p.html
//http://codeandlife.com/2012/04/02/simple-fat-and-sd-tutorial-part-1/
//http://www.compuphase.com/mbr_fat.htm
/*
This function formats the SD card for FATF, only use this once per card.
Formating the card too much shortens the life of that card.  
*/
void Init_Fat32(void){
	//pointer to the struct boot_sector
	boot_sector *boot_sector_pointer;
	partition_info *partition_info_pointer;
	first_partition_info *first_par_pointer;
	
	//This changes the void pointer returned from ReadBlock
	//The max block size in SD cards is 512 bytes.
	boot_sector_pointer = (boot_sector*)ReadBlock(0,malloc(512));
	
	//This checks if something is in bootsector
	//The '->' dereferences one of the fields of a pointer to a struct.
	if ((boot_sector_pointer -> jumpboot[0] != 0xE9 ) && ( boot_sector_pointer -> jumpboot[0] != 0xEB ) ){
		//This is a type conversion for the bpb pointer to a mbr pointer?
		partition_info_pointer = ( partition_info*) boot_sector_pointer;
		//The signature default should be 0xaa55
		//This means that 
		if (partition_info_pointer -> signature != 0xaa55) 
			return 1;     
		else;
	}
}

//This function reads 512 bytes of data and loads it into the ATMEGA328p flash
//  memory.  It returns a void pointer to the flash memory where it is stored.
//The inputs are the block address you want to read on the SD card and the 
//place you want to put it in flash.
void* ReadBlock(uint64_t block_address, uint8_t *pointer_to_memory){
	
}


////////////////////////////////////////////////////////////////////////////////
//SD Card Functions/////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//This function uses these codes to determine what to send.
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
uint8_t Send_SD_command(uint8_t command_number, uint32_t arg){
	uint8_t error_codes = 0;
	//printBinaryByte(error_codes);
	//newLine();
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
	//error_codes = 255;
	// Waits for a response other than 1111 1111.
	//The 1's are because the MISO line is pulled to high.
	while((error_codes = SPI_receive()) == 0xff);
	//error_codes = SPI_receive();
	PORTB &= ~(1<<DDB2); //Turns off SS line
	return error_codes;
}

//This command is to be used with command 8 or 58.
//These commands return a 40 bit error code, but are packaged in 64 bits
//because it's easier to package.
uint64_t Send_SD_command_long(uint8_t command_number, uint32_t arg){
	uint8_t purple[5];
	uint64_t error_codes = 0; 
	//print64BitNumber(error_codes);
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
	//error_codes = 255;
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
				//transmitByte('1');
				error_codes |= (push_bit << (loop_bit+(i*8)));
			}
			else{
				//error_codes |= (1 <<(loop_bit+(0*8)));
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

	//This waits for a receive signal, the pullup resistor makes it stay at
	//1 until a command is sent from the SD card.
	while (0xFF == (first_command = Send_SD_command(0,0)));

		
	printBinaryByte(first_command);
	newLine();
	
	//Put SS to 1, turn on PB2
	//PORTB = (PORTB | 0b00000100);
	
	//Asks whether the SD card supports the voltage in the arguments.
	//It returns a 40 bit statement, but only the first 8 bits really
	//matter anyway.
	//SEND_IF_COND, returns 40bits
	//while(0x01 != Send_SD_command(8, 0x000001AA));
	//uint8_t apple = SPI_receive();
	//printBinaryByte(apple);
	//uint32_t fuchia = 0x000001AA;
	while (0xFF == (second_command = Send_SD_command_long(8,0x000001AA)));
	//while(0x01 != (second_command = Send_SD_command(8,purple)));
	
	//while(0x01 != (second_command = Send_SD_command_long(8,fuchia)));
	print64BitNumber(second_command);
	
	//00000000 00000000 00000001 10101010
	//The following commands work with standard capacity SD cards.
	//print32BitNumber(second_command);
	newLine();
	while (0xFF == (third_command = Send_SD_command(55, 0)));
	printBinaryByte(third_command);
	newLine();
	while (0xFF == (fourth_command = Send_SD_command(41, 0x40000000)));
	printBinaryByte(fourth_command);
	newLine();
	while (0xFF == (fifth_command = Send_SD_command_long(58, 0)));
	print64BitNumber(fifth_command);
	newLine();
	
	/*
	//Loops until one of them isn't true.
	//APP_CMD is command number 55, It tells the SD card that the next command is an application
	//specific command, rather than a standard command.
	//SD_SEND_OP COMP is command number 41, Sends host capacity support (HCS) information
	//and activates the card initialization process.
	//printBinaryByte(third_command);
	uint32_t green = 0x40000000;
	uint8_t orange = 55;
	uint8_t yellow = 41;
	while((third_command = Send_SD_command(orange, 0)) && (fourth_command = Send_SD_command(yellow, green)));
	printBinaryByte(third_command);
	newLine();
	printBinaryByte(fourth_command);
	newLine();
	//Command 58, reads the ORC register on the SD card, assigns CSS bit to OCR register.
	//returns 40 bits
	while(0x00 != (fifth_command = Send_SD_command(58, 0)));
	print32BitNumber(fifth_command);
	newLine();
	*/
	//Put SS to 0, turn off PB2
	//PORTB &= ~(1 << PB2);
	
	/*
	SPCR = 0x50; //0101 0000, resets SPCR bits
	SPSR |= ( 1 << SPI2X ); //Turns on double speed, doubles SPI clock speed.
	//This is done because the SD card is supposed to wait a certain number of
	//clock cycles.
	_delay_ms(10);
	
	//These are used for trouble shooting.
	//Init_USART();
	*/

}
////////////////////////////////////////////////////////////////////////////////
//SPI Functions/////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void Init_SPI(void)
{
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
uint8_t SPI_receive(void)
{
	//I'm not sure why this line works, but 
	//without it MISO doesn't work.
	SPDR = 0xff; //1111 1111
	while ( !(SPSR &(1 << SPIF)));
	return SPDR;
}
////////////////////////////////////////////////////////////////////////////////
//Structs used with FAT Formating Functions/////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//These Structs and all of these other functions will be put into .h files 
//  eventually.
/*
#define READ_ONLY     		0x01
#define HIDDEN        		0x02
#define SYSTEM        		0x04
#define VOLUME_ID     		0x08
#define DIRECTORY     		0x10
#define ARCHIVE       		0x20
#define LONG_NAME     		0x0f
#define DIR_ENTRY_SIZE     	0x32
#define EMPTY              	0x00
#define DELETED            	0xe5
#define GET     		   	0
#define SET                	1
#define READ	           	0
#define VERIFY             	1
#define ADD		           	0
#define REMOVE	           	1
#define LOW		           	0
#define HIGH	           	1
#define TOTAL_FREE         	1
#define NEXT_FREE          	2
#define GET_LIST           	0
#define GET_FILE           	1
#define DELETE		       	2
#define FAT_EOF	           	0x0fffffff
*/
//Structure to access Master Boot Record for getting info about partitions
typedef struct
{
	unsigned char	ignore [446];							//ignore, placed here to fill the gap in the structure
	unsigned char	partitiondata [64];						//partition records (16x4)
	unsigned int	signature;								//0xaa55
}									partition_info;

//Structure to access info of the first partition of the disk
typedef struct
{
	unsigned char	status;									//0x80 - active partition
	unsigned char 	headstart;								//starting head
	unsigned int	cylsectstart;							//starting cylinder and sector
	unsigned char	type;									//partition type
	unsigned char	headend;								//ending head of the partition
	unsigned int	cylsectend;								//ending cylinder and sector
	unsigned long	firstsector;							//total sectors between MBR & the first sector of the partition
	unsigned long	sectorstotal;							//size of this partition in sectors
}									first_partition_info;

//Structure to access boot sector data
typedef struct
{
	unsigned char jumpboot [3]; 							//default: 0x009000EB
	unsigned char oemname [8];
	unsigned int bytespersector; 							//deafault: 512
	unsigned char sectorpercluster;
	unsigned int reservedsectorcount;
	unsigned char numberoffats;
	unsigned int rootentrycount;
	unsigned int totalsectors_f16; 							//must be 0 for FAT32
	unsigned char mediatype;
	unsigned int fatsize_f16; 								//must be 0 for FAT32
	unsigned int sectorspertrack;
	unsigned int numberofheads;
	unsigned long hiddensectors;
	unsigned long totalsectors_F32;
	unsigned long fatsize_f32; 								//count of sectors occupied by one FAT
	unsigned int extflags;
	unsigned int fsversion; 								//0x0000 (defines version 0.0)
	unsigned long rootcluster; 								//first cluster of root directory (=2)
	unsigned int fsinfo; 									//sector number of FSinfo structure (=1)
	unsigned int backupbootsector;
	unsigned char reserved [12];
	unsigned char drivenumber;
	unsigned char reserved1;
	unsigned char bootsignature;
	unsigned long volumeid;
	unsigned char volumelabel [11]; 						//"NO NAME "
	unsigned char filesystemtype [8];						//"FAT32"
	unsigned char bootdata [420];
	unsigned int bootendsignature; 							//0xaa55
}										boot_sector;

//Structure to access FSinfo sector data
typedef struct
{
	unsigned long leadsignature;   							//0x41615252
	unsigned char reserved1 [480];
	unsigned long structuresignature;						//0x61417272
	unsigned long freeclustercount; 						//initial: 0xffffffff
	unsigned long nextfreecluster; 							//initial: 0xffffffff
	unsigned char reserved2 [12];
	unsigned long trailsignature; 							//0xaa550000
}										fs_info;

//Structure to access Directory Entry in the FAT
typedef struct
{
	unsigned char name [11];
	unsigned char attrib; 									//file attributes
	unsigned char ntreserved; 								//always 0
	unsigned char timetenth; 								//tenths of seconds, set to 0 here
	unsigned int createtime; 								//time file was created
	unsigned int createdate; 								//date file was created
	unsigned int lastaccessdate;
	unsigned int firstclusterhi; 							//higher word of the first cluster number
	unsigned int writetime; 								//time of last write
	unsigned int writedate; 								//date of last write
	unsigned int firstclusterlo; 							//lower word of the first cluster number
	unsigned long filesize; 								//size of file in bytes
}										fs_dir;
