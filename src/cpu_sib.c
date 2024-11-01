// cpu_sib.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <string.h>
#include "cpu.h"

void get_sib_displ_reg_reg_mnemonic(char* buf, int32_t displacement, uint32_t address_size, uint32_t reg, uint32_t reg2, uint32_t scale)
{
	MNEMONIC_REG((buf, "[%s", reg, address_size));
	MNEMONIC_REG((buf + strlen(buf), "+%s", reg2, address_size));
	sprintf(buf + strlen(buf), "*%x", scale);

	if (displacement < 0)
		sprintf(buf + strlen(buf), "%d]", displacement);
	else
		sprintf(buf + strlen(buf), "+%d]", displacement);
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

int addressing_mode_reg_ptr(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type)
{
	// [reg]
	uint32_t addr = x86CPUGetRegister(cpu, reg, 4); // ptr is 4 bytes.
	uint32_t val = x86CPUReadMemory(cpu, addr, operand_size);
	if (address != NULL)
		*address = addr;
	if (value != NULL)
		*value = val;
	if (type != NULL)
		*type = INSTRUCTION_RM_MEM;
	MNEMONIC_REG((cpu->addressing_str, "[%s]", reg, 4)); // ptr is 4 bytes.
	return 0;
}
int addressing_mode_reg(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t operand_size, uint32_t* value, INSTRUCTION_RM* type)
{
	// reg
	if (value != NULL)
		*value = x86CPUGetRegister(cpu, mode->rm, operand_size);
	if (type != NULL)
		*type = INSTRUCTION_RM_REG;
	MNEMONIC_REG((cpu->addressing_str, "%s", mode->rm, operand_size));
	return 0;
}
int addressing_mode_reg_ptr_disp(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t displacement_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter)
{
	// [reg+disp]
	uint32_t reg = mode->rm;
	uint32_t reg_v = x86CPUGetRegister(cpu, reg, operand_size);
	uint32_t displacement = x86CPUFetchMemory(cpu, displacement_size, counter);
	uint32_t addr = reg_v + displacement;

	if (address != NULL)
		*address = addr;
	if (value != NULL)
		*value = x86CPUReadMemory(cpu, addr, operand_size);
	if (type != NULL)
		*type = INSTRUCTION_RM_MEM;

	MNEMONIC_REG((cpu->addressing_str, "[%s", reg, operand_size));
	MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "+0x%x]", displacement));
	return 0;
}
int addressing_mode_sib_reg_reg_disp8(X86_CPU* cpu, uint32_t address_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter)
{
	// SIB + disp8 -> [ disp8 + reg + reg * n ]

	BYTE byte = x86CPUFetchByte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	uint32_t scale = get_sib_scale(sib->scale);
	uint32_t reg = sib->base;
	uint32_t reg2 = sib->index;
	uint32_t reg_v = x86CPUGetRegister(cpu, reg, address_size);
	uint32_t reg2_v = x86CPUGetRegister(cpu, reg2, address_size);
	int8_t displacement = (int8_t)x86CPUFetchByte(cpu, counter); // SIGNED	
	uint32_t addr = displacement + reg_v + reg2_v * scale;

	if (address != NULL)
		*address = addr;
	if (value != NULL)
		*value = x86CPUReadMemory(cpu, addr, operand_size);
	if (type != NULL)
		*type = INSTRUCTION_RM_MEM;
	
	get_sib_displ_reg_reg_mnemonic(cpu->addressing_str, displacement, address_size, reg, reg2, scale);
	return 0;
}
int addressing_mode_sib_reg_reg_disp32(X86_CPU* cpu, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter)
{
	// SIB + disp32 -> [ disp32 + reg[8/32] + reg * n ]

	BYTE byte = x86CPUFetchByte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	uint32_t scale = get_sib_scale(sib->scale);
	uint32_t reg = sib->base;
	uint32_t reg2 = sib->index;
	uint32_t reg_v = x86CPUGetRegister(cpu, reg, operand_size);
	uint32_t reg2_v = x86CPUGetRegister(cpu, reg2, operand_size);
	int32_t displacement = (int32_t)x86CPUFetchDword(cpu, counter); // SIGNED
	uint32_t addr = displacement + reg_v + reg2_v * scale;
	if (address != NULL)
		*address = addr;
	if (value != NULL)
		*value = x86CPUReadMemory(cpu, addr, operand_size);
	if (type != NULL)
		*type = INSTRUCTION_RM_MEM;
	
	get_sib_displ_reg_reg_mnemonic(cpu->addressing_str, displacement, operand_size, reg, reg2, scale);
	return 0;
}
int addressing_mode_sib_reg_reg(X86_CPU* cpu, uint32_t displacement_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter)
{
	// SIB -> [ reg[8/32] + reg * n ]

	BYTE byte = x86CPUFetchByte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	uint32_t scale = get_sib_scale(sib->scale);
	uint32_t reg = sib->base;
	uint32_t reg2 = sib->index;
	uint32_t reg_v = x86CPUGetRegister(cpu, reg, displacement_size);
	uint32_t reg2_v = x86CPUGetRegister(cpu, reg2, operand_size);
	uint32_t addr = reg_v + reg2_v * scale;
	
	if (address != NULL)
		*address = addr;
	if (value != NULL)
		*value = x86CPUReadMemory(cpu, addr, operand_size);
	if (type != NULL)
		*type = INSTRUCTION_RM_MEM;

	MNEMONIC_REG((cpu->addressing_str, "[%s", reg, operand_size));
	MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "+%s", reg2, operand_size));
	MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "*%x]", scale));
	return 0;
}
int addressing_mode_sib_disp(X86_CPU* cpu, uint32_t address_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter)
{
	// SIB mode -> [ disp32 + reg * n ]

	BYTE byte = x86CPUFetchByte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	BYTE scale = get_sib_scale(sib->scale);
	uint32_t reg = sib->index;
	uint32_t reg_v = x86CPUGetRegister(cpu, reg, address_size);

	if (address_size != 2) {
		address_size = 4;
	}

	if (sib->base == 0b101) {
		// [ disp + reg * n ]
		DWORD disp32 = x86CPUFetchDword(cpu, counter);
		uint32_t addr = disp32 + reg_v * scale;

		if (address != NULL)
			*address = addr;
		if (value != NULL)
			*value = x86CPUReadMemory(cpu, addr, operand_size);
		if (type != NULL)
			*type = INSTRUCTION_RM_MEM;

		MNEMONIC_STR((cpu->addressing_str, "[0x%x", disp32));
	}
	else {
		// [ reg + reg * n ]
		uint32_t reg2 = sib->base;
		uint32_t reg2_v = x86CPUGetRegister(cpu, reg2, address_size);
		uint32_t addr = reg_v + reg2_v * scale;
		if (address != NULL)
			*address = addr;
		if (value != NULL)
			*value = x86CPUReadMemory(cpu, addr, operand_size);
		if (type != NULL)
			*type = INSTRUCTION_RM_MEM;

		MNEMONIC_REG((cpu->addressing_str, "[%s", reg2, address_size));
	}

	MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "+%s", reg, address_size));
	MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "*%x]", scale));

	return 0;
}
int addressing_mode_disp32(X86_CPU* cpu, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter)
{
	// disp32 - displacement only addressing mode
	DWORD disp32 = x86CPUFetchDword(cpu, counter);
	uint32_t addr = disp32;

	if (address != NULL)
		*address = addr;
	if (value != NULL)
		*value = x86CPUReadMemory(cpu, addr, 4);
	if (type != NULL)
		*type = INSTRUCTION_RM_MEM;

	MNEMONIC_STR((cpu->addressing_str, "[0x%x]", disp32));
	return 0;
}

