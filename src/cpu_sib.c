// cpu_sib.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <string.h>
#include <stdint.h>

#include "cpu.h"
#include "cpu_sib.h"
#include "cpu_memory.h"

uint32_t get_16bit_indirect_address(X86_CPU* cpu, X86_MOD_RM_BITS* mode)
{
	uint32_t addr = 0;
	switch (mode->rm) {
		case 0b000: // [ BX + SI ]
			addr = x86CPUGetRegister(cpu, REG_BX, 2);
			addr += x86CPUGetRegister(cpu, REG_SI, 2);
			break;
		case 0b001: // [ BX + DI ]
			addr = x86CPUGetRegister(cpu, REG_BX, 2);
			addr += x86CPUGetRegister(cpu, REG_DI, 2);
			break;
		case 0b010: // [ BP + SI ]
			addr = x86CPUGetRegister(cpu, REG_BP, 2);
			addr += x86CPUGetRegister(cpu, REG_SI, 2);
			break;
		case 0b011: // [ BP + DI ]
			addr = x86CPUGetRegister(cpu, REG_BP, 2);
			addr += x86CPUGetRegister(cpu, REG_DI, 2);
			break;
		case 0b100: // [ SI ]
			addr = x86CPUGetRegister(cpu, REG_SI, 2);
			break;
		case 0b101: // [ DI ]
			addr = x86CPUGetRegister(cpu, REG_DI, 2);
			break;
		case 0b111: // [ BX ]
			addr = x86CPUGetRegister(cpu, REG_BX, 2);
			break;
	}
	return addr;
}
uint32_t get_sib_scale(BYTE scale)
{
	switch (scale) {
		case 0b00:
			return 1;
		case 0b01:
			return 2;
		case 0b10:
			return 4;
		case 0b11:
			return 8;
	}
	return 1;
}

int addressing_mode_reg(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state)
{
	// 8/16/32 reg
	if (state != NULL) {
		state->value = x86CPUGetRegister(cpu, mode->rm, operand_size);
		state->type = INSTRUCTION_RM_REGISTER;
	}
	return 0;
}
int addressing_mode_disp(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter)
{
	// disp16/32 - displacement only addressing mode
	uint32_t disp = x86CPUFetchMemory(cpu, operand_size, counter);

	if (state != NULL) {
		state->address = disp;
		state->value = x86CPUReadMemory(cpu, disp, operand_size);
		state->type = INSTRUCTION_RM_INDIRECT;
	}
	return 0;
}
int addressing_mode_32bit(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state)
{
	// [reg]
	uint32_t addr = x86CPUGetRegister(cpu, mode->rm, 4);

	if (state != NULL) {
		state->address = addr;
		state->value = x86CPUReadMemory(cpu, addr, operand_size);
		state->type = INSTRUCTION_RM_INDIRECT;
	}
	return 0;
}
int addressing_mode_32bit_disp(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t displacement_size, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter)
{
	// [reg32 + disp8/32]
	uint32_t reg = mode->rm;
	uint32_t reg_v = x86CPUGetRegister(cpu, reg, 4);
	uint32_t displacement = x86CPUFetchMemory(cpu, displacement_size, counter);
	uint32_t addr = reg_v + displacement;

	if (state != NULL) {
		state->address = addr;
		state->value = x86CPUReadMemory(cpu, addr, operand_size);
		state->type = INSTRUCTION_RM_INDIRECT;
	}
	return 0;
}
int addressing_mode_16bit(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state)
{
	// [reg16 + reg16]

	uint32_t addr = get_16bit_indirect_address(cpu, mode);

	if (state != NULL) {
		state->address = addr;
		state->value = x86CPUReadMemory(cpu, addr, operand_size);
		state->type = INSTRUCTION_RM_INDIRECT;
	}
	return 0;
}
int addressing_mode_16bit_disp(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t displacement_size, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter)
{
	// [reg16 + reg16 + disp8/16]
	uint32_t displacement = x86CPUFetchMemory(cpu, displacement_size, counter);
	uint32_t addr = get_16bit_indirect_address(cpu, mode);
	addr += displacement;
	
	if (state != NULL) {
		state->address = addr;
		state->value = x86CPUReadMemory(cpu, addr, operand_size);
		state->type = INSTRUCTION_RM_INDIRECT;
	}
	return 0;
}
int addressing_mode_sib_disp8(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter)
{
	// SIB + disp8

	BYTE byte = x86CPUFetchByte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	uint32_t base = x86CPUGetRegister(cpu, sib->base, 4);
	signed char displacement = (int8_t)x86CPUFetchByte(cpu, counter);

	if (sib->index == 0b100) {
		// SIB + disp8 -> [ base + disp8 ]
		uint32_t addr = base + displacement;
		if (state != NULL) {
			state->address = addr;
			state->value = x86CPUReadMemory(cpu, addr, operand_size);
			state->type = INSTRUCTION_RM_INDIRECT;
		}
	}
	else {
		// SIB + disp8 -> [ base + (index * n) + disp8 ]
		uint32_t scale = get_sib_scale(sib->scale);
		uint32_t index = x86CPUGetRegister(cpu, sib->index, 4);
		uint32_t addr = base + (index * scale) + displacement;
		if (state != NULL) {
			state->address = addr;
			state->value = x86CPUReadMemory(cpu, addr, operand_size);
			state->type = INSTRUCTION_RM_INDIRECT;
		}
	}
	return 0;
}
int addressing_mode_sib_disp32(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter)
{
	// SIB + disp32

	BYTE byte = x86CPUFetchByte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	uint32_t base = x86CPUGetRegister(cpu, sib->base, 4);
	int displacement = (int)x86CPUFetchDword(cpu, counter);

	if (sib->index == 0b100) {
		// SIB + disp32 -> [ base + disp32 ]
		uint32_t addr = base + displacement;
		if (state != NULL) {
			state->address = addr;
			state->value = x86CPUReadMemory(cpu, addr, operand_size);
			state->type = INSTRUCTION_RM_INDIRECT;
		}
	}
	else {
		// SIB + disp32 -> [ base + (index * n) + disp32 ]
		uint32_t scale = get_sib_scale(sib->scale);
		uint32_t index = x86CPUGetRegister(cpu, sib->index, 4);
		uint32_t addr = base + (index * scale) + displacement;
		if (state != NULL) {
			state->address = addr;
			state->value = x86CPUReadMemory(cpu, addr, operand_size);
			state->type = INSTRUCTION_RM_INDIRECT;
		}
	}
	return 0;
}
int addressing_mode_sib(X86_CPU* cpu, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter)
{
	// SIB mode

	BYTE byte = x86CPUFetchByte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	BYTE scale = get_sib_scale(sib->scale);
	uint32_t index = x86CPUGetRegister(cpu, sib->index, 4);

	if (sib->index == 0b100) {
		if (sib->base == 0b101) {
			// [ disp32 ]
			DWORD disp32 = x86CPUFetchDword(cpu, counter);
			uint32_t addr = disp32;

			if (state != NULL) {
				state->address = addr;
				state->value = x86CPUReadMemory(cpu, addr, operand_size);
				state->type = INSTRUCTION_RM_INDIRECT;
			}
		}
		else {
			// [ base ]
			uint32_t base = x86CPUGetRegister(cpu, sib->base, 4);
			uint32_t addr = base;

			if (state != NULL) {
				state->address = addr;
				state->value = x86CPUReadMemory(cpu, addr, operand_size);
				state->type = INSTRUCTION_RM_INDIRECT;
			}
		}
	}
	else {
		if (sib->base == 0b101) {
			// [ (index * n) + disp32 ]
			DWORD disp32 = x86CPUFetchDword(cpu, counter);
			uint32_t addr = (index * scale) + disp32;

			if (state != NULL) {
				state->address = addr;
				state->value = x86CPUReadMemory(cpu, addr, operand_size);
				state->type = INSTRUCTION_RM_INDIRECT;
			}
		}
		else {
			// [ base + (index * n) ]
			uint32_t base = x86CPUGetRegister(cpu, sib->base, 4);
			uint32_t addr = base + (index * scale);

			if (state != NULL) {
				state->address = addr;
				state->value = x86CPUReadMemory(cpu, addr, operand_size);
				state->type = INSTRUCTION_RM_INDIRECT;
			}
		}
	}

	return 0;
}

