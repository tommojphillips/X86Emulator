// cpu.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "cpu_instruction.h"
#include "cpu_sib.h"

#include "type_defs.h"
#include "mem_tracking.h"

const char* register_names_32bit[] = {
	"eax",
	"ecx",
	"edx",
	"ebx",
	"esp",
	"ebp",
	"esi",
	"edi",
};
const char* register_names_16bit[] = {
	"ax",
	"cx",
	"dx",
	"bx",
	"sp",
	"bp",
	"si",
	"di",
};
const char* register_names_8bit[] = {
	"al",
	"cl",
	"dl",
	"bl",
	"ah",
	"ch",
	"dh",
	"bh",
};
const char* segment_names[] = {
	"es",
	"cs",
	"ss",
	"ds",
	"fs",
	"gs",
};

void error_out(char* fmt, BYTE opcode, BYTE op, char* str);

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
void x86CPULoadSegmentDescriptor(X86_CPU* cpu, int segment, uint16_t selector)
{
	// Calculate the address of the segment descriptor in the GDT
	uint32_t descriptor_address = cpu->gdt.base + (selector & ~0x7);

	// Read the segment descriptor
	uint64_t descriptor = *(uint64_t*)x86GetCPUMemoryPtr(cpu, descriptor_address);

	// Extract the base, limit, and access rights from the descriptor
	uint32_t base = ((descriptor >> 16) & 0xFFFF) | ((descriptor >> 32) & 0xFF0000) | ((descriptor >> 56) & 0xFF000000);
	uint32_t limit = (descriptor & 0xFFFF) | ((descriptor >> 48) & 0xF0000);
	uint8_t access = (descriptor >> 40) & 0xFF;
	uint8_t flags = (descriptor >> 52) & 0xF;

	// Update the segment register
	cpu->segment_registers[segment] = selector;

	// Update the segment descriptor cache
	/*cpu->segment_descriptor_cache[segment].base = base;
	cpu->segment_descriptor_cache[segment].limit = limit;
	cpu->segment_descriptor_cache[segment].access = access;
	cpu->segment_descriptor_cache[segment].flags = flags;*/
}
int x86CPUGetMemonic(X86_CPU* cpu, char* str)
{
	return 0;
}

void set_memory_value(X86_CPU* cpu, uint32_t address, uint32_t operand_size, uint32_t value)
{
	void* ptr = x86GetCPUMemoryPtr(cpu, address);

	if (ptr == NULL)
		return;

	switch (operand_size) {
		case 1:
			*(uint8_t*)ptr = (uint8_t)value;
		case 2:
			*(uint16_t*)ptr = (uint16_t)value;
			break;
		case 4:
			*(uint32_t*)ptr = (uint32_t)value;
			break;
	}
}

/* opcodes */

