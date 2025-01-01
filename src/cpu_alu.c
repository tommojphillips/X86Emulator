// alu.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include "cpu_alu.h"
#include "cpu_eflags.h"
#include "cpu_instruction.h"

#include "type_defs.h"
#include "mem_tracking.h"

BYTE set_overflow_add(int32_t a, int32_t b, int32_t c, uint32_t operand_size)
{
	switch (operand_size) {
		case 1: {
			const unsigned char r = (c & 0xFF);
			return ((signed char)(a ^ c) < 0 && (signed char)(b ^ c) >= 0);
		} break;
		case 2: {
			const unsigned short r = (c & 0xFFFF);
			return ((signed short)(a ^ c) < 0 && (signed short)(b ^ c) >= 0);
		} break;
		case 4: {
			const unsigned int r = (c & 0xFFFFFFFF);
			return ((signed int)(a ^ c) < 0 && (signed int)(b ^ c) >= 0);
		} break;
	}
	return 0;
}
BYTE set_overflow_sub(int32_t a, int32_t b, int32_t c, uint32_t operand_size)
{
	switch (operand_size) {
		case 1: {
			const unsigned char r = (c & 0xFF);
			return ((signed char)(a ^ b) < 0 && (signed char)(b ^ r) >= 0);
		} break;
		case 2: {
			const unsigned short r = (c & 0xFFFF);
			return ((signed short)(a ^ b) < 0 && (signed short)(b ^ r) >= 0);
		} break;
		case 4: {
			const unsigned int r = (c & 0xFFFFFFFF);
			return ((signed int)(a ^ b) < 0 && (signed int)(b ^ r) >= 0);
		} break;
	}
	return 0;
}
BYTE set_carry(int32_t a, int32_t b, uint32_t operand_size)
{
	switch (operand_size) {
		case 1: {
			return (signed char)a < (signed char)b;
		} break;
		case 2: {
			return (signed short)a < (signed short)b;
		} break;
		case 4: {
			return (signed int)a < (signed int)b;
		} break;
	}
	return 0;
}

int x86Alu(X86_EFLAGS* eflags, INSTRUCTION_TYPE type, uint32_t operand1, uint32_t operand2, uint32_t operand_size, uint32_t* result)
{
	uint32_t r = 0;
	switch (type) {
		case INSTRUCTION_TYPE_INC: // preserve CF; set OF, AF, ZF, PSF, F
			r = (operand1 + 1);
			eflags->OF = set_overflow_add(operand1, 1, r, operand_size);
			break;
		
		case INSTRUCTION_TYPE_DEC: // preserve CF; set OF, AF, ZF, SF, PF
			r = (operand1 - 1);
			eflags->OF = set_overflow_sub(operand1, 1, r, operand_size);
			break;
		
		case INSTRUCTION_TYPE_ADD: // set OF, CF, AF, ZF, SF, PF
			r = (operand1 + operand2);
			eflags->OF = set_overflow_add(operand1, operand2, r, operand_size);
			eflags->CF = set_carry(r, operand1, operand_size);
			break;

		case INSTRUCTION_TYPE_SUB: // set OF, CF, AF, ZF, SF, PF
		case INSTRUCTION_TYPE_CMP:
			r = (operand1 - operand2);
			eflags->OF = set_overflow_sub(operand1, operand2, r, operand_size);
			eflags->CF = set_carry(operand1, operand2, operand_size);
			break;

		case INSTRUCTION_TYPE_AND: // clear: OF, CF; set ZF, SF, PF; AF undefined
			r = (operand1 & operand2);
			eflags->OF = 0;
			eflags->CF = 0;
			break;
		
		case INSTRUCTION_TYPE_XOR: // clear: OF, CF; set ZF, SF, PF; AF undefined
			r = (operand1 ^ operand2);
			eflags->OF = 0;
			eflags->CF = 0;
			break;
		
		case INSTRUCTION_TYPE_OR: // clear: OF, CF; set ZF, SF, PF; AF undefined
			r = (operand1 | operand2);
			eflags->OF = 0;
			eflags->CF = 0;
			break;

		case INSTRUCTION_TYPE_ADC: // set OF, SF, ZF, AF, CF, PF
			r = operand1 + operand2 + eflags->CF;
			eflags->OF = set_overflow_add(operand1, operand2, r, operand_size);
			eflags->CF = set_carry(r, operand1, operand_size) || (eflags->CF && operand1 == r);
			break;

		case INSTRUCTION_TYPE_SBB: // set OF, SF, ZF, AF, CF, PF
			r = operand1 - operand2 - eflags->CF;
			eflags->OF = set_overflow_sub(operand1, operand2, r, operand_size);
			eflags->CF = set_carry(operand1, operand2, operand_size) || (eflags->CF && operand1 == operand2);
			break;
	}

	eflags->ZF = (r == 0);
	eflags->SF = (r >> (operand_size * 8 - 1)) & 1;
	eflags->PF = (r % 2 == 0);
			
	if (result != NULL) {
		*result = r;
	}

	return 0;
}