// cpu_mnemonics.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "cpu_mnemonics.h"

#undef MNEMONIC_STR
#undef MNEMONIC_REG
#undef MNEMONIC_SEG
#undef MNEMONIC_CON
#undef MNEMONIC_SEG_OVERIDE

#define X86_MNEMONIC_STR(_x_) sprintf _x_
#define X86_MNEMONIC_REG(_x_) x86CPUGetRegisterMnemonic _x_
#define X86_MNEMONIC_SEG(_x_) x86CPUGetSegmentRegisterMnemonic _x_
#define X86_MNEMONIC_CON(_x_) x86CPUGetControlRegisterMnemonic _x_
#define X86_MNEMONIC_SEG_OVERIDE(_x_) \
switch (_x_) { \
	case 0x26: \
		X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "es:")); \
		break; \
	case 0x2E: \
		X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "cs:")); \
		break; \
	case 0x36: \
		X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "ss:")); \
		break; \
	case 0x3E: \
		X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "ds:")); \
		break; \
	case 0x64: \
		X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "fs:")); \
		break; \
	case 0x65: \
		X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "gs:")); \
		break; \
}

uint32_t get_sib_scale(BYTE scale);

const char* register_names_32bit[X86_GENERAL_REGISTER_COUNT] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
const char* register_names_16bit[X86_GENERAL_REGISTER_COUNT] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
const char* register_names_8bit[X86_GENERAL_REGISTER_COUNT] = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
const char* segment_names[X86_SEGMENT_REGISTER_COUNT] = { "es", "cs", "ss", "ds", "fs", "gs" };

void get_sib_disp_mnemonic(char* buf, int displacement, X86_SIB* sib)
{
	if (sib->index == 0b100) {
		X86_MNEMONIC_REG((buf, "[%s", sib->base, 4));
	}
	else {
		uint32_t scale = get_sib_scale(sib->scale);
		X86_MNEMONIC_REG((buf, "[%s", sib->base, 4));
		X86_MNEMONIC_REG((buf + strlen(buf), "+%s", sib->index, 4));
		X86_MNEMONIC_STR((buf + strlen(buf), "*%x", scale));
	}

	if (displacement < 0)
		X86_MNEMONIC_STR((buf + strlen(buf), "%d]", displacement));
	else
		X86_MNEMONIC_STR((buf + strlen(buf), "+%d]", displacement));
}
void get_16bit_indirect_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t displacement)
{
	switch (mode->rm) {
		case 0b000: // BX + SI
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s+", REG_BX, 2));
			X86_MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "%s", REG_SI, 2));
			break;
		case 0b001: // BX + DI
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s+", REG_BX, 2));
			X86_MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "%s", REG_DI, 2));
			break;
		case 0b010: // BP + SI
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s+", REG_BP, 2));
			X86_MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "%s", REG_SI, 2));
			break;
		case 0b011: // BP + DI
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s+", REG_BP, 2));
			X86_MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "%s", REG_DI, 2));
			break;
		case 0b100: // SI
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s", REG_SI, 2));
			break;
		case 0b101: // DI
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s", REG_DI, 2));
			break;
		case 0b111: // BX
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s", REG_BX, 2));
			break;
	}

	if (displacement == 0) {
		X86_MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "]"));
	}
	else {
		X86_MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "+0x%x]", displacement));
	}
}

BYTE fetch_byte(X86_CPU* cpu, uint32_t* counter)
{
	BYTE* ptr = (BYTE*)(cpu->eip_ptr + *counter);
	*counter += 1;
	if (ptr != NULL)
		return *ptr;
	return 0;
}
WORD fetch_word(X86_CPU* cpu, uint32_t* counter)
{
	WORD* ptr = (WORD*)(cpu->eip_ptr + *counter);
	*counter += 2;
	if (ptr != NULL)
		return *ptr;
	return 0;
}
DWORD fetch_dword(X86_CPU* cpu, uint32_t* counter)
{
	DWORD* ptr = (DWORD*)(cpu->eip_ptr + *counter);
	*counter += 4;
	if (ptr != NULL)
		return *ptr;
	return 0;
}
uint32_t fetch_memory(X86_CPU* cpu, uint32_t operand_size, uint32_t* counter)
{
	switch (operand_size) {
		case 1:
			return fetch_byte(cpu, counter);
		case 2:
			return fetch_word(cpu, counter);
		case 4:
			return fetch_dword(cpu, counter);
	}
	return 0;
}

