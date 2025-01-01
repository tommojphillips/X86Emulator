// cpu_eflags.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef EFLAGS_H
#define EFLAGS_H

#include "type_defs.h"

typedef struct _X86_EFLAGS {
	BYTE CF : 1; // carry flag
	BYTE REV1 : 1;
	BYTE PF : 1; // parity flag
	BYTE REV2 : 1;
	BYTE AF : 1; // AUX flag
	BYTE REV3 : 1;
	BYTE ZF : 1; // zero flag
	BYTE SF : 1; // sign flag
	BYTE TF : 1; // trap flag
	BYTE IF : 1;
	BYTE DF : 1; // direction flag
	BYTE OF : 1; // overflow flag
	BYTE IOPL : 2;
	BYTE NT : 1;
	BYTE REV4 : 1;
	BYTE RF : 1;
	BYTE VM : 1;
	BYTE AC : 1;
	BYTE VIF : 1;
	BYTE VIP : 1;
	BYTE ID : 1;
	BYTE REV5 : 1;
	BYTE REV6 : 1;
	BYTE REV7 : 1;
	BYTE REV8 : 1;
	BYTE REV9 : 1;
	BYTE REV10 : 1;
	BYTE REV11 : 1;
	BYTE REV12 : 1;
	BYTE REV13 : 1;
	BYTE REV14 : 1;
} X86_EFLAGS;

inline void x86InitEflags(X86_EFLAGS* eflags) {
	eflags->CF = 0b0;
	eflags->REV1 = 0b1;
	eflags->PF = 0b0;
	eflags->REV2 = 0b0;
	eflags->AF = 0b0;
	eflags->REV3 = 0b0;
	eflags->ZF = 0b0;
	eflags->SF = 0b0;
	eflags->TF = 0b0;
	eflags->IF = 0b0;
	eflags->DF = 0b0;
	eflags->OF = 0b0;
	eflags->IOPL = 0b00;
	eflags->NT = 0b0;
	eflags->REV4 = 0b0;
	eflags->RF = 0b0;
	eflags->VM = 0b0;
	eflags->AC = 0b0;
	eflags->VIF = 0b0;
	eflags->VIP = 0b0;
	eflags->ID = 0b0;
	eflags->REV5 = 0b0;
	eflags->REV6 = 0b0;
	eflags->REV7 = 0b0;
	eflags->REV8 = 0b0;
	eflags->REV9 = 0b0;
	eflags->REV10 = 0b0;
	eflags->REV11 = 0b0;
	eflags->REV12 = 0b0;
	eflags->REV13 = 0b0;
	eflags->REV14 = 0b0;
};

#endif
