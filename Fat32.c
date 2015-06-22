#include "include.h"


//This function checks the FAT32 file system and retrieves general
//	info about it.
void Fat32_Init(Fat_Info *current_file_info, Single_Block* Read_and_Write){
		
	Single_Partition first_partition;
	
	get_first_partition_info(&first_partition, Read_and_Write);
	get_volume_information(current_file_info, &first_partition, Read_and_Write);
	
}

////////////////////////////////////////////////////////////////////////////////
//Functions Used for FAT32 Initialization///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//	Parses the Master Boot Record (the first 512 bytes) on the SD card, and retrieves information
// about the first partition.  The other 3 partitions are ignored.
void get_first_partition_info(Single_Partition *return_partition, Single_Block *Read_and_Write){
	unsigned char i;
	
	read_one_block(0, Read_and_Write); 	//Read the master boot record.

	char *ptr = (char*) return_partition;
	uint8_t* block_ptr =(uint8_t *) Read_and_Write;	
	for (i=0; i<16; i++){		
		*ptr = *(block_ptr + 446 + i);	//The first 446 bytes in the MBR are boot code, the next 64 bytes
		++ptr;							//	are for partition data.  Every 16 bytes describes one partition.
	}
}

//This function retrieves the information about a single partition.
void get_volume_information(Fat_Info *current_file_info, Single_Partition *current_partition, Single_Block *Read_and_Write){
	uint8_t* block_ptr =(uint8_t *) Read_and_Write;
	
	if ((current_partition->type_code != 0x0B) && (current_partition->type_code != 0x0C)){
		flashPrintf(PSTR("This partition is not in Fat32 format. Type code not 0x0B or 0x0C."));
		newLine();
		flashPrintf(EXIT_PROG);
		exit(1);
	}

	read_one_block((uint32_t)(current_partition->LBA_Begin), Read_and_Write);	
	
	//Parse Volume ID block into struct.
	current_file_info->Bytes_Per_Sector = Bit_shift16((block_ptr + 0x0B));
	current_file_info->Sectors_Per_Cluster = *(block_ptr + 0x0D);
	current_file_info->Number_of_Reserved_Sectors = Bit_shift16((block_ptr + 0x0E));
	current_file_info->Number_of_FATs = *(block_ptr + 0x10);
	current_file_info->Sectors_Per_FAT = Bit_shift32((block_ptr + 0x24));
	current_file_info->Root_Directory_First_Cluster = Bit_shift32((block_ptr + 0x2C));

	//The next two numbers are needed to access the FAT table and the root directory respectively.
	
	//fat_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors
	current_file_info->fat_begin_lba = (uint32_t)(current_partition->LBA_Begin) + (uint32_t)current_file_info->Number_of_Reserved_Sectors;

	//cluster_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors + (Number_of_FATs * Sectors_Per_FAT)
	current_file_info->cluster_begin_lba = (uint32_t) current_file_info->Number_of_FATs * (uint32_t) current_file_info->Sectors_Per_FAT;	
	current_file_info->cluster_begin_lba = current_file_info->cluster_begin_lba + (uint32_t)(current_partition->LBA_Begin) + (uint32_t)(current_file_info->Number_of_Reserved_Sectors);
	
	//(cluster_begins_lba - fat_begin_lba)/2
	current_file_info->fat_end_lba = (uint32_t)((current_file_info->cluster_begin_lba - current_file_info->fat_begin_lba )/2);
	
	current_file_info->volume_ID = current_partition->LBA_Begin;
	
}