int addressing_X86_MNEMONIC_REG(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t operand_size)
{
	// 8/16/32 reg
	X86_MNEMONIC_REG((cpu->addressing_str, "%s", mode->rm, operand_size));
	return 0;
}
int addressing_mnemonic_disp(X86_CPU* cpu, uint32_t displacement_size, uint32_t* counter)
{
	// disp16/32 - displacement only addressing mode

	uint32_t displacement = fetch_memory(cpu, displacement_size, counter);
	X86_MNEMONIC_STR((cpu->addressing_str, "[0x%x]", displacement));
	return 0;
}
int addressing_mnemonic_32bit(X86_CPU* cpu, X86_MOD_RM_BITS* mode)
{
	// [reg]
	X86_MNEMONIC_REG((cpu->addressing_str, "[%s]", mode->rm, 4));
	return 0;
}
int addressing_mnemonic_32bit_disp(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t displacement_size, uint32_t* counter)
{
	// [reg32 + disp8/32]

	uint32_t displacement = fetch_memory(cpu, displacement_size, counter);
	X86_MNEMONIC_REG((cpu->addressing_str, "[%s", mode->rm, 4));
	X86_MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "+0x%x]", displacement));
	return 0;
}
int addressing_mnemonic_16bit(X86_CPU* cpu, X86_MOD_RM_BITS* mode)
{
	// [reg16 + reg16]
	get_16bit_indirect_mnemonic(cpu, mode, 0);
	return 0;
}
int addressing_mnemonic_16bit_disp(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t displacement_size, uint32_t* counter)
{
	// [reg16 + reg16 + disp8/16]
	uint32_t displacement = fetch_memory(cpu, displacement_size, counter);		
	get_16bit_indirect_mnemonic(cpu, mode, displacement);
	return 0;
}
int addressing_mnemonic_sib_disp8(X86_CPU* cpu, uint32_t* counter)
{
	// SIB + disp8 -> [ base + (index * n) + disp8 ]

	BYTE byte = fetch_byte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	signed char displacement = (int8_t)fetch_byte(cpu, counter);	
	get_sib_disp_mnemonic(cpu->addressing_str, displacement, sib);
	return 0;
}
int addressing_mnemonic_sib_disp32(X86_CPU* cpu, uint32_t* counter)
{
	// SIB + disp32 -> [ base + (index * n) + disp32 ]

	BYTE byte = fetch_byte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	int displacement = (int)fetch_dword(cpu, counter);	
	get_sib_disp_mnemonic(cpu->addressing_str, displacement, sib);
	return 0;
}
int addressing_mnemonic_sib(X86_CPU* cpu, uint32_t* counter)
{
	// SIB mode

	BYTE byte = fetch_byte(cpu, counter);
	X86_SIB* sib = (X86_SIB*)&byte;
	BYTE scale = get_sib_scale(sib->scale);
	uint32_t index = x86CPUGetRegister(cpu, sib->index, 4);

	if (sib->index == 0b100) {
		if (sib->base == 0b101) {
			// [ disp32 ]
			DWORD disp32 = fetch_dword(cpu, counter);
			X86_MNEMONIC_STR((cpu->addressing_str, "[0x%x]", disp32));
		}
		else {
			// [ base ]
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s]", sib->base, 4));
		}
	}
	else {
		if (sib->base == 0b101) {
			// [ (index * n) + disp32 ]
			DWORD disp32 = fetch_dword(cpu, counter);
			X86_MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "[%s", sib->index, 4));
			X86_MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "*%x", scale));
			X86_MNEMONIC_STR((cpu->addressing_str, "+0x%x]", disp32));
		}
		else {
			// [ base + (index * n) ]
			X86_MNEMONIC_REG((cpu->addressing_str, "[%s", sib->base, 4));
			X86_MNEMONIC_REG((cpu->addressing_str + strlen(cpu->addressing_str), "+%s", sib->index, 4));
			X86_MNEMONIC_STR((cpu->addressing_str + strlen(cpu->addressing_str), "*%x]", scale));
		}
	}
	return 0;
}
int get_addressing_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t address_size, uint32_t operand_size, uint32_t* counter)
{
	// figure out the addressing mode.

	switch (mode->mod) {

		case 0b00: {
			if (address_size == 4) {
				if (mode->rm == 0b100) {
					// [ SIB ]
					addressing_mnemonic_sib(cpu, counter);
				}
				else if (mode->rm == 0b101) {
					// [ disp32 ]
					addressing_mnemonic_disp(cpu, 4, counter);
				}
				else {
					// [ reg32 ]
					addressing_mnemonic_32bit(cpu, mode);
				}
			}
			else {
				if (mode->rm == 0b110) {
					// [ disp16 ]
					addressing_mnemonic_disp(cpu, 2, counter);
				}
				else {
					// [ reg16 + reg16 ]
					addressing_mnemonic_16bit(cpu, mode);
				}
			}
		} break;

		case 0b01: {
			if (address_size == 4) {
				if (mode->rm == 0b100) {
					// [ SIB + disp8 ]
					addressing_mnemonic_sib_disp8(cpu, counter);
				}
				else {
					// [ reg32 + disp8 ]
					addressing_mnemonic_32bit_disp(cpu, mode, 1, counter);
				}
			}
			else {
				// [ reg16 + reg16 + disp8 ]
				addressing_mnemonic_16bit_disp(cpu, mode, 1, counter);
			}
		} break;

		case 0b10: {
			if (address_size == 4) {
				if (mode->rm == 0b100) {
					// [ SIB + disp32 ]
					addressing_mnemonic_sib_disp32(cpu, counter);
				}
				else {
					// [ reg32 + disp32 ]
					addressing_mnemonic_32bit_disp(cpu, mode, 4, counter);
				}
			}
			else {
				// [ reg16 + reg16 + disp16 ]
				addressing_mnemonic_16bit_disp(cpu, mode, 2, counter);
			}
		} break;

		case 0b11: {
			// reg
			addressing_X86_MNEMONIC_REG(cpu, mode, operand_size);
		} break;
	}
	return 0;
}

