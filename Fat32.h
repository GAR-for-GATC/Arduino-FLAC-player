
#ifndef Fat32
#define Fat32

#define ONEBYTE sizeof(unsigned char)

//This struct holds blocks to read from or to be written to the SD card.
typedef struct{
	unsigned char single_block[512];
}	Single_Block;

//This struct holds a partition info taken from the Master Boot Record.
//Uses 16 bytes.
typedef struct{
	unsigned char boot_flag;
	unsigned char CHS_Begin[3];
	unsigned char type_code; 	//is 0x0B or 0x0C for Fat32
	unsigned char CHS_End[3];
	unsigned long LBA_Begin; //This is where the partition volume ID starts, which
							 //	describes the partition.
	unsigned long Number_of_Sectors;
	
}	Single_Partition;

typedef struct
{
	unsigned short 	Bytes_Per_Sector;
	unsigned char 	Sectors_Per_Cluster;
	unsigned short 	Number_of_Reserved_Sectors;
	unsigned char 	Number_of_FATs;
	unsigned long 	Sectors_Per_FAT;
	unsigned long 	Root_Directory_First_Cluster;
	unsigned long	volume_ID;
	
	unsigned long 	fat_begin_lba;			//fat_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors
	unsigned long	fat_end_lba;			//This is where the FAT ends, same as  = (cluster_begins_lba - fat_begin_lba)/2
	unsigned long 	cluster_begin_lba;		//cluster_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors + (Number_of_FATs * Sectors_Per_FAT)
	//unsigned long	data_start_sector		//data_start_sector = data_start_sector + file_size
	
	unsigned long	current_data_start; 	// Data start sector
	unsigned long	current_data_end;		// Data end sector
	unsigned long	file_pointer; 			// File R/W pointer
	unsigned long	file_size;				// File size
	unsigned long	file_start_cluster;		// File start cluster
	unsigned long	current_file_cluster;	// This is the Cluster that is currently being used.  Pass this to Fat_next_block to get the
											//	the next cluster in the FAT linked list.  Pass it to FAT_Rewind to get the previous cluster
											//	in the FAT linked list.  Changed frequently in Fat_next_block and Fat_rewind.
	
	
	unsigned char status_flag;
} Fat_Info;


void Fat32_Init(Fat_Info *current_file_info, Single_Block* Read_and_Write);
void get_first_partition_info(Single_Partition *return_partition, Single_Block *Read_and_Write);
void get_volume_information(Fat_Info *current_file_info, Single_Partition *current_partition, Single_Block *Read_and_Write);

void print_SDcard_detailsFat32(Fat_Info *current_file_info, Single_Block *Read_and_Write );
uint32_t fat_next_block (Fat_Info *current_file_info, Single_Block *FAT_read);
uint8_t checkSumChecker(uint8_t *checksum_entry);
uint32_t FAT_Rewind(Fat_Info *input_struct, Single_Block *FAT_read);

void print_long_root_contents(Fat_Info *current_file_info, Single_Block *Read_and_Write);
void parse_print_short(Fat_Info *current_file_info, Single_Block *Read_and_Write, uint16_t *a);
void parse_print_long(Fat_Info *current_file_info, Single_Block *Read_and_Write, uint16_t *d, uint8_t *number_of_32_bytes_printed);

void load_file(const char *file_name, Fat_Info *current_file_info, Single_Block *Read_and_Write);
void display_FAT (uint32_t address, Fat_Info *current_file_info, Single_Block *FAT_read);


#endif