////////////////////////////////////////////////////////////////////////////////
//Functions Used for FAT32 Linked List Navigation///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//Give this function the address of the current 512 block in memory.
//	Then this function will go into the File Allocation Table and find the next
//	block of the file.  If the current block is greater than 0x0FFFFFFF, or
//	FF FF FF 0F (because of little endieness), then return 0, which means that
//	then End of File has been reached.
uint32_t fat_next_block (Fat_Info *current_file_info, Single_Block *FAT_read){
	uint32_t fat_address = 0;
	uint32_t return_address = 0;
	uint32_t new_FAT = 0;	
	uint16_t number_of_entries = 0;

	uint8_t* block_ptr =(uint8_t *) FAT_read;

	//Get the relevant FAT table and FAT entry from the current cluster number.
	fat_address = (current_file_info->current_file_cluster) - (current_file_info->cluster_begin_lba);
	fat_address = fat_address * 4;
	new_FAT =  (fat_address / current_file_info->Bytes_Per_Sector);
	number_of_entries = fat_address  % current_file_info->Bytes_Per_Sector;
	new_FAT = new_FAT + current_file_info->fat_begin_lba;
	
	
	read_one_block(new_FAT, FAT_read);
	
	number_of_entries = number_of_entries + 8; //because the very first 64 bits are reserved.
	
	return_address = Bit_shift32((block_ptr + number_of_entries));

	if(return_address > 0x0EFFFFFF){//If the end of the file has been reached.
		return 0;
	}
	
	return_address = return_address & 0x0FFFFFFF;
	return_address =  return_address - 2;
	return_address = return_address + current_file_info->cluster_begin_lba;

	return return_address;
}

//This function takes an address from the data region and returns the
//	previous cluster address.  The function works by going to the
//	first address of the file and goes to the previous cluster
//	using the fat_next_block function.
//
//file_start = the first cluster address of each file.
//address = the current address.  The function returns the cluster
//			address that comes previously in the linked list in the
//			FAT.
uint32_t FAT_Rewind(Fat_Info *input_struct, Single_Block *FAT_read){   
	uint32_t previous_address = 0;
	uint32_t current_address = input_struct->current_file_cluster;

	input_struct->current_file_cluster = input_struct->current_data_start;
	
	//loop from the first cluster of the FAT linked list to the current cluster.
	while(input_struct->current_file_cluster != current_address ){

		previous_address = input_struct->current_file_cluster;

		input_struct->current_file_cluster = fat_next_block(input_struct, FAT_read);
	}
	return previous_address;
}

//Calculate the checksum values from the first 11 entries from a short file entry.
uint8_t checkSumChecker(uint8_t *checksum_entry){
	uint8_t p;
	uint8_t sum = 0;
	
	for(p=0; p<11; p++){
		sum = ((sum & 1) ? 0x80 : 0) + (sum>>1) + *checksum_entry++;
	}
	return sum;
}

