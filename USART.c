////////////////////////////////////////////////////////////////////////////////
//USART Functions///////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "include.h"

void Init_USART(void){
	clock_prescale_set(clock_div_1);
	UCSR0B = (1<<RXEN0)|(1<<TXEN0); //Enables the USART transmitter and receiver
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00)|(1<<USBS0); //tells it to send 8bit characters (setting both USCZ01 and UCSZ00 to one)
	//now it has 2 stop bits.

	UBRR0H = (BAUD_PRESCALE >> 8); //loads the upper 8 bits into the high byte of the UBRR register
	UBRR0L = BAUD_PRESCALE; //loads the lower 8 bits
}
////////////////////////////////////////////////////////////////////////////////
//Basic Tx/Rx Functions/////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void transmitByte(uint8_t my_byte){
	//This can print 8bits to the serial monitor.  The serial monitor interprets the bits as ascii chars.
	//If UDRE0 is empty, then the transmit buffer is empty and ready to receive new
	//data.
	do{}while(bit_is_clear(UCSR0A, UDRE0));
	//UDR0 is the transmit register.
	UDR0 = my_byte;
}

uint8_t receiveByte(void){
	
	while(!(UCSR0A & (1<<RXC0)));
	return UDR0;
	
}
//This function dumps the receive buffer.  Use this before receiving user input.
void USART_Buffer_Flush(void){
	unsigned char dummy;
	//Read data into dummy char until buffer indicator, RXCn, is 0.
	while ( UCSR0A & (1<<RXC0) ){ dummy = UDR0;}
}

////////////////////////////////////////////////////////////////////////////////
//Used to print individual bits/////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void printBinaryByte(uint8_t byte){
	for (uint8_t bit=7; bit<255; bit--){
		if(bit_is_set(byte, bit)){
			transmitByte('1');
		}
		else{
			transmitByte('0');
		}
	}
}

void print32BitNumber(uint32_t bits){
	printBinaryByte(bits >> 24);
	printBinaryByte(bits >> 16);
	printBinaryByte(bits >> 8);
	printBinaryByte(bits);
}

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

////////////////////////////////////////////////////////////////////////////////
//String Printing Functions/////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void newLine(void){
	//const char a[] = "\r\n";
	//printString(a);
	flashPrintf(PSTR("\r\n"));
}

//Prints a string. Example of use:
//	const char my_string[] = "purple";
//	printString(my_string);
void printString(const char *myString){
	uint8_t i = 0;
	while(myString[i]){
		while ((UCSR0A &(1<<UDRE0)) == 0){}//do nothing until transmission flag is set
		UDR0 = myString[i]; // stick Chars in the register.  They gets sent.
		i++;
	}
}


//This is used to print strings from program memory.
//	Don't give it too strings too large or you'll get a malloc error.
//	This function needs ~1.6kB of program memory.

//Function should use ~40 bytes of SRAM when used, plus the length of the string
//	passed to it.  Assume it uses ~80 bytes.
void flashPrintf(const char *my_string){
	//pgm_read_byte returns a 16 bit addresses of things stored in flash.
	uint8_t i;
	uint8_t counter = 0;
	uint8_t array_counter = 1;
	uint8_t array_size = 10;
	unsigned char* cup;
	
	//The following allocates a small array using malloc.  Elements are read into
	//	the array and the amount of elements are recorded.  When the array is full,
	//	use realloc to double the size of the array.  This will repeat until the
	//	there is nothing left to read into the array.  Then the char array will be
	//	printed using printf.
	
	cup = malloc(array_size*sizeof(unsigned char));
	if (cup == NULL){
		const char a[] = "malloc failed. ";
		printString(a);
		exit(1);
	}

	while(pgm_read_byte(my_string) != 0x00){
		if (array_counter == 10){ //If the malloced space is full
			cup = realloc(cup, array_size+10);
			if(cup == NULL){
				free(cup);
				const char a[] = "Realloc failed.";
				printString(a);
				exit(1);
			}
			array_counter = 0;
		}
		cup[counter] = pgm_read_byte(my_string++);
		//This exits the loop when a null char is detected.
		//All strings should have null chars at the end of them.
		if(cup[counter] == '\0'){
			goto EXIT_LOOP;  //Small jump command to break out of a loop
		}
		counter++;
		array_counter++;
	}
	EXIT_LOOP:;
	char print_array[counter];
	//const char garbb[] = "purple";
	for (i=0; i<(counter ); i++){
		print_array[i] = cup[i];

	}
	
	free(cup);
	print_array[counter] = '\0';
	printString(print_array);


}

////////////////////////////////////////////////////////////////////////////////
//Number Printing Functions/////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//Uses around 5 bytes of SRAM
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
	//this gives the carry over bit.
	q = d0/10;
	//This gives the last digit in base 10
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


//This prints an unsigned 16 bit number as ascii numbers.
//The max number that this function can print should be 65,536.
//Source: http://homepage.cs.uiowa.edu/~jones/bcd/decimal.html
//If print_leading_zeros is set, print leading zeros.