int move_imm_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// mov reg, imm8
	uint32_t imm = x86CPUFetchMemory(cpu, operand_size, &counter);
	x86CPUSetRegister(cpu, reg, operand_size, imm);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "mov %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int move_ptr_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// mov reg, [address]
	uint32_t address = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint32_t value = x86CPUReadMemory(cpu, address, operand_size);
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "mov %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "[0x%x]", address));
	return 0;
}
int inc_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// inc reg
	uint32_t v = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_INC, v, 1, operand_size, &v);
	x86CPUSetRegister(cpu, reg, operand_size, v);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "inc %s", reg, operand_size));
	return 0;
}
int dec_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// dec reg
	uint32_t v = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_DEC, v, 1, operand_size, &v);
	x86CPUSetRegister(cpu, reg, operand_size, v);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "dec %s", reg, operand_size));
	return 0;
}
int shr_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// shr reg, imm
	BYTE imm8 = x86CPUFetchByte(cpu, &counter);
	uint32_t value = x86CPUGetRegister(cpu, reg, operand_size);
	value = value >> imm8;
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "shr %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "%d", imm8));
	return 0;
}
int shl_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// shl reg, imm
	BYTE imm8 = x86CPUFetchByte(cpu, &counter);
	uint32_t value = x86CPUGetRegister(cpu, reg, operand_size);
	value = value << imm8;
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "shl %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "%d", imm8));
	return 0;
}
int rep_movs(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	uint32_t value;
	X86_REG32 ecx = cpu->registers[REG_ECX].r32;
	X86_REG32 esi = cpu->registers[REG_ESI].r32;
	X86_REG32 edi = cpu->registers[REG_EDI].r32;

	for (; ecx > 0; ecx -= 1) {
		value = x86CPUReadMemory(cpu, esi, operand_size);
		set_memory_value(cpu, edi, operand_size, value);
		
		esi += operand_size;
		edi += operand_size;
	}

	cpu->registers[REG_ECX].r32 = ecx;
	cpu->registers[REG_ESI].r32 = esi;
	cpu->registers[REG_EDI].r32 = edi;
	cpu->eip += counter;

	switch (operand_size) {
		case 4:
			MNEMONIC_STR((cpu->output_str, "rep movsd"));
			break;
		case 2:
			MNEMONIC_STR((cpu->output_str, "rep movsw"));
			break;
		case 1:
			MNEMONIC_STR((cpu->output_str, "rep movsb"));
			break;
	}

	return 0;
}
int cld(X86_CPU* cpu, uint32_t counter)
{
	// clear direction flag
	cpu->eflags.DF = 0;
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "cld"));
	return 0;
}
int std(X86_CPU* cpu, uint32_t counter)
{
	// set direction flag
	cpu->eflags.DF = 1;
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "std"));
	return 0;
}
int cli(X86_CPU* cpu, uint32_t counter)
{
	// clear interrupt flag
	cpu->eflags.IF = 0;
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "cli"));
	return 0;
}
int sti(X86_CPU* cpu, uint32_t counter)
{
	// set interrupt flag
	cpu->eflags.IF = 1;
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "sti"));
	return 0;
}
int hlt(X86_CPU* cpu, uint32_t counter)
{
	// hlt
	cpu->hlt = 1;
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "hlt"));
	return 0;
}
int jmp_imm_rel(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	int offset = 0;
	switch (operand_size) {
		case 1:
			offset = (signed char)x86CPUFetchByte(cpu, &counter);
			break;
		case 2:
			offset = (short)x86CPUFetchWord(cpu, &counter);
			break;
		case 4:
			offset = (int)x86CPUFetchDword(cpu, &counter); 
			break;
	}
	cpu->eip += counter + offset;
	MNEMONIC_STR((cpu->output_str, "jmp %d", offset));
	return 0;
}
int jmp_far(X86_CPU* cpu, uint32_t counter)
{
	uint32_t address = x86CPUFetchDword(cpu, &counter);
	uint16_t selector = x86CPUFetchWord(cpu, &counter);

	/* update CS register and reload segment descriptor */
	x86CPULoadSegmentDescriptor(cpu, SEG_CS, selector);

	/* update mode */
	switch (cpu->mode) {
		case CPU_REAL_MODE:
			if ((cpu->control_registers[0] & 0b1) == 1) {
				// REAL TO PROTECTED
				cpu->mode = CPU_PROTECTED_MODE;
			}
			break;
		case CPU_PROTECTED_MODE:
			if ((cpu->control_registers[0] & 0b1) == 0) {
				// PROTECTED TO REAL
				cpu->mode = CPU_REAL_MODE;
			}
			break;
	}
		
	cpu->eip = address;

	MNEMONIC_STR((cpu->output_str, "jmp 0x%04x:0x%08x", selector, address));
	return 0;
}
int jcc(X86_CPU* cpu, BYTE opcode, uint32_t operand_size, uint32_t counter)
{
	// jump condition

	X86_EFLAGS eflags = cpu->eflags;
	int offset = 0;

	switch (operand_size) {
		case 1:
			offset = (signed char)x86CPUFetchByte(cpu, &counter);
			break;
		case 2:
			offset = (short)x86CPUFetchWord(cpu, &counter);
			break;
		case 4:
			offset = (int)x86CPUFetchDword(cpu, &counter);
			break;
	}

	// last 4 bits  of the opcode are the conditional test.
	switch ((opcode & 0x0f)) {
		case CONDITIONAL_TEST_OVERFLOW:
			if (eflags.OF == 1)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jo %d", offset));
			break;
		case CONDITIONAL_TEST_NO_OVERFLOW:
			if (eflags.OF == 0)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jno %d", offset));
			break;
		case CONDITIONAL_TEST_CARRY:
			if (eflags.CF == 1)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jc %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_CARRY:
			if (eflags.CF == 0)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jnc %d", offset));
			break;
		case CONDITIONAL_TEST_EQUAL_ZERO:
			if (eflags.ZF == 1)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jz %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_EQUAL_ZERO:
			if (eflags.ZF == 0)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jnz %d", offset));
			break;
		case CONDITIONAL_TEST_BELOW_OR_EQUAL:
			if (eflags.CF == 1 || eflags.ZF == 1)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jbe %d", offset));
			break;
		case CONDITIONAL_TEST_ABOVE:
			if (eflags.CF == 0 && eflags.ZF == 0)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "ja %d", offset));
			break;
		case CONDITIONAL_TEST_SIGN:
			if (eflags.SF == 1)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "js %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_SIGN:
			if (eflags.SF == 0)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jns %d", offset));
			break;
		case CONDITIONAL_TEST_PARITY:
			if (eflags.PF == 1)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jp %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_PARITY:
			if (eflags.PF == 0)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jnp %d", offset));
			break;
		case CONDITIONAL_TEST_LESS:
			if (eflags.SF != eflags.OF)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jl %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_LESS:
			if (eflags.SF == eflags.OF)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jge %d", offset));
			break;
		case CONDITIONAL_TEST_LESS_OR_EQUAL:
			if (eflags.ZF == 1 || eflags.SF != eflags.OF)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jle %d", offset));
			break;
		case CONDITIONAL_TEST_NOT_LESS_OR_EQUAL:
			if (eflags.ZF == 0 && eflags.SF == eflags.OF)
				counter += offset;
			MNEMONIC_STR((cpu->output_str, "jg %d", offset));
			break;
	}

	//inc
	cpu->eip += counter;

	return 0;
}
int cmp_imm_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// cmp value in reg to imm
	uint32_t imm = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint32_t reg_v = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_CMP, imm, reg_v, operand_size, NULL);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "cmp %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int in_byte_reg(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// input byte/word/dword from I/O port in DX into AL/AX/EAX.
	WORD address = (WORD)x86CPUGetRegister(cpu, REG_DX, 2);
	uint32_t value = x86CPUGetIOByte(cpu, address);
	x86CPUSetRegister(cpu, REG_EAX, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "in %s, dx", REG_EAX, operand_size));
	return 0;
}
int out_byte_reg(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// output byte/word/dword in AL/AX/EAX to I/O port address in DX.
	WORD address = (WORD)x86CPUGetRegister(cpu, REG_DX, 2);
	uint32_t value = x86CPUGetRegister(cpu, REG_EAX, operand_size);
	x86CPUSetIOByte(cpu, operand_size, address, value);
	cpu->eip += counter;		
	MNEMONIC_REG((cpu->output_str, "out dx, %s", REG_EAX, operand_size));
	return 0;
}
int in_byte_imm(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// input byte/word/dword from imm8 I/O port address into AL/AX/EAX
	BYTE imm = x86CPUFetchByte(cpu, &counter);
	uint32_t value = x86CPUGetIOByte(cpu, imm);
	x86CPUSetRegister(cpu, REG_EAX, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "in %s, ", REG_EAX, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int out_byte_imm(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// output byte/word/dword in AL/AX/EAX to I/O port address imm8.
	BYTE imm = x86CPUFetchByte(cpu, &counter);
	uint32_t value = x86CPUGetRegister(cpu, REG_EAX, operand_size);
	x86CPUSetIOByte(cpu, operand_size, imm, value);
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "out 0x%x, ", imm));
	MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), "%s", REG_EAX, operand_size));
	return 0;
}
int movzx(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t src_size, uint32_t dest_size, uint32_t counter)
{
	// B6 MOVZX r16, r/m8
	// B6 MOVZX r32, r/m8
	// B7 MOVZX r32, r/m16

	uint32_t value = 0;
	get_addressing_mode(cpu, mode, dest_size, src_size, NULL, &value, NULL, &counter);
	x86CPUSetRegister(cpu, mode->reg, dest_size, value);
	cpu->eip += counter;

	MNEMONIC_REG((cpu->output_str, "movzx %s, ", mode->reg, dest_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), cpu->addressing_str));

	return 0;
}
int and_imm_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// logical AND reg imm
	uint32_t imm = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint32_t value = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_AND, value, imm, operand_size, &value);
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "and %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int or_imm_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// logical OR reg imm
	uint32_t imm = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint32_t value = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_OR, value, imm, operand_size, &value);
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "or %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int add_imm_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// add reg imm
	uint32_t imm = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint32_t value = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_ADD, value, imm, operand_size, &value);
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;
	MNEMONIC_REG((cpu->output_str, "add %s, ", reg, operand_size));
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "0x%x", imm));
	return 0;
}
int lgdt(X86_CPU* cpu, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter) {
	// Load the GDT register
	uint32_t address = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint16_t limit = x86CPUReadWord(cpu, address);
	uint32_t base = x86CPUReadDword(cpu, address + 2);

	cpu->gdt.limit = (uint16_t)limit;
	cpu->gdt.base = base;
	if (operand_size == 2) {
		cpu->gdt.base &= 0x00FFFFFF;
	}

	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "lgdt "));
	MNEMONIC_SEG_OVERIDE(segment_override_byte);
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "[0x%x]", address));
	return 0;
}
int lidt(X86_CPU* cpu, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter) {
	// Load the IDT register
	uint32_t address = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint16_t limit = x86CPUReadWord(cpu, address);
	uint32_t base = x86CPUReadDword(cpu, address + 2);

	cpu->idt.limit = (uint16_t)limit; 
	cpu->gdt.base = base;
	if (operand_size == 2) {
		cpu->gdt.base &= 0x00FFFFFF;
	}

	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "lidt "));
	MNEMONIC_SEG_OVERIDE(segment_override_byte);
	MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "[0x%x]", address));
	return 0;
}
int mov_seg_r32(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t counter)
{
	// MOV seg, r32
	uint32_t value = x86CPUGetRegister(cpu, mode->rm, 4);
	cpu->segment_registers[mode->reg] = value;
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "mov "));
	MNEMONIC_SEG((cpu->output_str + strlen(cpu->output_str), "%s, ", mode->reg));
	MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), "%s", mode->rm, 4));
	return 0;
}
int mov_cr_r32(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t counter)
{
	// MOV cr0-7, r32
	uint32_t value = x86CPUGetRegister(cpu, mode->reg, 4);
	cpu->control_registers[mode->rm] = value;
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "mov "));
	MNEMONIC_CON((cpu->output_str + strlen(cpu->output_str), mode->rm));
	MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), ", %s", mode->reg, 4));
	return 0;
}
int mov_r32_cr(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t counter)
{
	// MOV r32, cr0-7
	uint32_t value = cpu->control_registers[mode->reg];
	x86CPUSetRegister(cpu, mode->rm, 4, value);
	cpu->eip += counter;
	MNEMONIC_STR((cpu->output_str, "mov "));
	MNEMONIC_REG((cpu->output_str + strlen(cpu->output_str), "%s, ", mode->reg, 4));
	MNEMONIC_CON((cpu->output_str + strlen(cpu->output_str), mode->rm));
	return 0;
}