int get_addressing_mode(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t address_size, uint32_t operand_size, ADDRESSING_MODE_FIELD_STRUCT* state, uint32_t* counter)
{
	// figure out the addressing mode.

	switch (mode->mod) {

		case 0b00: {
			if (address_size == 4) {
				if (mode->rm == 0b100) {
					// [ SIB ]
					addressing_mode_sib(cpu, operand_size, state, counter);
				}
				else if (mode->rm == 0b101) {
					// [ disp32 ]
					addressing_mode_disp(cpu, 4, state, counter);
				}
				else {
					// [ reg32 ]
					addressing_mode_32bit(cpu, mode, 4, state);
				}
			}
			else {
				if (mode->rm == 0b110) {
					// [ disp16 ]
					addressing_mode_disp(cpu, 2, state, counter);
				}
				else {
					// [ reg16 + reg16 ]
					addressing_mode_16bit(cpu, mode, 4, state);
				}
			}
		} break;

		case 0b01: {
			if (address_size == 4) {
				if (mode->rm == 0b100) {
					// [ SIB + disp8 ]
					addressing_mode_sib_disp8(cpu, operand_size, state, counter);
				}
				else {
					// [ reg32 + disp8 ]
					addressing_mode_32bit_disp(cpu, mode, 1, operand_size, state, counter);
				}
			}
			else {
				// [ reg16 + reg16 + disp8 ]
				addressing_mode_16bit_disp(cpu, mode, 1, operand_size, state, counter);
			}
		} break;

		case 0b10: {
			if (address_size == 4) {
				if (mode->rm == 0b100) {
					// [ SIB + disp32 ]
					addressing_mode_sib_disp32(cpu, operand_size, state, counter);
				}
				else {
					// [ reg32 + disp32 ]
					addressing_mode_32bit_disp(cpu, mode, 4, operand_size, state, counter);
				}
			}
			else {
				// [ reg16 + reg16 + disp16 ]
				addressing_mode_16bit_disp(cpu, mode, 2, operand_size, state, counter);
			}
		} break;

		case 0b11: {
			// reg
			addressing_mode_reg(cpu, mode, operand_size, state);
		} break;
	}
	return 0;
}