////////////////////////////////////////////////////////////////////////////////
//Functions Used for Reading Files//////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//This function searches the root directory and puts the file info into the
//	"current_file_info" struct.
//load_file("01. &Z.FLAC", &current_file_info, &Read_and_Write);
void load_file(const char *file_name, Fat_Info *current_file_info, Single_Block *Read_and_Write){///
	//	Compare the first 5 characters of the short file name in the root directory to the first
	//five characters from from the given file name.  If the short file name has a "~" in it,
	//only compare the letters before the "~".  For example, the short file name for "01. &Z.FLAC"
	//is "01&Z~1  FLA", so only "01&Z" will be compared to the characters in "01. &Z.FLAC".  The
	//function should ignore spaces and periods in the user given file name.
	//	If the short entry matches, then get the long entry and compare the whole long entry to
	//the user given file name.  If the names match, then enter the file information into the
	//current_file_info struct.  If no short matches are found with matching long entry names,
	//then print the string "Given file not found" and end the function.
	
	
	uint8_t *block_ptr =(uint8_t *) Read_and_Write;
	uint8_t * file_ptr =(uint8_t *) file_name;
	uint8_t short_name[5] = {0};
	uint8_t parsed_long[5] = {0};
	uint8_t checksum_entries[11] = {0};
		
	uint16_t	a; 
	uint16_t	e;
	uint8_t		b; 
	uint8_t		c=0;	//This variable counts the number of characters that are in the short name before the tilde.
	uint8_t		d=0;
	uint8_t		flag1;	//	This is set if there's a char in the short entry and a 
						//char in the long entry that's different.
	uint8_t		flag2 = 0;	//	This is set if the FAT has to be rewound.
	uint8_t		long_sim_flag;	//This is set if a long entry is different than the user generated entry.
	
	uint8_t		number_of_32_bytes_interated; 
	uint8_t		counter_thingy = 0;
	uint8_t		checksum_number;
	
	current_file_info->current_file_cluster = current_file_info->cluster_begin_lba; //Because the current cluster is root.
	
	while(current_file_info->current_file_cluster != 0){//Loop through root to find all short entries. ///
		read_one_block(current_file_info->current_file_cluster, Read_and_Write);
		
		
		for(a=0; a<512; a=a+32){//Loop through each block looking for short names.///
			
			if (	!( ( *(block_ptr + (a*ONEBYTE) + 11)  ) & 0x0F )	&& //if a valid short entry is found.///
						(*(block_ptr + (a*ONEBYTE)) != 0x05)			&&
						(*(block_ptr + (a*ONEBYTE))	!= 0xE5) 			&&
						(*(block_ptr + (a*ONEBYTE))	!= 0x00)			){
					
				//get bytes of the short name until either the 5th byte is reached or it is a "~".
				c=0;
				for (b=0; b<5; b++){//0x7E is "~" in ASCII
					if ( *(block_ptr + (a*ONEBYTE) + b) != 0x7E ){
						short_name[b] = *(block_ptr + (a*ONEBYTE) + b);	
						//transmitByte( short_name[b] );
						c++;
					}
				}
				//	Get the corresponding number of characters from the user generated entry.
				//Don't accept periods or spaces. 0x20 are spaces, and 0x2E are periods.
				b=0; //Reusing b and d
				d=0;
				flag1 = 0;
				while(b<c){
					if( (*(file_ptr + b + d) != 0x20) && (*(file_ptr + b + d) != 0x2E) ){
						parsed_long[b] = *(file_ptr + b + d);
						if (is_lower(parsed_long[b])){
							parsed_long[b] = change_to_upper(parsed_long[b]);
						}
						b++;
					}					
					else{
						d++;
					}					
				}
				//Reusing b.
				for(b=0; b<c; b++){
					if(parsed_long[b] != short_name[b]){
						flag1++;
					}
				}
				
				//	If the short entry is the same as the user entered entry,
				//check the long entry.
				number_of_32_bytes_interated = 0;
				long_sim_flag = 0;
				if(flag1 == 0){ ///
					for(b=0; b<11; b++){//Get the checksum number
						checksum_entries[b] = *(block_ptr + a + b);
					}
					checksum_number = checkSumChecker(checksum_entries);
					
					e = a;	//Extracts a, so it can be manipulated
					b = 0;	//reusing b
					number_of_32_bytes_interated = 1;
					flag2 = 0;
					while(b == 0){ //loop until the checksum number of a long entry is different.
						
						if((e-32) > 512){//if the previous entry is in a previous block
							current_file_info->current_data_start = current_file_info->cluster_begin_lba;
							current_file_info->current_file_cluster = FAT_Rewind(current_file_info, Read_and_Write);
							read_one_block (current_file_info->current_file_cluster, Read_and_Write);
							e = 512; //Puts the entry pointer at the end of the block
							flag2 = 1;
						}
						
						if(checksum_number == *(  ((block_ptr + e) - (32*number_of_32_bytes_interated)) + 13 )){ ///
							
							for(d=1; d<11; d=d+2){
								if( *((block_ptr + e - (32 * (number_of_32_bytes_interated)))+ d) == 0xFF){
									goto END_OF_CHECK;
								}
								if(*((block_ptr + e - (32 * (number_of_32_bytes_interated)))+ d) != *(file_ptr + counter_thingy) ){
									long_sim_flag++;
								}	
								counter_thingy++;						
							}	
							if( *((block_ptr + e - (32 * (number_of_32_bytes_interated))) + 14) == 0xFF){
								goto END_OF_CHECK;
							}
							if( *((block_ptr + e - (32 * (number_of_32_bytes_interated)))+ 14) != *(file_ptr + counter_thingy) ){
								long_sim_flag++;
							}
							counter_thingy++;
							for(d=16; d<25; d=d+2){
								if( *((block_ptr + e - (32 * (number_of_32_bytes_interated)))+ d) == 0xFF){
									goto END_OF_CHECK;
								}
								if( *((block_ptr + e - (32 * (number_of_32_bytes_interated)))+ d) != *(file_ptr + counter_thingy) ){
									long_sim_flag++;
								}
								counter_thingy++;
							}
							for(d=28; d<32; d=d+2){
								if( *((block_ptr + e - (32 * (number_of_32_bytes_interated)))+ d) == 0xFF){
									goto END_OF_CHECK;
								}
								if( *((block_ptr + e - (32 * (number_of_32_bytes_interated)))+ d) != *(file_ptr + counter_thingy) ){
									long_sim_flag++;
								}
								counter_thingy++;
							}
							END_OF_CHECK:;
							number_of_32_bytes_interated++;
						}//End of if 
						
						//	If both the long entry and the user entered entries are the same, then load the information into the 
						//the current_file_info struct.
						if(long_sim_flag == 0){
							if (flag2 == 1 ){//Go to the place where the short entry is located.
								current_file_info->current_file_cluster = fat_next_block(current_file_info, Read_and_Write);
								read_one_block (current_file_info->current_file_cluster, Read_and_Write);
								//Get the first file cluster from the short entry.
								current_file_info->file_start_cluster = ( (uint32_t)(*(block_ptr + a + 26))			|
																		 ((uint32_t)(*(block_ptr + a + 27)) << 8)	|
																		 ((uint32_t)(*(block_ptr + a + 20)) << 16)	|
																		 ((uint32_t)(*(block_ptr + a + 21)) << 24)
																		) + (current_file_info->cluster_begin_lba - 2);
								return; 
							}
							else{//If the FAT doesn't have to be rewound.
								current_file_info->file_start_cluster = (  (uint32_t)(*(block_ptr + a + 26))			|
																		  ((uint32_t)(*(block_ptr + a + 27)) << 8)	|
																		  ((uint32_t)(*(block_ptr + a + 20)) << 16)	|
																		  ((uint32_t)(*(block_ptr + a + 21)) << 24)
																		 ) + (current_file_info->cluster_begin_lba - 2);
								return;
							}
						}
						
						
						//If a long entry is found where the checksum number is different
						if(checksum_number != *(  ((block_ptr + e) - (32*number_of_32_bytes_interated)) + 13 )  ){

							b++;
							if (flag2 == 1 ){
								current_file_info->current_file_cluster = fat_next_block(current_file_info, Read_and_Write);
								read_one_block (current_file_info->current_file_cluster, Read_and_Write);
								flag2 = 0;
							}
						}	
						
					}//End of while(b==0)
				}//End of if(flag1 == 0)				
				
			}//End of If a valid short entry is found ///
		}//End of for(a=0; a<512; a=a+32)///
	current_file_info->current_file_cluster = fat_next_block(current_file_info, Read_and_Write);
	}//End of while(current_file_info->current_file_cluster != 0) ///
}///
	
