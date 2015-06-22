#ifndef SD_comm
#define SD_comm



uint8_t Send_SD_command(uint8_t command_number, uint32_t arg, uint8_t is_init);
uint64_t Send_SD_command_long(uint8_t command_number, uint32_t arg);
void Init_SD_Card(void);
void read_one_block(uint32_t address, void* hold_sector);


#endif