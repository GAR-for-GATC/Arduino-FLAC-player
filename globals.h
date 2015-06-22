#ifndef globals
#define globals

const char SPACER_1[56];
const char SPACER_2[96];
const char SPACER_3[96];
const char SPACER_4[70];
const char CONTINUE[29];
const char WIDEN[43];
const char READ_BLOCK_ERROR[36];
const char MALLOC_ERROR[25];
const char REALLOC_ERROR[19];
const char EXIT_PROG[29];
const char SINGLE_SPACE[4];
const char DIVIDER[6];
const char GENERAL_ERROR[34];
const char FAT32_ERROR[31];
const char FORWARD_SLASH[4];

#define Bit_shift16(ptr)			(uint16_t)(     ((uint16_t)*ptr) | (  (uint16_t)(*(ptr+1)) << 8  )    )
#define Bit_shift32(ptr)			(uint32_t)(	    ((uint32_t)*ptr) | (  (uint32_t)(*(ptr+1)) << 8  )	| (  (uint32_t)(*(ptr+2))  << 16  )  |  (  (uint32_t)(*(ptr+3)) << 24  )  )

#define ONEBYTE sizeof(unsigned char)



#endif