////////////////////////////////////////////////////////////////////////////////
//Functions Used to Display File and Directory Information on the Terminal//////
////////////////////////////////////////////////////////////////////////////////

//Prints all of the details of the SD card.
void print_SDcard_detailsFat32(Fat_Info *current_file_info, Single_Block *Read_and_Write ){
	uint8_t a = 0;
	uint8_t green = 0;
	
	//Display Master Boot Record//////////////////////////////////////////////////////////
	do{
		flashPrintf(PSTR("  Would you like to display the Master Boot Record? (y/n): "));
		USART_Buffer_Flush();
		green = receiveByte();
		USART_Buffer_Flush();
		receiveByte();
	}while((green != 'y') && (green !='n'));
	if (green == 'y'){
		//Single_Block master_boot_record;
		uint32_t b = 0;
		read_one_block(a, Read_and_Write);
		display_Block(&b, Read_and_Write);
	}
	
	//Display First Partition Volume ID///////////////////////////////////////////////////
	do{
		flashPrintf(PSTR("  Would you like to display the Partition Volume ID? (y/n): "));
		USART_Buffer_Flush();
		green = receiveByte();
		USART_Buffer_Flush();
		receiveByte();
	}while((green != 'y') && (green !='n'));
	if (green == 'y'){
		//Single_Block volumeID;
		read_one_block(current_file_info->volume_ID, Read_and_Write);
		display_Block(&(current_file_info->volume_ID), Read_and_Write);
	}
	//Display Partition Info/////////////////////////////////////////////////////////////
	do{
		flashPrintf(PSTR("  Would you like to display the Partition Volume Info? (y/n): "));
		USART_Buffer_Flush();
		green = receiveByte();
		USART_Buffer_Flush();
		receiveByte();
	}while((green != 'y') && (green !='n'));
	if (green == 'y'){
		flashPrintf(PSTR("Bytes Per Sector: "));
		printNumber16((uint16_t)current_file_info->Bytes_Per_Sector, 0);
		newLine();
		flashPrintf(PSTR("Sectors per Cluster: "));
		printNumber((uint8_t)current_file_info->Sectors_Per_Cluster);
		newLine();
		flashPrintf(PSTR("Number of Reserved Sectors: "));
		printNumber16((uint16_t)current_file_info->Number_of_Reserved_Sectors, 0);
		newLine();
		flashPrintf(PSTR("Number of FATs: "));
		printNumber((uint8_t)current_file_info->Number_of_FATs);
		newLine();
		flashPrintf(PSTR("Sectors per FAT: "));
		printNumber16((uint16_t)current_file_info->Sectors_Per_FAT, 0);
		newLine();
		flashPrintf(PSTR("Root Directory First Cluster: 0x"));
		printHex((uint16_t)current_file_info->Root_Directory_First_Cluster, &a, &a);
		newLine();
		newLine();
		flashPrintf(PSTR("Beginning of FAT, LBA address is: 0x"));
		printHex((uint16_t) current_file_info->fat_begin_lba ,&a, &a);
		newLine();
		flashPrintf(PSTR("End of FAT copy 1: 0x"));
		printHex32((uint32_t) current_file_info->fat_end_lba, &a, &a);
		newLine();
		flashPrintf(PSTR("Cluster begin (Root directory), LBA address is: 0x"));
		printHex32((uint32_t) current_file_info->cluster_begin_lba, &a, &a);
		newLine();
	}
	//Display First FAT entry////////////////////////////////////////////////////////////
	do{
		flashPrintf(PSTR("  Would you like to display first FAT entry? (y/n): "));
		USART_Buffer_Flush();
		green = receiveByte();
		USART_Buffer_Flush();
		receiveByte();
	}while((green != 'y') && (green !='n'));
	if (green == 'y'){

		read_one_block(current_file_info->fat_begin_lba, Read_and_Write);
		display_Block(&(current_file_info->fat_begin_lba), Read_and_Write);
	}
	
	//Display Root Directory/////////////////////////////////////////////////////////////
	do{
		flashPrintf(PSTR("  Would you like to display the Root Directory? (y/n): "));
		USART_Buffer_Flush();
		green = receiveByte();
		USART_Buffer_Flush();
		receiveByte();
	}while((green != 'y') && (green !='n'));
	if (green == 'y'){
		
		current_file_info->current_file_cluster = current_file_info->cluster_begin_lba;
		
		read_one_block(current_file_info->current_file_cluster, Read_and_Write);
		display_Block(&(current_file_info->current_file_cluster), Read_and_Write);
		
		current_file_info->current_file_cluster = fat_next_block (current_file_info, Read_and_Write);
		//Loop until the end of the Root Directory is reached.
		while(current_file_info->current_file_cluster != 0){
			read_one_block(current_file_info->current_file_cluster, Read_and_Write);
			display_Block(&(current_file_info->current_file_cluster), Read_and_Write);
			current_file_info->current_file_cluster = fat_next_block (current_file_info, Read_and_Write);
		}
	}
	
	//Display Root Directory Files///////////////////////////////////////////////////////
	do{
		flashPrintf(PSTR("  Would you to like to display Root Directory Files? (y/n): "));
		USART_Buffer_Flush();
		green = receiveByte();
		USART_Buffer_Flush();
		receiveByte();
	}while((green != 'y') && (green !='n'));
	if (green == 'y'){
		current_file_info->current_file_cluster = current_file_info->cluster_begin_lba;
		current_file_info->current_data_start = current_file_info->cluster_begin_lba;

		print_long_root_contents(current_file_info, Read_and_Write);
		
	}
}

