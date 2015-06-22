#include "include.h"

//These are all spacers used for showing stuff on the terminal and making it all formatted and pretty.
//	PROGMEM means that the string is stored in program memory and not SRAM.
//PSTR is used all over the code to put strings in Flash memory instead of SRAM.

const char SPACER_1[] PROGMEM = "/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\0";
const char SPACER_2[] PROGMEM = "/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\0";
const char SPACER_3[] PROGMEM = "---------------------------------------------------------------------------------------------\0";
const char SPACER_4[] PROGMEM = "------------------------------------------------------------------\0";
const char CONTINUE[] PROGMEM = "Press [Enter] to continue...\0";
const char WIDEN[] PROGMEM = "Widen terminal for proper text formating.\0\0";
const char READ_BLOCK_ERROR[] PROGMEM = "Unable to read block.  Error code: \0";
const char MALLOC_ERROR[] PROGMEM = "Unable to malloc memory.\0";
const char REALLOC_ERROR[] PROGMEM = "Unable to Realloc.\0";
const char EXIT_PROG[] PROGMEM = "The program will now exit...\0";
const char SINGLE_SPACE[] PROGMEM = " \0";
const char DIVIDER[] PROGMEM = " | \0";
const char GENERAL_ERROR[] PROGMEM = "General error, Error code: \0";
const char FAT32_ERROR[] PROGMEM = "Error, file system not Fat32.\0";
const char FORWARD_SLASH[] PROGMEM = "/\0";