int get_addressing_mode(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t address_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_TYPE* type, uint32_t* counter)
{
	// figure out the addressing mode.

	switch (mode->mod) {
		case 0b00: {
			if (mode->rm == 0b100) {
				// SIB mode -> [ disp32 + reg * n ]
				addressing_mode_sib_disp(cpu, address_size, operand_size, address, value, type, counter);
			}
			else if (mode->rm == 0b101) {
				// disp32 - displacement only addressing mode
				addressing_mode_disp32(cpu, address, value, type, counter);
			}
			else {
				// [reg]
				addressing_mode_reg_ptr(cpu, mode->rm, 4, address, value, type);
			}

		} break;

		case 0b01: {
			// 1 byte displacement
			if (mode->rm == 0b100) {
				// SIB + disp8 -> [ disp8 + reg8 + reg * n ]
				addressing_mode_sib_reg_reg_disp8(cpu, address_size, operand_size, address, value, type, counter);
			}
			else /*if (mode->rm == 0b110)*/ {
				// [reg+disp8]
				addressing_mode_reg_ptr_disp(cpu, mode, 1, operand_size, address, value, type, counter);
			}

		} break;

		case 0b10: {
			// 4 byte displacement
			if (mode->rm == 0b100) {
				// SIB + disp32 -> [ disp32 + reg32 + reg * n ]
				addressing_mode_sib_reg_reg_disp32(cpu, operand_size, address, value, type, counter);
			}
			else if (mode->rm == 0b101) {
				// [reg+disp32]
				addressing_mode_reg_ptr_disp(cpu, mode, 4, operand_size, address, value, type, counter);
			}
			else /*if (mode->rm == 0b110)*/ {
				// [reg+disp32]
				addressing_mode_reg_ptr_disp(cpu, mode, 4, 4, address, value, type, counter);
			}
		} break;

		case 0b11: {
			// reg
			addressing_mode_reg(cpu, mode, operand_size, value, type);
		} break;
	}
	return 0;
}
