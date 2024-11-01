// alu.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include "cpu_alu.h"
#include "cpu_eflags.h"
#include "cpu_instruction.h"

#include "type_defs.h"
#include "mem_tracking.h"

void alu_set_AF(X86_EFLAGS* eflags, uint32_t result, uint32_t operand_size)
{
	eflags->AF = 0;
}
void alu_set_CF(X86_EFLAGS* eflags, uint32_t operand1, uint32_t operand2, uint32_t operand_size)
{
	eflags->CF = (operand2 < operand1);
}
void alu_set_SF(X86_EFLAGS* eflags, uint32_t result, uint32_t operand_size)
{
	eflags->SF = (result >> (operand_size * 8 - 1)) & 1;
}
void alu_set_OF(X86_EFLAGS* eflags, uint32_t operand1, uint32_t operand2, uint32_t result, uint32_t operand_size)
{
	uint32_t sign1 = (operand1 >> (operand_size * 8 - 1)) & 1;
	uint32_t sign2 = (operand2 >> (operand_size * 8 - 1)) & 1;
	uint32_t sign_result = (result >> (operand_size * 8 - 1)) & 1;
	eflags->OF = (sign1 == sign2) && (sign1 != sign_result);
}
void alu_set_PF(X86_EFLAGS* eflags, uint32_t result)
{
	eflags->PF = (result % 2 == 0);
}
void alu_set_ZF(X86_EFLAGS* eflags, uint32_t result)
{
	eflags->ZF = (result == 0);
}

int x86Alu(X86_EFLAGS* eflags, INSTRUCTION_TYPE type, uint32_t operand1, uint32_t operand2, uint32_t operand_size, uint32_t* result)
{
	uint32_t r = 0;
	switch (type) {
		case INSTRUCTION_TYPE_INC: // preserve CF; set OF, SF, ZF, AF, PF
			r = (operand1 + 1);
			eflags->OF = (((operand1 ^ r) & (operand2 ^ r)) >> (operand_size * 8 - 1)) & 1;
			break;
		
		case INSTRUCTION_TYPE_DEC: // preserve CF; set OF, SF, ZF, AF, PF
			r = (operand1 - 1);
			eflags->OF = (((operand1 ^ operand2) & (operand1 ^ r)) >> (operand_size * 8 - 1)) & 1;
			break;
		
		case INSTRUCTION_TYPE_ADD: // set OF, SF, ZF, AF, CF, PF
			r = (operand1 + operand2);
			eflags->OF = (((operand1 ^ r) & (operand2 ^ r)) >> (operand_size * 8 - 1)) & 1;
			eflags->CF = (r < operand1);
			break;

		case INSTRUCTION_TYPE_SUB: // set OF, SF, ZF, AF, CF, PF
		case INSTRUCTION_TYPE_CMP:
			r = (operand1 - operand2);
			eflags->OF = (((operand1 ^ operand2) & (operand1 ^ r)) >> (operand_size * 8 - 1)) & 1;
			eflags->CF = (operand1 < operand2);
			break;

		case INSTRUCTION_TYPE_AND: // clear: OF, CF; set SF, ZF, PF; AF undefined
			r = (operand1 & operand2);
			eflags->OF = 0;
			eflags->CF = 0;
			break;
		
		case INSTRUCTION_TYPE_XOR: // clear: OF, CF; set SF, ZF, PF; AF undefined
			r = (operand1 ^ operand2);
			eflags->OF = 0;
			eflags->CF = 0;
			break;
		
		case INSTRUCTION_TYPE_OR: // clear: OF, CF; set SF, ZF, PF; AF undefined
			r = (operand1 | operand2);
			eflags->OF = 0;
			eflags->CF = 0;
			break;
	}

	alu_set_SF(eflags, r, operand_size);
	alu_set_ZF(eflags, r);
	alu_set_PF(eflags, r);
			
	if (result != NULL) {
		*result = r;
	}

	return 0;
}