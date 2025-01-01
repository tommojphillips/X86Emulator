// cpu_sib.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef _CPU_SIB_H
#define _CPU_SIB_H

#include <stdint.h>

typedef struct _ADDRESSING_MODE_FIELD_STRUCT {
	uint32_t value;
	uint32_t address;
	BYTE reg; // if type is reg, than this is the register number.
	INSTRUCTION_RM type; // type of addressing. ( register, imm, indirect )
} ADDRESSING_MODE_FIELD_STRUCT;

typedef struct _ADDRESSING_MODE_STRUCT {
	ADDRESSING_MODE_FIELD_STRUCT src;
	ADDRESSING_MODE_FIELD_STRUCT dest;
} ADDRESSING_MODE_STRUCT;

int addressing_mode_reg(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state);
int addressing_mode_disp(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter);

int addressing_mode_16bit(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state);
int addressing_mode_16bit_disp(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t displacement_size, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter);

int addressing_mode_32bit(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state);
int addressing_mode_32bit_disp(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t displacement_size, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter);

int addressing_mode_sib_disp8(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter);
int addressing_mode_sib_disp32(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter);
int addressing_mode_sib(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter);

int get_addressing_mode(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t address_size, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter);

#endif
