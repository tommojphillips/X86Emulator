// input.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef _CPU_INPUT_H
#define _CPU_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#define CPU_INPUT
//#define OUTPUT_BUFFER
#define OUTPUT_MNEMONIC

typedef struct _BREAKPOINT {
	bool set;
	uint32_t address;
} BREAKPOINT;

int input_loop(); // input.c

#endif