int decode_one_byte_opcode(X86_CPU* cpu, BYTE opcode, uint32_t operand_size, uint32_t counter)
{
	// MOV 8bit
	if (opcode >= 0xB0 && opcode <= 0xB7) { 
		move_imm_reg(cpu, (opcode & 0b111), 1, counter);
		return 0;
	}

	// MOV 16/32bit
	if (opcode >= 0xB8 && opcode <= 0xBF) { 
		move_imm_reg(cpu, (opcode & 0b111), (4 - operand_size), counter);
		return 0;
	}
	
	// INC 16/32bit
	if (opcode >= 0x40 && opcode <= 0x47) { 
		inc_reg(cpu, (opcode & 0b111), (4 - operand_size), counter);
		return 0;
	}

	// DEC 16/32bit
	if (opcode >= 0x48 && opcode <= 0x4F) { 
		dec_reg(cpu, (opcode & 0b111), (4 - operand_size), counter);
		return 0;
	}

	// JMP condition 8bit
	if (opcode >= 0x70 && opcode <= 0x7F) {
		jcc(cpu, opcode, 1, counter);
		return 0;
	}

	// check for one byte opcodes
	switch (opcode) {
		case 0x05:
			add_imm_reg(cpu, REG_EAX, (4 - operand_size), counter);
			break;

		case 0x0C:
			or_imm_reg(cpu, REG_EAX, 1, counter);
			break;

		case 0x24:
			and_imm_reg(cpu, REG_EAX, 1, counter);
			break;		
		case 0x25:
			and_imm_reg(cpu, REG_EAX, (4 - operand_size), counter);
			break;

		case 0xA1:
			move_ptr_reg(cpu, REG_EAX, (4 - operand_size), counter);
			break;

		case 0x3C:
			cmp_imm_reg(cpu, REG_EAX, 1, counter);
			break;
		case 0x3D:
			cmp_imm_reg(cpu, REG_EAX, (4 - operand_size), counter);
			break;

		case 0xE4:
			in_byte_imm(cpu, 1, counter);
			break;
		case 0xE5:
			in_byte_imm(cpu, (4 - operand_size), counter);
			break;

		case 0xE6:
			out_byte_imm(cpu, 1, counter);
			break;
		case 0xE7:
			out_byte_imm(cpu, (4 - operand_size), counter);
			break;

		case 0xE9:
			jmp_imm_rel(cpu, (4 - operand_size), counter);
			break;
		case 0xEA:
			jmp_far(cpu, counter);
			break;
		case 0xEB:
			jmp_imm_rel(cpu, 1, counter);
			break;
		
		case 0xEC:
			in_byte_reg(cpu, 1, counter);
			break;
		case 0xED:
			in_byte_reg(cpu, (4 - operand_size), counter);
			break;

		case 0xEE:
			out_byte_reg(cpu, 1, counter);
			break;
		case 0xEF:
			out_byte_reg(cpu, (4 - operand_size), counter);
			break;

		case 0xF4:
			hlt(cpu, counter);
			return X86_CPU_ERROR_HLT;

		case 0xFA:
			cli(cpu, counter);
			break;
		case 0xFB:
			sti(cpu, counter);
			break;
		case 0xFC:
			cld(cpu, counter);
			break;
		case 0xFD:
			std(cpu, counter);
			break;
		
		case 0xFE: {
				BYTE b = x86CPUFetchByte(cpu, &counter);
				X86_MOD_RM* mode = (X86_MOD_RM*)&b;
				if (mode->mod == 0b11) {
					switch (mode->reg) {
						case 0b000: // INC
							inc_reg(cpu, mode->rm, 1, counter);
							break;
						case 0b001: // DEC
							dec_reg(cpu, mode->rm, 1, counter);
							break;
						default:
							return 1;
					}
				}
			} break;

		case 0x90:
			cpu->eip += counter;
			MNEMONIC_STR((cpu->output_str, "nop"));
			break;

		default:
			return 1;
	}

	return X86_CPU_ERROR_SUCCESS;
}
int decode_extended_opcode(X86_CPU* cpu, BYTE extended_opcode, uint32_t dest_v, uint32_t src_v, uint32_t operand_size, char* dest_str, char* src_str, uint32_t* instr_result, uint32_t counter)
{
	// 0x00 - 0x07 (0b000 - 0b111) ( 8 )
	switch (extended_opcode) {
		case 0b000: // ADD
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_ADD, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "add %s, %s", dest_str, src_str));
			break;

		case 0b001: // OR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_OR, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "or %s, %s", dest_str, src_str));
			break;

		case 0b010: // ADC
			//x86Alu(&cpu->eflags, INSTRUCTION_TYPE_ADC, dest_v, src_v, operand_size, instr_result);
			//MNEMONIC_STR((cpu->output_str, "adc %s, %s", dest_str, src_str));
			goto OP_EXT_UNK;

		case 0b011: // SBB
			//x86Alu(&cpu->eflags, INSTRUCTION_TYPE_SBB, dest_v, src_v, operand_size, instr_result);
			//MNEMONIC_STR((cpu->output_str, "sbb %s, %s", dest_str, src_str));
			goto OP_EXT_UNK;

		case 0b100: // AND
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_AND, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "and %s, %s", dest_str, src_str));
			break;

		case 0b101: // SUB
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_SUB, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "sub %s, %s", dest_str, src_str));
			break;

		case 0b110: // XOR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_XOR, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "xor %s, %s", dest_str, src_str));
			break;

		case 0b111: // CMP
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_CMP, dest_v, src_v, operand_size, instr_result);
			cpu->eip += counter;
			MNEMONIC_STR((cpu->output_str, "cmp %s, %s", dest_str, src_str));
			return X86_CPU_ERROR_DECODED; // cmp only sets flags.

		default:
		OP_EXT_UNK:
			error_out("OP EXT: 80 0x%02x ", extended_opcode, extended_opcode, cpu->output_str);
			return X86_CPU_ERROR_UD;
	}
	return X86_CPU_ERROR_SUCCESS;
}
int decode_opcode(X86_CPU* cpu, BYTE opcode, uint32_t dest_v, uint32_t src_v, uint32_t operand_size, char* dest_str, char* src_str, uint32_t* instr_result, uint32_t counter)
{
	// 0x00 - 0x3F (0b000000 - 0b111111) ( 64 )
	
	switch (opcode) {
		case 0b000000: // ADD
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_ADD, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "add %s, %s", dest_str, src_str));
			break;

		case 0b000010: // OR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_OR, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "or %s, %s", dest_str, src_str));
			break;

		case 0b001000: // AND
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_AND, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "and %s, %s", dest_str, src_str));
			break;

		case 0b001010: // SUB
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_SUB, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "sub %s, %s", dest_str, src_str));
			break;

		case 0b001100: // XOR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_XOR, dest_v, src_v, operand_size, instr_result);
			MNEMONIC_STR((cpu->output_str, "xor %s, %s", dest_str, src_str));
			break;

		case 0b001110: // CMP
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_CMP, dest_v, src_v, operand_size, instr_result);
			cpu->eip += counter;
			MNEMONIC_STR((cpu->output_str, "cmp %s, %s", dest_str, src_str));
			return X86_CPU_ERROR_DECODED; // cmp only sets flags.

		case 0b100010: // MOV
			*instr_result = src_v;
			MNEMONIC_STR((cpu->output_str, "mov %s, %s", dest_str, src_str));
			break;

		case 0b111111: // JMP
			cpu->eip = src_v;
			MNEMONIC_STR((cpu->output_str, "jmp %s", src_str));
			return X86_CPU_ERROR_DECODED;

		default:
			error_out("0x%02x -> 0x%02x ", opcode, opcode, cpu->output_str);
			return X86_CPU_ERROR_UD;
	}
	return X86_CPU_ERROR_SUCCESS;
}

