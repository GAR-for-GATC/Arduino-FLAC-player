////////////////////////////////////////////////////////////////////////////////
//Main Program//////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#include "include.h"

	
int main(void){

	
	Init_USART();
	Init_SPI();
	Init_SD_Card();
	
	Fat_Info current_file_info = {0};
	Single_Block Read_and_Write = {{0}};
		
	Fat32_Init(&current_file_info, &Read_and_Write);
	print_SDcard_detailsFat32(&current_file_info, &Read_and_Write );
	
	load_file("example file.flac", &current_file_info, &Read_and_Write);

	//displays the first block of "example file.flac"
	read_one_block(current_file_info.file_start_cluster, &Read_and_Write);
	display_Block( &(current_file_info.file_start_cluster), &Read_and_Write);
	

	
	return 0;
}