int move_imm_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// mov reg, imm8
	uint32_t imm = fetch_memory(cpu, operand_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "mov %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int move_ptr_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// mov reg, [address]
	uint32_t address = fetch_memory(cpu, operand_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "mov %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "[0x%x]", address));
	return 0;
}
int inc_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// inc reg
	X86_MNEMONIC_REG((cpu->output_str, "inc %s", reg, operand_size));
	return 0;
}
int dec_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// dec reg
	X86_MNEMONIC_REG((cpu->output_str, "dec %s", reg, operand_size));
	return 0;
}
int shr_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// shr reg, imm
	BYTE imm8 = fetch_byte(cpu, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "shr %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "%d", imm8));
	return 0;
}
int shl_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// shl reg, imm
	BYTE imm8 = fetch_byte(cpu, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "shl %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "%d", imm8));
	return 0;
}
int cld_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// clear direction flag
	X86_MNEMONIC_STR((cpu->output_str, "cld"));
	return 0;
}
int std_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// set direction flag
	X86_MNEMONIC_STR((cpu->output_str, "std"));
	return 0;
}
int cli_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// clear interrupt flag
	X86_MNEMONIC_STR((cpu->output_str, "cli"));
	return 0;
}
int sti_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// set interrupt flag
	X86_MNEMONIC_STR((cpu->output_str, "sti"));
	return 0;
}
int hlt_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// hlt
	X86_MNEMONIC_STR((cpu->output_str, "hlt"));
	return 0;
}
int jmp_imm_rel_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	int offset = 0;
	switch (operand_size) {
		case 1:
			offset = (signed char)fetch_byte(cpu, &counter);
			break;
		case 2:
			offset = (short)fetch_word(cpu, &counter);
			break;
		case 4:
			offset = (int)fetch_dword(cpu, &counter);
			break;
	}
	X86_MNEMONIC_STR((cpu->output_str, "jmp %d", offset));
	return 0;
}
int jmp_far_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	uint32_t address = fetch_dword(cpu, &counter);
	uint16_t selector = fetch_word(cpu, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "jmp 0x%04x:0x%08x", selector, address));
	return 0;
}
int jcc_mnemonic(X86_CPU* cpu, BYTE opcode, uint32_t operand_size, uint32_t counter)
{
	// jump condition

	int offset = 0;
	switch (operand_size) {
		case 1:
			offset = (signed char)fetch_byte(cpu, &counter);
			break;
		case 2:
			offset = (short)fetch_word(cpu, &counter);
			break;
		case 4:
			offset = (int)fetch_dword(cpu, &counter);
			break;
	}

	// last 4 bits of the opcode are the conditional test.
	switch (opcode & 0x0f) {
		case CONDITIONAL_TEST_OVERFLOW:
			X86_MNEMONIC_STR((cpu->output_str, "jo %d", offset));
			break;
		case CONDITIONAL_TEST_NO_OVERFLOW:
			X86_MNEMONIC_STR((cpu->output_str, "jno %d", offset));
			break;
		case CONDITIONAL_TEST_CARRY:
			X86_MNEMONIC_STR((cpu->output_str, "jc %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_CARRY:
			X86_MNEMONIC_STR((cpu->output_str, "jnc %d", offset));
			break;
		case CONDITIONAL_TEST_EQUAL_ZERO:
			X86_MNEMONIC_STR((cpu->output_str, "jz %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_EQUAL_ZERO:
			X86_MNEMONIC_STR((cpu->output_str, "jnz %d", offset));
			break;
		case CONDITIONAL_TEST_BELOW_OR_EQUAL:
			X86_MNEMONIC_STR((cpu->output_str, "jbe %d", offset));
			break;
		case CONDITIONAL_TEST_ABOVE:
			X86_MNEMONIC_STR((cpu->output_str, "ja %d", offset));
			break;
		case CONDITIONAL_TEST_SIGN:
			X86_MNEMONIC_STR((cpu->output_str, "js %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_SIGN:
			X86_MNEMONIC_STR((cpu->output_str, "jns %d", offset));
			break;
		case CONDITIONAL_TEST_PARITY:
			X86_MNEMONIC_STR((cpu->output_str, "jp %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_PARITY:
			X86_MNEMONIC_STR((cpu->output_str, "jnp %d", offset));
			break;
		case CONDITIONAL_TEST_LESS:
			X86_MNEMONIC_STR((cpu->output_str, "jl %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_LESS:
			X86_MNEMONIC_STR((cpu->output_str, "jge %d", offset));
			break;
		case CONDITIONAL_TEST_LESS_OR_EQUAL:
			X86_MNEMONIC_STR((cpu->output_str, "jle %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_LESS_OR_EQUAL:
			X86_MNEMONIC_STR((cpu->output_str, "jg %d", offset));
			break;
	}
	return 0;
}
int cmp_imm_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// cmp value in reg to imm
	uint32_t imm = fetch_memory(cpu, operand_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "cmp %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int in_byte_reg_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// input byte/word/dword from I/O port in DX into AL/AX/EAX.
	X86_MNEMONIC_REG((cpu->output_str, "in %s, dx", REG_EAX, operand_size));
	return 0;
}
int out_byte_reg_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// output byte/word/dword in AL/AX/EAX to I/O port address in DX.
	X86_MNEMONIC_REG((cpu->output_str, "out dx, %s", REG_EAX, operand_size));
	return 0;
}
int in_byte_imm_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// input byte/word/dword from imm8 I/O port address into AL/AX/EAX
	BYTE imm = fetch_byte(cpu, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "in %s, ", REG_EAX, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int out_byte_imm_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// output byte/word/dword in AL/AX/EAX to I/O port address imm8.
	BYTE imm = fetch_byte(cpu, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "out 0x%x, ", imm));
	X86_MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), "%s", REG_EAX, operand_size));
	return 0;
}
int movzx_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t src_size, uint32_t dest_size, uint32_t counter)
{
	// B6 MOVZX r16, r/m8
	// B6 MOVZX r32, r/m8
	// B7 MOVZX r32, r/m16

	get_addressing_mnemonic(cpu, mode, dest_size, src_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "movzx %s, ", mode->reg, dest_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), cpu->addressing_str));
	return 0;
}
int movsx_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t src_size, uint32_t dest_size, uint32_t counter)
{
	// B6 MOVSX r16, r/m8
	// B6 MOVSX r32, r/m8
	// B7 MOVSX r32, r/m16

	get_addressing_mnemonic(cpu, mode, dest_size, src_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "movsx %s, ", mode->reg, dest_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), cpu->addressing_str));
	return 0;
}
int and_imm_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// logical AND reg imm
	uint32_t imm = fetch_memory(cpu, operand_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "and %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int or_imm_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// logical OR reg imm
	uint32_t imm = fetch_memory(cpu, operand_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "or %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int add_imm_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// add reg imm
	uint32_t imm = fetch_memory(cpu, operand_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "add %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int sub_imm_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// sub reg imm
	uint32_t imm = fetch_memory(cpu, operand_size, &counter);
	X86_MNEMONIC_REG((cpu->output_str, "sub %s, ", reg, operand_size));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int lgdt_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t address_size, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter) {
	// Load the GDT register
	get_addressing_mnemonic(cpu, mode, address_size, operand_size, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "lgdt "));
	X86_MNEMONIC_SEG_OVERIDE(segment_override_byte);
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), cpu->addressing_str));
	return 0;
}
int lidt_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t address_size, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter) {
	// Load the IDT register
	get_addressing_mnemonic(cpu, mode, address_size, operand_size, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "lidt "));
	X86_MNEMONIC_SEG_OVERIDE(segment_override_byte);
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), cpu->addressing_str));
	return 0;
}
int lldt_imm_mnemonic(X86_CPU* cpu, uint32_t counter) {
	// Load the LDT register
	WORD address = fetch_word(cpu, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "lldt "));
	X86_MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", address));
	return 0;
}
int lldt_reg_mnemonic(X86_CPU* cpu, uint32_t reg, uint32_t counter) {
	// Load the LDT register	
	X86_MNEMONIC_STR((cpu->output_str, "lldt "));
	X86_MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), "%s", reg, 2));
	return 0;
}
int mov_seg_r32_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t counter)
{
	// MOV seg, r32
	X86_MNEMONIC_STR((cpu->output_str, "mov "));
	X86_MNEMONIC_SEG((cpu->output_str + strlen(cpu->output_str), "%s, ", mode->reg));
	X86_MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), "%s", mode->rm, 4));
	return 0;
}
int mov_cr_r32_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t counter)
{
	// MOV cr0-7, r32
	X86_MNEMONIC_STR((cpu->output_str, "mov "));
	X86_MNEMONIC_CON((cpu->output_str + strlen(cpu->output_str), mode->rm));
	X86_MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), ", %s", mode->reg, 4));
	return 0;
}
int mov_r32_cr_mnemonic(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t counter)
{
	// MOV r32, cr0-7
	X86_MNEMONIC_STR((cpu->output_str, "mov "));
	X86_MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), "%s, ", mode->reg, 4));
	X86_MNEMONIC_CON((cpu->output_str + strlen(cpu->output_str), mode->rm));
	return 0;
}
int wrmsr_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	//Write to Model Specific Register
	X86_MNEMONIC_STR((cpu->output_str, "wrmsr"));
	return 0;
}
int invd_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// Invalidate Cache
	X86_MNEMONIC_STR((cpu->output_str, "invd"));
	return 0;
}
int wbinvd_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// Write Back and Invalidate Cache
	X86_MNEMONIC_STR((cpu->output_str, "wbinvd"));
	return 0;
}
int nop_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	X86_MNEMONIC_STR((cpu->output_str, "nop"));
	return 0;
}
int movs_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// Move Data From String to String - MOVS r8/r16/r32
	// move byte/word/dword from address ds:si/esi to es:di/edi.
	
	switch (operand_size) {
		case 4:
			X86_MNEMONIC_STR((cpu->output_str, "movsd"));
			break;
		case 2:
			X86_MNEMONIC_STR((cpu->output_str, "movsw"));
			break;
		case 1:
			X86_MNEMONIC_STR((cpu->output_str, "movsb"));
			break;
	}

	return 0;
}
int stos_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// Store String - STOS r8/m16/m32
	switch (operand_size) {
		case 4:
			X86_MNEMONIC_STR((cpu->output_str, "stosd"));
			break;
		case 2:
			X86_MNEMONIC_STR((cpu->output_str, "stosw"));
			break;
		case 1:
			X86_MNEMONIC_STR((cpu->output_str, "stosb"));
			break;
	}

	return 0;
}
int rep_movs_mnemonic(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	switch (operand_size) {
		case 4:
			X86_MNEMONIC_STR((cpu->output_str, "rep movsd"));
			break;
		case 2:
			X86_MNEMONIC_STR((cpu->output_str, "rep movsw"));
			break;
		case 1:
			X86_MNEMONIC_STR((cpu->output_str, "rep movsb"));
			break;
	}

	return 0;
}
int loop_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// Loop According to ECX Counter -  LOOP rel8
	signed char rel8 = (signed char)fetch_byte(cpu, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "loop %d", rel8));
	return 0;
}
int loope_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// Loop According to ECX Counter -  LOOPE rel8
	signed char rel8 = (signed char)fetch_byte(cpu, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "loope %d", rel8));
	return 0;
}
int loopne_mnemonic(X86_CPU* cpu, uint32_t counter)
{
	// Loop According to ECX Counter -  LOOPNE rel8
	signed char rel8 = (signed char)fetch_byte(cpu, &counter);
	X86_MNEMONIC_STR((cpu->output_str, "loopne %d", rel8));
	return 0;
}
int push_reg_mnemonic(X86_CPU* cpu, BYTE reg, uint32_t operand_size, uint32_t counter)
{
	X86_MNEMONIC_REG((cpu->output_str, "push %s", reg, operand_size));
	return 0;
}
int pop_reg_mnemonic(X86_CPU* cpu, BYTE reg, uint32_t operand_size, uint32_t counter)
{
	X86_MNEMONIC_REG((cpu->output_str, "pop %s", reg, operand_size));
	return 0;
}
int xchg_reg_mnemonic(X86_CPU* cpu, BYTE reg1, BYTE reg2, uint32_t operand_size, uint32_t counter)
{
	// xchg reg, reg
	X86_MNEMONIC_REG((cpu->output_str, "xchg %s, ", reg1, operand_size));
	X86_MNEMONIC_REG((cpu->output_str+strlen(cpu->output_str), "%s", reg2, operand_size));
	return 0;
}

