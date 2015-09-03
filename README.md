# Arduino-FLAC-player

The goal of this project uses an ATMega328P to play FLAC lossless audio files from a
microSD card.  A 74HC450N Hex converter is used to shift the control chip's 5V logic level
to a 3.3V logic level.  SPI is used to interface with the SD card.  The program is written
in C in Atmel Studio 6.2. 