//This function finds and prints the long entries of all of the entries in root.  
void print_long_root_contents(Fat_Info *current_file_info, Single_Block *Read_and_Write){
	uint8_t flag = 0;
	uint16_t a, d;
	uint8_t b, c;
	
	//checksum buffers
	uint8_t checksum_entries[11] = {0};
	uint8_t checksum_number;

	uint8_t number_of_32_bytes_printed = 0;
	
	//Loop through each 512 byte block until it reaches the end of the linked list, where
	//	fat_next_block returns 0.
	while(current_file_info->current_file_cluster != 0){
		read_one_block(current_file_info->current_file_cluster, Read_and_Write);

		uint8_t* block_ptr =(uint8_t *) Read_and_Write;
		///////////////////
		for(a=0; a<512; a=a+32){

			//If a valid short entry is found
			if (	!( ( *(block_ptr + (a*ONEBYTE) + 11)  ) & 0x0F )	&&
			(*(block_ptr + (a*ONEBYTE)) != 0x05)				&&
			(*(block_ptr + (a*ONEBYTE))	!= 0xE5) 			&&
			(*(block_ptr + (a*ONEBYTE))	!= 0x00)			){
				
				//Parse and print short entry.////////////////
				parse_print_short(current_file_info, Read_and_Write, &a);
				
				for(b=0; b<11; b++){//Get the checksum number
					checksum_entries[b] = *(block_ptr + a + b);
				}
				checksum_number = checkSumChecker(checksum_entries);				
				
				d = (signed short) a; //Extracts a, so it can be manipulated
				c=0;
				number_of_32_bytes_printed = 1;
				flag = 0;
				while(c == 0){
					
					if((d-32) > 512){//if the previous entry is in a previous block
						current_file_info->current_file_cluster = FAT_Rewind(current_file_info, Read_and_Write);
						read_one_block (current_file_info->current_file_cluster, Read_and_Write);
						d = 512; //(512-32), Puts the entry pointer at the end of the block
						flag = 1;
					}
					//If the checksum numbers are the same, meaning that this 32 bytes
					//	are part of the current long entry.
					if(checksum_number == *(  ((block_ptr + d) - (32*number_of_32_bytes_printed)) + 13 )){
						parse_print_long(current_file_info, Read_and_Write, &d, &number_of_32_bytes_printed);
						number_of_32_bytes_printed++;
					}
					
					
					if(checksum_number != *(  ((block_ptr + d) - (32*number_of_32_bytes_printed)) + 13 )  ){
						c++;
						if (flag == 1 ){
							current_file_info->current_file_cluster = fat_next_block(current_file_info, Read_and_Write);
							read_one_block (current_file_info->current_file_cluster, Read_and_Write);
							flag = 0;
						}
					}
				}//End of while loop
				newLine();				
			}//end of If statement
		}//end of For loop////////////////		

	current_file_info->current_file_cluster = fat_next_block(current_file_info, Read_and_Write);
	}// end of while loop
	
}