//Uses 10 bytes of SRAM
void printNumber16(uint16_t n, uint8_t print_leading_zeros){
	uint8_t d4, d3, d2, d1, q;
	uint32_t d0;
	uint8_t counter = 0;
	//uint8_t i;
	
	//	This gives you the number of bits that are set in the
	//4 rightmost bits of n, since 0xF = 00001111.
	d0 = n & 0xF;
	//	Then you push the bits in the original number over 4 places and
	//repeat.  d1 now holds the next 4 bits that are either set of not in
	//the original number.
	d1 = (n>>4) & 0xF;
	//	Now repeat until all of the bits from the original number are in
	//8-bit variables.
	d2 = (n>>8) & 0xF;
	//set d3 to 0.
	d3 = (n>>12);
	
	
	d0 = 6*(d3 + d2 + d1) + d0;
	q = d0/10;
	d0 = d0%10;
	
	d1 = q + 9*d3 + 5*d2 + d1;
	if (d1!=0){
		q = d1/10;
		d1 = d1%10;
		
		d2 = q + 2*d2;
		if ((d2 != 0) || (d3 != 0)){
			q = d2/10;
			d2 = d2%10;
			
			d3 = q + 4*d3;
			if (d3!=0){
				q = d3/10;
				d3 = d3%10;
				
				d4 = q;
				if (d4!=0){
					transmitByte(d4 + '0');
				}
				
				if (d4 == 0 && print_leading_zeros == 1){
					transmitByte('0');
				}
				transmitByte(d3 + '0');
			}
			
			if (d3 == 0 && print_leading_zeros == 1){
				transmitByte('0');
				transmitByte('0');
				counter = 1;
			}
			transmitByte(d2 + '0');
		}
		
		if(((d2 == 0) || (d3 == 0)) && print_leading_zeros == 1 && counter != 1){
			transmitByte('0');
			transmitByte('0');
			transmitByte('0');
			counter = 1;
		}
		transmitByte(d1 + '0');
	}
	if(d1 == 0 && print_leading_zeros ==1 && counter != 1){
		transmitByte('0');
		transmitByte('0');
		transmitByte('0');
		transmitByte('0');
	}
	transmitByte(d0 + '0');

}


//This function converts binary to hexidecimal and prints them.
//Arguments are:  binary number, The number of leading zeros before the hex number.
//ex: if you say leading zeros is 5, then an output will be: 0x004aa.
//If endieness is set to 1, print numbers as little endianess, or where the 
//	least significant byte is in the smallest address.