void error_out(char* fmt, BYTE opcode, BYTE op, char* str)
{

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c "
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

	sprintf(str, fmt, opcode, op);
	sprintf(str+strlen(str), BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(opcode));
}

int x86CPUExecute(X86_CPU* cpu)
{
	// vars
#if 1
	uint32_t operand_size = 0;
	uint32_t addressing_size = 0;

	typedef union _OPCODE_COUP {
		X86_OPCODE bits;
		BYTE byte;
	} OPCODE_COUP;
	OPCODE_COUP opcode = {0};


	typedef union _MOD_RM_COUP {
		X86_MOD_RM bits;
		BYTE byte;
	} MOD_RM_COUP;
	MOD_RM_COUP mode = { 0 };

	BYTE prefix_bytes[15] = { 0 };
	uint32_t src_v = 0;
	INSTRUCTION_RM src_rm = 0;
	uint32_t dest_v = 0;
	uint32_t dest_reg = 0;
	INSTRUCTION_RM dest_rm = 0;
	uint32_t rm_v = 0;
	uint32_t rm_mem = 0;
	INSTRUCTION_RM rm_type = 0;
	uint32_t reg_v = 0;
	uint32_t reg = 0;
	INSTRUCTION_RM reg_type = 0;
	BYTE direction = 0;

	char reg_str[32] = {0};
	char* src_str; // ptr to whichever str turns out to be src.
	char* dest_str; // ptr to whichever str turns out to be dest.

	uint32_t counter = 0;

	int result;

	bool is_prefixed_66 = false; // operand size override
	bool is_prefixed_67 = false; // address size override
	bool is_prefixed_0f = false;
	bool is_prefixed_f3 = false; // rep

	BYTE segment_override_byte = 0;
#endif

	cpu->prev_eip = cpu->eip;

	// fetch prefix bytes and first opcode byte.
	for (int i = 0; i < 15; i++) {
		prefix_bytes[i] = x86CPUFetchByte(cpu, &counter);
		switch (prefix_bytes[i]) {
			case 0x66:
				operand_size = 2;
				is_prefixed_66 = true;
				continue;
			case 0x67:
				is_prefixed_67 = true;
				continue;
			case 0xF3:
				is_prefixed_f3 = true;
				continue;
			case 0x0F:
				is_prefixed_0f = true;
				continue;

			/* segment overrides */
			case 0x26: // es
			case 0x2E: // cs
			case 0x36: // ss
			case 0x3E: // ds
			case 0x64: // fs
			case 0x65: // gs
				segment_override_byte = prefix_bytes[i];				
				continue;
		}
		opcode.byte = prefix_bytes[i];
		prefix_bytes[i] = 0x00;
		break;
	}
	
	// check for opcodes prefixed by '0f'
	if (is_prefixed_0f) {

		// 16/32 bit JCC
		if (opcode.byte <= 0x80 && opcode.byte >= 0x8F) {
			jcc(cpu, opcode.byte, (4 - operand_size), counter);
			return X86_CPU_ERROR_SUCCESS;
		}

		// WRMSR
		if (opcode.byte == 0x30) {
			cpu->eip += counter;
			MNEMONIC_STR((cpu->output_str, "wrmsr"));
			return X86_CPU_ERROR_SUCCESS;
		}

		// LGDT, LIDT
		if (opcode.byte == 0x01) {
			mode.byte = x86CPUFetchByte(cpu, &counter);
			if (mode.bits.reg == 0b010) { // 0f 01 /2 = LGDT
				lgdt(cpu, operand_size, segment_override_byte, counter);
				return X86_CPU_ERROR_SUCCESS;
			}
			else if (mode.bits.reg == 0b011) { // 0f 01 /3 = LIDT
				lidt(cpu, operand_size, segment_override_byte, counter);
				return X86_CPU_ERROR_SUCCESS;
			}
			else {
				return X86_CPU_ERROR_UD;
			}
		}
	}
	// check for opcodes prefixed by 'f3'
	else if (is_prefixed_f3) {
		
		switch (opcode.byte) {
			case 0xA5: // MOV WORD or DWORD
				rep_movs(cpu, (4 - operand_size), counter);
				return X86_CPU_ERROR_SUCCESS;

			case 0xA4: // MOV BYTE
				rep_movs(cpu, 1, counter);
				return X86_CPU_ERROR_SUCCESS;
		}
	} 
	// check for one byte opcodes
	else {
		result = decode_one_byte_opcode(cpu, opcode.byte, operand_size, counter);
		if (result == 0)
			return X86_CPU_ERROR_SUCCESS;
		else if (result != 1)
			return result;
	}

	if (opcode.bits.size == 0) {
		operand_size = 1;
	}
	else {
		if (is_prefixed_66)
			operand_size = 2;
		else
			operand_size = 4;
	}

	if (is_prefixed_67)
		addressing_size = 2;
	else
		addressing_size = 4;
		
	// assume mod r/m byte
	mode.byte = x86CPUFetchByte(cpu, &counter);

	if (opcode.byte == 0x8E) {
		// MOV seg, r32
		mov_seg_r32(cpu, &mode.bits, counter);
		return X86_CPU_ERROR_SUCCESS;
	}

	if (opcode.byte == 0xC0) {
		// C0 = Eb, ib (operand1 = 8 regardless of operand_size) (operand2 = imm8)
		if (mode.byte >= 0xe0 && mode.byte <= 0xe7) {
			// SHL r/m 8 imm8
			shl_reg(cpu, mode.bits.rm, 1, counter);
			return X86_CPU_ERROR_SUCCESS;
		}
		else if (opcode.byte >= 0xe8 && opcode.byte <= 0xef) {
			// SHR r/m 8 imm8
			shr_reg(cpu, mode.bits.rm, 1, counter);
			return X86_CPU_ERROR_SUCCESS;
		}
	}
	else if (opcode.byte == 0xC1) {
		// C1 = Ev, ib (operand1 = 16 or 32 depending on operand_size) (operand2 = imm8)
		if (mode.byte >= 0xe0 && mode.byte <= 0xe7) {
			// SHL r/m 16/32 imm8
			shl_reg(cpu, mode.bits.rm, operand_size, counter);
			return X86_CPU_ERROR_SUCCESS;
		} 
		else if (mode.byte >= 0xe8 && mode.byte <= 0xef) {
			// SHR r/m 16/32 imm8
			shr_reg(cpu, mode.bits.rm, operand_size, counter);
			return X86_CPU_ERROR_SUCCESS;
		}
	}

	if (is_prefixed_0f) {
		switch (opcode.byte) { // 0F xx
			case 0xB6:
				// MOVZX r16, r/m8
				// MOVZX r32, r/m8
				movzx(cpu, &mode.bits, 1, (is_prefixed_66 ? 2 : 4), counter);
				return X86_CPU_ERROR_SUCCESS;

			case 0xB7:
				// MOVZX r32, r/m16
				movzx(cpu, &mode.bits, 2, 4, counter);
				return X86_CPU_ERROR_SUCCESS;

			case 0x20: //mov r32, cr0-7
				mov_r32_cr(cpu, &mode.bits, counter);
				return X86_CPU_ERROR_SUCCESS;

			case 0x22: //mov cr0-7, r32
				mov_cr_r32(cpu, &mode.bits, counter);
				return X86_CPU_ERROR_SUCCESS;
		}
	}

	if (opcode.bits.op == 0b100000) {
		//immediate instruction
		reg = mode.bits.rm;
		reg_type = INSTRUCTION_RM_MEM;
		direction = 0; // direction always const -> reg (cant be reg -> const)

		uint32_t const_size;
		if (opcode.bits.direction == 1)	// constant is a signed 1 byte operand
			const_size = 1;
		else // constant is the same size specified by s.
			const_size = operand_size;

		reg_v = x86CPUFetchMemory(cpu, const_size, &counter);
		MNEMONIC_STR((reg_str, "0x%x", reg_v));
	}
	else {
		// r/m or SIB 
		reg = mode.bits.reg;
		reg_type = INSTRUCTION_RM_REG;
		direction = opcode.bits.direction;

		reg_v = x86CPUGetRegister(cpu, reg, operand_size);
		MNEMONIC_REG((reg_str, "%s", reg, operand_size));
	}
	
	// figure out the addressing mode.
	get_addressing_mode(cpu, &mode.bits, addressing_size, operand_size, &rm_mem, &rm_v, &rm_type, &counter);
		
	// figure out the addressing direction.
	if (direction == 0) {
		// reg -> r/m

		src_v = reg_v;
		src_rm = reg_type;

		dest_v = rm_v;
		dest_reg = mode.bits.rm;
		dest_rm = rm_type;

		src_str = reg_str;
		dest_str = cpu->addressing_str;
	}
	else {
		// r/m -> reg

		src_v = rm_v;
		src_rm = rm_type;

		dest_v = reg_v;
		dest_reg = reg;
		dest_rm = reg_type;

		src_str = cpu->addressing_str;
		dest_str = reg_str;
	}

	// perform instruction.

	uint32_t instr_result = 0;
	if (is_prefixed_0f) {
		// 0x0F 0xXX
		switch (opcode.bits.op) {
			case 0b101101: // MOVZX
				instr_result = src_v;
				MNEMONIC_STR((cpu->output_str, "movzx %s, %s", dest_str, src_str));
				break;

			default:
				error_out("0F 0x%02x -> 0x%02x ", opcode.byte, opcode.bits.op, cpu->output_str);
				return X86_CPU_ERROR_UD;
		}
	}
	else {
		if (opcode.bits.op == 0b100000) {
			// 0x80 - 0x83 ( 1 0 0 0 0 0 X X )
			result = decode_extended_opcode(cpu, mode.bits.reg, dest_v, src_v, operand_size, dest_str, src_str, &instr_result, counter);
			if (result != 0) {
				if (result == X86_CPU_ERROR_DECODED)
					return X86_CPU_ERROR_SUCCESS;
				else
					return result;
			}
		}
		else {
			// 0x00 - 0x3F ( X X X X X X )
			result = decode_opcode(cpu, opcode.bits.op, dest_v, src_v, operand_size, dest_str, src_str, &instr_result, counter);
			if (result != 0) {
				if (result == X86_CPU_ERROR_DECODED)
					return X86_CPU_ERROR_SUCCESS;
				else
					return result;
			}
		}
	}

	// set register or memory.
	switch (dest_rm) {
		case INSTRUCTION_RM_MEM:
			set_memory_value(cpu, rm_mem, operand_size, instr_result);
			break;
		case INSTRUCTION_RM_REG:			
			x86CPUSetRegister(cpu, dest_reg, operand_size, instr_result);
			break;
	}

	// inc
	cpu->eip += counter;

	return 0;
}

void x86CPUDumpRegisters(X86_CPU* cpu) 
{
	printf(	"\n\tEAX: %08x\tEBX: %08x\tECX: %08x\tEDX: %08x" \
			"\n\tESI: %08x\tEDI: %08x\tEBP: %08x\tESP: %08x"\
			"\n\tCF=%x\tZF=%x\tSF=%x\tOF=%x\tPF=%x"\
			"\n",
		cpu->registers[REG_EAX].r32, cpu->registers[REG_EBX].r32, cpu->registers[REG_ECX].r32, cpu->registers[REG_EDX].r32,
		cpu->registers[REG_ESI].r32, cpu->registers[REG_EDI].r32, cpu->registers[REG_EBP].r32, cpu->registers[REG_ESP].r32,
		cpu->eflags.CF, cpu->eflags.ZF, cpu->eflags.SF, cpu->eflags.OF, cpu->eflags.PF);
};