//This function parses and prints the details of a short entry
void parse_print_short(Fat_Info *current_file_info, Single_Block *Read_and_Write, uint16_t *a){
	uint8_t i;
	uint8_t z = 0;
	uint32_t bytes;
	uint8_t *block_ptr = (uint8_t *) Read_and_Write;
	
	flashPrintf(SPACER_4); newLine();
	//Short name////////////
	flashPrintf(PSTR("Short name:                                     "));
	for (i=0; i<11; i++){
		transmitByte( *(block_ptr + *a + i) );
	}
	newLine();
	//Date Created/Time Created////////////
	//printHex((uint16_t) *(block_ptr + *a + 17), &z, &z); newLine();
	flashPrintf(PSTR("Date/Time Created (DD/MM/YEAR, HH:MM:SS):       "));
	printNumber( (*(block_ptr + *a + 16)) & 0b00011111  );//days
	flashPrintf(FORWARD_SLASH);
	printNumber( (((*(block_ptr + *a + 17)) & 0b00000001) << 4) |
	(((*(block_ptr + *a + 16)) & 0b11100000) >> 5)	);//months
	flashPrintf(FORWARD_SLASH);
	printNumber( 20 );
	printNumber( (80 + (((*(block_ptr + *a + 17)) & 0b11111110) >> 1)) - 100 );//years
	flashPrintf(PSTR(", "));
	printNumber( ((*(block_ptr + *a + 15)) & 0b11111000) >> 3 );//hours, last 5 bits
	flashPrintf(PSTR(":"));
	printNumber( (((*(block_ptr + *a + 14)) & 0b11100000) >> 5) |
	(((*(block_ptr + *a + 15)) & 0b00000111) << 3)	);//minutes, middle 6 bits
	flashPrintf(PSTR(":"));
	printNumber( (*(block_ptr + *a + 14) & 0b00011111 ));//seconds, lower 5 bits
	newLine();
	
	//Last Date Accessed//////
	flashPrintf(PSTR("Date Last Accessed (DD/MM/YEAR):                "));
	printNumber( (*(block_ptr + *a + 18)) & 0b00011111  );//days
	flashPrintf(FORWARD_SLASH);
	printNumber( (((*(block_ptr + *a + 19)) & 0b00000001) << 4) |
	(((*(block_ptr + *a + 18)) & 0b11100000) >> 5)	);//months
	flashPrintf(FORWARD_SLASH);
	printNumber( 20 );
	printNumber( (80 + (((*(block_ptr + *a + 19)) & 0b11111110) >> 1)) - 100 );//years
	newLine();

	//Last Date Modified/Time Modified///// 22/23 24/25
	flashPrintf(PSTR("Date/Time Last Modified (DD/MM/YEAR, HH:MM:SS): "));
	printNumber( (*(block_ptr + *a + 24)) & 0b00011111  );//days
	flashPrintf(FORWARD_SLASH);
	printNumber( (((*(block_ptr + *a + 25)) & 0b00000001) << 4) |
	(((*(block_ptr + *a + 24)) & 0b11100000) >> 5)	);//months
	flashPrintf(FORWARD_SLASH);
	printNumber( 20 );
	printNumber( (80 + (((*(block_ptr + *a + 25)) & 0b11111110) >> 1)) - 100 );//years
	flashPrintf(PSTR(", "));
	printNumber( ((*(block_ptr + *a + 23)) & 0b11111000) >> 3 );//hours, last 5 bits
	flashPrintf(PSTR(":"));
	printNumber( (((*(block_ptr + *a + 22)) & 0b11100000) >> 5) |
	(((*(block_ptr + *a + 23)) & 0b00000111) << 3)	);//minutes, middle 6 bits
	flashPrintf(PSTR(":"));
	printNumber( (*(block_ptr + *a + 22) & 0b00011111 ));//seconds, lower 5 bits
	newLine();
	
	//First Cluster///
	flashPrintf(PSTR("Starting Cluster Number is:                     0x"));
	printHex32(	( (uint32_t)(*(block_ptr + *a + 26))			|
				 ((uint32_t)(*(block_ptr + *a + 27)) << 8)	|
				 ((uint32_t)(*(block_ptr + *a + 20)) << 16)	|
				 ((uint32_t)(*(block_ptr + *a + 21)) << 24)
				) +  (current_file_info->cluster_begin_lba - 2)
	, &z, &z);
	newLine();
	
	//File Size///////
	//1 MB = 1048576 bytes
	flashPrintf(PSTR("File Length (Megabytes):                        "));
	bytes = Bit_shift32((block_ptr + *a + 28));
	bytes = bytes / 1048576;
	printNumber16((uint16_t)bytes, 0);

	bytes = Bit_shift32((block_ptr + *a + 28));
	bytes = bytes % 1048576;
	flashPrintf(PSTR(" and "));
	printNumber16((uint16_t)bytes, 0);
	flashPrintf(PSTR(" bytes"));
	newLine();
}