//God this function is such a mess.
void printHex(uint16_t number, uint8_t *leading_zeros, uint8_t *endianess){
	//48 in ascii is 0
	char hex_index[] = {'0', '1', '2', '3', '4', '5', '6', '7', 
						'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	uint8_t purple[4];
	uint8_t i;	
	char output[4]; //The max size of a 16 bit number is 65536. 
	uint8_t counter = 0;
	
	purple[0] = number & 0xF;
	purple[1] = (number>>4) & 0xF;
	purple[2] = (number>>8) & 0xF;
	purple[3] = (number>>12) & 0xF;	
	
	for (i=1; i<5; i++){
		output[4-i] = hex_index[purple[i-1]];	

	}

	for (i=0; i<4; i++){
		if (output[i] == '0'){
			
			counter++;
		}
		else{
			break;
		}
	}
	//Prints the required number of 0's
	for(i=0; i<(((*leading_zeros) - (4- counter))); i++){
		transmitByte('0');
	}
	if (number == 0x10000){
		transmitByte('1');
		for (uint8_t i =0; i<4; i++){
			transmitByte('0');
		}
		return;
	}
	
	if ( (number == 0) && (*leading_zeros == 0) ){
		transmitByte('0');
	}
	
	if (*endianess == 1){
		for(i=4; i>0; i--){
			transmitByte(output[i-1]);
		}
	}
	else{
		for(i=0; i<4; i++){		
			if (counter > 0){
				counter--;
			}
			else{
				transmitByte(output[i]);
			}
		}
	}
					
}

//A similar god awful function for printing 32 bit numbers.
//
void printHex32(uint32_t number, uint8_t *leading_zeros, uint8_t *endianess){
	char hex_index[] = {'0', '1', '2', '3', '4', '5', '6', '7',
						'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	uint8_t purple[8];
	char output[8];
	uint8_t counter = 0;
	uint8_t i;
		
	if (number == 0){
		transmitByte('0');
		goto END_OF_PRINTHEX32; //Will refactor this function later.
	}	
	purple[0] = number & 0xF;
	purple[1] = (number>>4) & 0xF;
	purple[2] = (number>>8) & 0xF;
	purple[3] = (number>>12) & 0xF;
	
	purple[4] = (number>>16) & 0xF;
	purple[5] = (number>>20) & 0xF;
	purple[6] = (number>>24) & 0xF;
	purple[7] = (number>>28) & 0xF;
	
	
	for (i=1; i<9; i++){
		output[8-i] = hex_index[purple[i-1]];
	}

	for (i=0; i<8; i++){
		if (output[i] == '0'){			
			counter++;
		}
		else{
			break;
		}
	}
	//Prints the required number of 0's
	for(i=0; i<(((*leading_zeros) - (8 - counter))); i++){
		transmitByte('0');
	}

	if (*endianess == 1){
		for(i=4; i>0; i--){
			transmitByte(output[i-1]);
		}
	}
	else{
		for(i=0; i<8; i++){
			if (counter > 0){
				counter--;
			}
			else{
				transmitByte(output[i]);
			}
		}
	}
	END_OF_PRINTHEX32:;
	
}

////////////////////////////////////////////////////////////////////////////////
//SD Block Printing Functions///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//This function is used to output fancy, readable 512 bytes of blocks of data
//from the SD card.
//The first variable is the block address of the block.  The second argument is 512 bytes
//	previously malloced from reading the SD card.
void display_Block(uint32_t *block_address_offset, void* the_block){
	uint16_t i = 0;
	uint8_t places = 8;
	uint8_t endieness = 0;
	uint8_t places2 = 2;
	uint8_t *new_ptr = (uint8_t*) the_block;
	
	flashPrintf(SPACER_1);
	newLine();
	flashPrintf(WIDEN);
	newLine();
	flashPrintf(CONTINUE);
	USART_Buffer_Flush();
	receiveByte();
	newLine();
	flashPrintf(SPACER_2);
	newLine();
	
	flashPrintf(PSTR("	Block Address 0x"));
	printHex32(*block_address_offset, &endieness, &endieness);
	flashPrintf(PSTR(" hex offset"));
	
	newLine();
	newLine();
	flashPrintf(PSTR("           00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |"));
	newLine();
	flashPrintf(SPACER_3);
	newLine();

	//Output is like so:
	//printf("%08X | %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X | %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c %c |"
	for(i=0; i<512;i=i+16){
		
		printHex((uint16_t)i, &places, &endieness);		flashPrintf(DIVIDER);	
		printHex((uint16_t)( *new_ptr), &places2, &endieness);						flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+sizeof(uint8_t))), &places2, &endieness);		flashPrintf(SINGLE_SPACE);//1
		printHex((uint16_t)( *(new_ptr+2*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+3*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);//3
		printHex((uint16_t)( *(new_ptr+4*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+5*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);//5 
		printHex((uint16_t)( *(new_ptr+6*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+7*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);//7
		printHex((uint16_t)( *(new_ptr+8*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+9*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);//9
		printHex((uint16_t)( *(new_ptr+10*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+11*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);//11
		printHex((uint16_t)( *(new_ptr+12*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+13*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);//13
		printHex((uint16_t)( *(new_ptr+14*sizeof(uint8_t))), &places2, &endieness);	flashPrintf(SINGLE_SPACE);	printHex((uint16_t)( *(new_ptr+15*sizeof(uint8_t))), &places2, &endieness);	//15
		
		flashPrintf(DIVIDER);	
		
		transmitByte(formatChar(new_ptr)); flashPrintf(SINGLE_SPACE);						transmitByte(formatChar( (new_ptr+sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);
		transmitByte(formatChar((new_ptr+2*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);	transmitByte(formatChar( (new_ptr+3*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);
		transmitByte(formatChar((new_ptr+4*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);	transmitByte(formatChar( (new_ptr+5*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);
		transmitByte(formatChar((new_ptr+6*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);	transmitByte(formatChar( (new_ptr+7*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);
		transmitByte(formatChar((new_ptr+8*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);	transmitByte(formatChar( (new_ptr+9*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);
		transmitByte(formatChar((new_ptr+10*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);	transmitByte(formatChar( (new_ptr+11*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);
		transmitByte(formatChar((new_ptr+12*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);	transmitByte(formatChar( (new_ptr+13*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);
		transmitByte(formatChar((new_ptr+14*sizeof(uint8_t)))); flashPrintf(SINGLE_SPACE);	transmitByte(formatChar( (new_ptr+15*sizeof(uint8_t)))); 

		flashPrintf(DIVIDER);		

		newLine();
		new_ptr = new_ptr + (16*(sizeof (uint8_t)));
	}
	flashPrintf(SPACER_2);
	newLine();
	flashPrintf(CONTINUE);
	USART_Buffer_Flush();
	receiveByte();
	newLine();

}

//This function formats chars to be printed.
//	if the char is 00, then it returns a space char ' ',
//	if it isn't then it just casts the char and returns it.
char formatChar(uint8_t *purple){
	//These chars do not print as ascii chars.
	//	They are either for formatting or unicode chars.
	if (*purple <= 0x20 || *purple >= 0x7f){
		return ' ';
	}
	else{
		return  *purple;
	}
}

//converts a lower case ascii character to an uppercase letter:
uint8_t change_to_upper(uint8_t lower){
	lower = lower - 32;
	return lower;
}

//returns a one of the char is a lower case ascii char.
uint8_t is_lower(uint8_t my_char){
	if ( (my_char<123) && (my_char>96) ){//if lower case ascii
		return 1;
	}
	else{
		return 0;
	}
}