int opcode_f3_mnemonic(X86_CPU* cpu, BYTE opcode, uint32_t operand_size, uint32_t counter)
{
	switch (opcode) {
		case 0xA5: // MOV WORD or DWORD
			rep_movs_mnemonic(cpu, operand_size, counter);
			return X86_CPU_ERROR_SUCCESS;

		case 0xA4: // MOV BYTE
			rep_movs_mnemonic(cpu, 1, counter);
			return X86_CPU_ERROR_SUCCESS;
	}

	return X86_CPU_ERROR_UD;
}
int opcode_0f_mnemonic(X86_CPU* cpu, BYTE opcode, uint32_t address_size, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter)
{
	X86_MOD_RM mode;

	switch (opcode) { // 0F xx
		case 0x00: // LLDT
			mode.byte = cpu->eip_ptr[counter++];
			if (mode.bits.reg == 0b010) { // 0f 00 /2 = LLDT r/m
				if (mode.bits.mod == 0b11) {
					return lldt_reg_mnemonic(cpu, mode.bits.rm, counter);
				}
				else {
					return lldt_imm_mnemonic(cpu, counter);
				}
			} break;

		case 0x01: // LGDT / LIDT
			mode.byte = cpu->eip_ptr[counter++];
			if (mode.bits.reg == 0b010) { // 0f 01 /2 = LGDT m
				return lgdt_mnemonic(cpu, &mode.bits, address_size, operand_size, segment_override_byte, counter);
			}
			else if (mode.bits.reg == 0b011) { // 0f 01 /3 = LIDT m
				return lidt_mnemonic(cpu, &mode.bits, address_size, operand_size, segment_override_byte, counter);
			} break;

		case 0x08: // invd
			return invd_mnemonic(cpu, counter);

		case 0x09: // wbinvd
			return wbinvd_mnemonic(cpu, counter);

		case 0x20: //mov r32, cr0-7
			mode.byte = cpu->eip_ptr[counter++];
			return mov_r32_cr_mnemonic(cpu, &mode.bits, counter);

		case 0x22: //mov cr0-7, r32
			mode.byte = cpu->eip_ptr[counter++];
			return mov_cr_r32_mnemonic(cpu, &mode.bits, counter);

		case 0x30: // WRMSR
			return wrmsr_mnemonic(cpu, counter);

		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F: // 16/32 bit JCC
			return jcc_mnemonic(cpu, opcode, operand_size, counter);

		case 0xB6: // MOVZX r16/r32, r/m8
			mode.byte = cpu->eip_ptr[counter++];
			return movzx_mnemonic(cpu, &mode.bits, 1, operand_size, counter);

		case 0xB7: // MOVZX r32, r/m16
			mode.byte = cpu->eip_ptr[counter++];
			return movzx_mnemonic(cpu, &mode.bits, 2, 4, counter);

		case 0xBE: // MOVSX r16/r32, r/m8
			mode.byte = cpu->eip_ptr[counter++];
			return movsx_mnemonic(cpu, &mode.bits, 1, operand_size, counter);

		case 0xBF: // MOVSX r32, r/m16
			mode.byte = cpu->eip_ptr[counter++];
			return movsx_mnemonic(cpu, &mode.bits, 2, 4, counter);
	}

	return X86_CPU_ERROR_UD;
}
int opcode_one_byte_mnemonic(X86_CPU* cpu, BYTE opcode, uint32_t operand_size, uint32_t counter)
{
	// check for one byte opcodes
	switch (opcode) {
		case 0x04:
			return add_imm_reg_mnemonic(cpu, REG_AL, 1, counter);
		case 0x05:
			return add_imm_reg_mnemonic(cpu, REG_EAX, operand_size, counter);

		case 0x0C:
			return or_imm_reg_mnemonic(cpu, REG_AL, 1, counter);
		case 0x0D:
			return or_imm_reg_mnemonic(cpu, REG_EAX, operand_size, counter);

		case 0x24:
			return and_imm_reg_mnemonic(cpu, REG_AL, 1, counter);
		case 0x25:
			return and_imm_reg_mnemonic(cpu, REG_EAX, operand_size, counter);

		case 0x2C:
			return sub_imm_reg_mnemonic(cpu, REG_AL, 1, counter);
		case 0x2D:
			return sub_imm_reg_mnemonic(cpu, REG_EAX, operand_size, counter);

		case 0x3C:
			return cmp_imm_reg_mnemonic(cpu, REG_AL, 1, counter);
		case 0x3D:
			return cmp_imm_reg_mnemonic(cpu, REG_EAX, operand_size, counter);

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47: // INC 16/32bit
			return inc_reg_mnemonic(cpu, (opcode & 0b111), operand_size, counter);

		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F: // DEC 16/32bit
			return dec_reg_mnemonic(cpu, (opcode & 0b111), operand_size, counter);

		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57: // PUSH 16/32bit
			return push_reg_mnemonic(cpu, (opcode & 0b111), operand_size, counter);

		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F: // POP 16/32bit
			return pop_reg_mnemonic(cpu, (opcode & 0b111), operand_size, counter);

		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F: // JMP condition 8bit		
			return jcc_mnemonic(cpu, opcode, 1, counter);

		case 0x8E: {
			X86_MOD_RM b;
			b.byte = cpu->eip_ptr[counter++];
			return mov_seg_r32_mnemonic(cpu, &b.bits, counter);
		}

		case 0x90:
			return nop_mnemonic(cpu, counter);

		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
			return xchg_reg_mnemonic(cpu, REG_EAX, (opcode & 0b111), operand_size, counter);

		case 0xA0:
			return move_ptr_reg_mnemonic(cpu, REG_AL, 1, counter);
		case 0xA1:
			return move_ptr_reg_mnemonic(cpu, REG_EAX, operand_size, counter);

		case 0xA4:
			return movs_mnemonic(cpu, 1, counter);
		case 0xA5:
			return movs_mnemonic(cpu, operand_size, counter);

		case 0xAA:
			return stos_mnemonic(cpu, 1, counter);
		case 0xAB:
			return stos_mnemonic(cpu, operand_size, counter);

		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7: // MOV 8bit		
			return move_imm_reg_mnemonic(cpu, (opcode & 0b111), 1, counter);

		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF: // MOV 16/32bit
			return move_imm_reg_mnemonic(cpu, (opcode & 0b111), operand_size, counter);

		case 0xE0:
			return loopne_mnemonic(cpu, counter);
		case 0xE1:
			return loope_mnemonic(cpu, counter);
		case 0xE2:
			return loop_mnemonic(cpu, counter);

		case 0xE4:
			return in_byte_imm_mnemonic(cpu, 1, counter);
		case 0xE5:
			return in_byte_imm_mnemonic(cpu, operand_size, counter);
		case 0xE6:
			return out_byte_imm_mnemonic(cpu, 1, counter);
		case 0xE7:
			return out_byte_imm_mnemonic(cpu, operand_size, counter);

		case 0xE9:
			return jmp_imm_rel_mnemonic(cpu, operand_size, counter);
		case 0xEA:
			return jmp_far_mnemonic(cpu, counter);
		case 0xEB:
			return jmp_imm_rel_mnemonic(cpu, 1, counter);
		case 0xEC:
			return in_byte_reg_mnemonic(cpu, 1, counter);
		case 0xED:
			return in_byte_reg_mnemonic(cpu, operand_size, counter);
		case 0xEE:
			return out_byte_reg_mnemonic(cpu, 1, counter);
		case 0xEF:
			return out_byte_reg_mnemonic(cpu, operand_size, counter);

		case 0xF4:
			return hlt_mnemonic(cpu, counter);

		case 0xFA:
			return cli_mnemonic(cpu, counter);
		case 0xFB:
			return sti_mnemonic(cpu, counter);
		case 0xFC:
			return cld_mnemonic(cpu, counter);
		case 0xFD:
			return std_mnemonic(cpu, counter);

		case 0xFE: {
			X86_MOD_RM b;
			b.byte = cpu->eip_ptr[counter++];
			if (b.bits.mod == 0b11) {
				switch (b.bits.reg) {
					case 0b000: // INC
						inc_reg_mnemonic(cpu, b.bits.rm, 1, counter);
						break;
					case 0b001: // DEC
						dec_reg_mnemonic(cpu, b.bits.rm, 1, counter);
						break;
					default:
						return 1; // not decoded.
				}
			}
		} return X86_CPU_ERROR_SUCCESS;

		default:
			return 1; // not decoded.
	}
}
int opcode_extended_mnemonic(X86_CPU* cpu, BYTE extended_opcode, char* dest_str, char* src_str)
{
	// 0x00 - 0x07 (0b000 - 0b111) ( 8 )
	switch (extended_opcode) {
		case 0b000: // ADD
			X86_MNEMONIC_STR((cpu->output_str, "add %s, %s", dest_str, src_str));
			break;

		case 0b001: // OR
			X86_MNEMONIC_STR((cpu->output_str, "or %s, %s", dest_str, src_str));
			break;

		case 0b010: // ADC
			X86_MNEMONIC_STR((cpu->output_str, "adc %s, %s", dest_str, src_str));
			break;

		case 0b011: // SBB
			X86_MNEMONIC_STR((cpu->output_str, "sbb %s, %s", dest_str, src_str));
			break;

		case 0b100: // AND
			X86_MNEMONIC_STR((cpu->output_str, "and %s, %s", dest_str, src_str));
			break;

		case 0b101: // SUB
			X86_MNEMONIC_STR((cpu->output_str, "sub %s, %s", dest_str, src_str));
			break;

		case 0b110: // XOR
			X86_MNEMONIC_STR((cpu->output_str, "xor %s, %s", dest_str, src_str));
			break;

		case 0b111: // CMP			
			X86_MNEMONIC_STR((cpu->output_str, "cmp %s, %s", dest_str, src_str));
			break;

		default:
			return X86_CPU_ERROR_UD;
	}
	return 0;
}
int opcode_mnemonic(X86_CPU* cpu, BYTE opcode, char* dest_str, char* src_str)
{
	// 0x00 - 0x3F (0b000000 - 0b111111) ( 64 )

	switch (opcode) {
		case 0b000000: // ADD
			X86_MNEMONIC_STR((cpu->output_str, "add %s, %s", dest_str, src_str));
			break;

		case 0b000010: // OR
			X86_MNEMONIC_STR((cpu->output_str, "or %s, %s", dest_str, src_str));
			break;

		case 0b001000: // AND
			X86_MNEMONIC_STR((cpu->output_str, "and %s, %s", dest_str, src_str));
			break;

		case 0b001010: // SUB
			X86_MNEMONIC_STR((cpu->output_str, "sub %s, %s", dest_str, src_str));
			break;

		case 0b001100: // XOR
			X86_MNEMONIC_STR((cpu->output_str, "xor %s, %s", dest_str, src_str));
			break;

		case 0b001110: // CMP
			X86_MNEMONIC_STR((cpu->output_str, "cmp %s, %s", dest_str, src_str));
			break;

		case 0b100001: // XCHG
			// set register or memory.
			X86_MNEMONIC_STR((cpu->output_str, "xchg %s, %s", dest_str, src_str));
			break;

		case 0b100010: // MOV
			X86_MNEMONIC_STR((cpu->output_str, "mov %s, %s", dest_str, src_str));
			break;

		case 0b111111: // JMP
			X86_MNEMONIC_STR((cpu->output_str, "jmp %s", src_str));
			break;

		default:
			return X86_CPU_ERROR_UD;
	}
	return 0;
}