//This function parses and prints the details of a long entry
void parse_print_long(Fat_Info *current_file_info, Single_Block *Read_and_Write, uint16_t *d, uint8_t *number_of_32_bytes_printed){
	uint8_t i;
	uint8_t *block_ptr = (uint8_t *) Read_and_Write;
	
	
	if (*number_of_32_bytes_printed == 1){
		flashPrintf(PSTR("Long Name:                                      "));
	}
	
	for(i=1; i<11; i=i+2){
		if( *((block_ptr + *d - (32 * (*number_of_32_bytes_printed)))+ i) == 0xFF){
			return;
		}
		printString((const char*) ((block_ptr + *d - (32 * (*number_of_32_bytes_printed)))+ i) );
	}

	if( *((block_ptr + *d - (32 * (*number_of_32_bytes_printed))) + 14) == 0xFF){
		return;
	}
	printString((const char*) ((block_ptr + *d - (32 * (*number_of_32_bytes_printed)))+ 14) );

	for(i=16; i<25; i=i+2){
		if( *((block_ptr + *d - (32 * (*number_of_32_bytes_printed)))+ i) == 0xFF){
			return;
		}
		printString((const char*) ((block_ptr + *d - (32 * (*number_of_32_bytes_printed)))+ i) );
	}
	for(i=28; i<32; i=i+2){
		if( *((block_ptr + *d - (32 * (*number_of_32_bytes_printed)))+ i) == 0xFF){
			return;
		}
		printString((const char*) ((block_ptr + *d - (32 * (*number_of_32_bytes_printed)))+ i) );
	}
	
}

//Displays the current FAT that a given cluster address is in.
void display_FAT (uint32_t address, Fat_Info *current_file_info, Single_Block *FAT_read){
	uint32_t fat_address = 0;
	//uint32_t return_address = 0;
	uint32_t new_FAT = 0;
	
	//uint16_t number_of_entries;
	//Single_Block single_FAT_block;
	//uint8_t* block_ptr =(uint8_t *) FAT_read;
	
	//uint8_t g = 0;

	//These operations get the relevant FAT table and FAT entry from the current cluster number.
	//
	fat_address = (address) - (current_file_info->cluster_begin_lba);
	fat_address = fat_address * 4;
	new_FAT =  (fat_address / current_file_info->Bytes_Per_Sector);
	//number_of_entries = fat_address  % current_file_info->Bytes_Per_Sector;
	new_FAT = new_FAT + current_file_info->fat_begin_lba;
	
	
	read_one_block(new_FAT, FAT_read);
	display_Block(&new_FAT, FAT_read);
	return;
}