void x86CPUGetRegisterMnemonic(char* buf, const char* fmt, uint32_t reg, uint32_t operand_size)
{
	switch (operand_size) {
		case 1:
			sprintf(buf, fmt, register_names_8bit[reg]);
			break;
		case 2:
			sprintf(buf, fmt, register_names_16bit[reg]);
			break;
		case 4:
			sprintf(buf, fmt, register_names_32bit[reg]);
			break;
	}
}
void x86CPUGetSegmentRegisterMnemonic(char* buf, const char* fmt, uint32_t reg)
{
	sprintf(buf, fmt, segment_names[reg]);
}
void x86CPUGetControlRegisterMnemonic(char* buf, uint32_t reg)
{
	sprintf(buf, "cr%d", reg);
}

int x86CPUGetMnemonic(X86_CPU* cpu, char* str)
{
	int result = 0;
	uint32_t counter = 0;

	if (cpu->eip_ptr == NULL) {
		return X86_CPU_ERROR_UD;
	}

	X86_OPCODE opcode = { 0 };
	PREFIX_BYTE_STRUCT prefix = { 0 };
	x86CPUFetchPrefixBytes(cpu, &prefix, &opcode, &counter);

	uint8_t operand_size = 0;
	uint8_t address_size = 0;
	x86CPUGetDefaultSize(cpu, &prefix, &operand_size, &address_size);
	
	if (prefix.byte_0f) {
		return opcode_0f_mnemonic(cpu, opcode.byte, address_size, operand_size, prefix.segment_override, counter);
	}
	else if (prefix.byte_f3) {
		return opcode_f3_mnemonic(cpu, opcode.byte, operand_size, counter);
	}
	else {
		result = opcode_one_byte_mnemonic(cpu, opcode.byte, operand_size, counter);
		if (result != 1) // return if decoded or error.
			return result;
	}

	if (opcode.bits.size == 0) {
		operand_size = 1;
	}

	// assume mod r/m byte
	X86_MOD_RM mode = { 0 };
	mode.byte = cpu->eip_ptr[counter++];

	if (opcode.byte == 0xC0) {
		// C0 = Eb, ib (operand1 = 8 regardless of operand_size) (operand2 = imm8)
		if (mode.byte >= 0xe0 && mode.byte <= 0xe7) {
			// SHL r/m 8 imm8
			shl_reg_mnemonic(cpu, mode.bits.rm, 1, counter);
			return X86_CPU_ERROR_SUCCESS;
		}
		else if (opcode.byte >= 0xe8 && opcode.byte <= 0xef) {
			// SHR r/m 8 imm8
			shr_reg_mnemonic(cpu, mode.bits.rm, 1, counter);
			return X86_CPU_ERROR_SUCCESS;
		}
	}
	else if (opcode.byte == 0xC1) {
		// C1 = Ev, ib (operand1 = 16 or 32 depending on operand_size) (operand2 = imm8)
		if (mode.byte >= 0xe0 && mode.byte <= 0xe7) {
			// SHL r/m 16/32 imm8
			shl_reg_mnemonic(cpu, mode.bits.rm, operand_size, counter);
			return X86_CPU_ERROR_SUCCESS;
		}
		else if (mode.byte >= 0xe8 && mode.byte <= 0xef) {
			// SHR r/m 16/32 imm8
			shr_reg_mnemonic(cpu, mode.bits.rm, operand_size, counter);
			return X86_CPU_ERROR_SUCCESS;
		}
	}

	BYTE direction = 0;
	char reg_str[32] = { 0 };
	char* src_str;
	char* dest_str;

	if (opcode.bits.op == 0b100000) {
		//immediate instruction
		direction = 0; // direction always const -> reg (cant be reg -> const)

		uint32_t const_size = operand_size;
		if (opcode.bits.direction == 1)
			const_size = 1; // constant is a signed 1 byte operand

		uint32_t value = fetch_memory(cpu, const_size, &counter);
		X86_MNEMONIC_STR((reg_str, "0x%x", value));
	}
	else {
		// r/m or SIB
		direction = opcode.bits.direction;
		X86_MNEMONIC_REG((reg_str, "%s", mode.bits.reg, operand_size));
	}

	// figure out the addressing mode.
	get_addressing_mnemonic(cpu, &mode.bits, address_size, operand_size, &counter);

	// figure out the addressing direction.
	if (direction == 0) {
		// reg -> r/m
		src_str = reg_str;
		dest_str = cpu->addressing_str;
	}
	else {
		// r/m -> reg
		src_str = cpu->addressing_str;
		dest_str = reg_str;
	}

	if (opcode.bits.op == 0b100000) {
		// 0x80 - 0x83 ( 1 0 0 0 0 0 X X )
		return opcode_extended_mnemonic(cpu, mode.bits.reg, dest_str, src_str);
	}
	else {
		// 0x00 - 0x3F ( X X X X X X )
		return opcode_mnemonic(cpu, opcode.bits.op, dest_str, src_str);
	}
}
