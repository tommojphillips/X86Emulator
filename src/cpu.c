// cpu.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "cpu_instruction.h"
#include "cpu_memory.h"
#include "cpu_sib.h"

#include "type_defs.h"
#include "mem_tracking.h"

void error_out(X86_CPU* cpu, uint32_t counter)
{
	for (uint32_t i = 0; i < counter; ++i) {
		sprintf(cpu->output_str + (i * 3), "%02X ", cpu->eip_ptr[i]);
	}
}

/*MEMORY*/
int x86InitMemory(X86_MEMORY* mem, uint32_t rom_base, uint32_t rom_end, uint32_t ram_base, uint32_t ram_end) 
{
	// ram
	mem->ram_base = ram_base;
	mem->ram_end = ram_end;
	mem->ram_size = (ram_end - ram_base + 1);
	mem->ram = (BYTE*)malloc(mem->ram_size);

	// rom
	mem->rom_base = rom_base;
	mem->rom_end = rom_end;
	mem->rom_size = (rom_end - rom_base + 1);
	mem->rom = (BYTE*)malloc(mem->rom_size);

	if (mem->ram == NULL || mem->rom == NULL)
		return 1;

	return 0;
}
int x86ClearMemory(X86_MEMORY* mem) 
{
	// ram
	if (mem->ram != NULL)
		memset(mem->ram, 0, mem->ram_size);

	// rom
	if (mem->rom != NULL)
		memset(mem->rom, 0, mem->rom_size);

	return 0;
}
int x86FreeMemory(X86_MEMORY* mem) 
{
	// ram
	if (mem->ram != NULL) {
		free(mem->ram);
		mem->ram = NULL;
	}

	// rom
	if (mem->rom != NULL) {
		free(mem->rom);
		mem->rom = NULL;
	}

	return 0;
}

/*CPU*/
int x86ResetCPU(X86_CPU* cpu) 
{
	int i;
	for (i = 0; i < X86_GENERAL_REGISTER_COUNT; ++i) {
		cpu->registers[i].r32 = 0;
	}
	for (i = 0; i < X86_SEGMENT_REGISTER_COUNT; ++i) {
		cpu->segment_registers[i] = 0;
		cpu->segment_descriptors[i].base = 0;
		cpu->segment_descriptors[i].limit = 0;
		cpu->segment_descriptors[i].access = 0;
		cpu->segment_descriptors[i].flags = 0;
	}
	for (i = 0; i < X86_CONTROL_REGISTER_COUNT; ++i) {
		cpu->control_registers[i] = 0;
	}

	x86InitEflags(&cpu->eflags);

	cpu->mode = CPU_REAL_MODE;
	cpu->segment_descriptors[SEG_CS].base = 0xf000;
	cpu->segment_descriptors[SEG_CS].limit = 0xf000;
	cpu->segment_registers[SEG_CS] = 0;
	cpu->eip = 0xfff0;
	cpu->eip_ptr = NULL;

	// global descriptor table
	cpu->gdtr.base = 0;
	cpu->gdtr.limit = 0xFFFF;
	cpu->gdt = (uint64_t*)cpu->mem.ram;

	// interrupt descriptor table
	cpu->idtr.base = 0;
	cpu->idtr.limit = 0xFFFF;
	cpu->idt = (uint64_t*)cpu->mem.ram;

	// local descriptor table
	cpu->ldtr.base = 0;
	cpu->ldtr.limit = 0xFFFF;
	cpu->ldtr.selector = 0;

	cpu->hlt = 0;

	memset(cpu->output_str, 0, sizeof(cpu->output_str));
	memset(cpu->addressing_str, 0, sizeof(cpu->addressing_str));

	return 0;
}
int x86InitCPU(X86_CPU* cpu, uint32_t rom_base, uint32_t rom_end, uint32_t ram_base, uint32_t ram_end)
{
	if (x86InitMemory(&cpu->mem, rom_base, rom_end, ram_base, ram_end) != 0)
		return 1;

	x86ClearMemory(&cpu->mem);

	x86ResetCPU(cpu);

	return 0;
}
int x86FreeCPU(X86_CPU* cpu) 
{
	x86ResetCPU(cpu);
	x86FreeMemory(&cpu->mem);
	return 0;
}

/* Register read / write */
uint32_t x86CPUGetRegister(X86_CPU* cpu, uint32_t reg, uint32_t operand_size)
{
	uint32_t rm_v = 0;
	switch (operand_size) {
		case 1:
			if (reg <= 3)
				return cpu->registers[reg].r8.l;
			else
				return cpu->registers[reg - 4].r8.h;
		case 2:
			return cpu->registers[reg].r16;
		case 4:
			return cpu->registers[reg].r32;
	}
}
int32_t x86CPUGetRegisterSigned(X86_CPU* cpu, uint32_t reg, uint32_t operand_size)
{
	switch (operand_size) {
		case 1:
			if (reg <= 3)
				return (int8_t)cpu->registers[reg].r8.l;
			else
				return (int8_t)cpu->registers[reg- 4].r8.h;
		case 2:
			return (int16_t)cpu->registers[reg].r16;
		case 4:
			return (int32_t)cpu->registers[reg].r32;
	}
}
void x86CPUSetRegister(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t value)
{
	switch (operand_size) {
		case 1:
			if (reg <= 3)
				cpu->registers[reg].r8.l = value;
			else
				cpu->registers[reg - 4].r8.h = value;
			break;
		case 2:
			cpu->registers[reg].r16 = value;
			break;
		case 4:
			cpu->registers[reg].r32 = value;
			break;
	}
}

void set_memory_value(X86_CPU* cpu, uint32_t address, uint32_t operand_size, uint32_t value)
{
	if (address > cpu->mem.rom_base) {
		return;
	}
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

void x86CPULoadSegmentDescriptor(X86_CPU* cpu, uint16_t selector, X86_SEGMENT_DESCRIPTOR* descriptor)
{
	uint32_t index = (selector & ~7) >> 3;
	uint64_t value = cpu->gdt[index];

	descriptor->base = ((value >> 16) & 0xFFFFFF) + ((value >> 56) << 24);
	descriptor->limit = (value & 0xFFFF) + ((value >> 48) & 0xF0000);
	descriptor->access = (value >> 40) & 0xFF;
	descriptor->flags = (value >> 52) & 0xF;
	descriptor->default_size = (value >> 54) & 0b1;
}

int lldt(X86_CPU* cpu, uint32_t address, uint32_t counter) {
	// Load the LDT register
	uint16_t selector = *(uint16_t*)x86GetCPUMemoryPtr(cpu, address);

	// Load the source operand into the segment selector field of the local descriptor table register (LDTR)
	cpu->ldtr.selector = selector;

	// locate the segment descriptor for the LDT in the global descriptor table (GDT)
	X86_SEGMENT_DESCRIPTOR descriptor;
	x86CPULoadSegmentDescriptor(cpu, selector, &descriptor);

	//load the segment limit and base address for the LDT from the segment descriptor into the LDTR
	cpu->ldtr.limit = descriptor.limit;
	cpu->ldtr.base = descriptor.base;

	return 0;
}

/* opcodes */

int move_imm_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// mov reg, imm8
	uint32_t imm = x86CPUFetchMemory(cpu, operand_size, &counter);
	x86CPUSetRegister(cpu, reg, operand_size, imm);
	cpu->eip += counter;	
	return 0;
}
int move_ptr_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// mov reg, [address]
	uint32_t address = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint32_t value = x86CPUReadMemory(cpu, address, operand_size);
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;	
	return 0;
}
int inc_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// inc reg
	uint32_t v = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_INC, v, 1, operand_size, &v);
	x86CPUSetRegister(cpu, reg, operand_size, v);
	cpu->eip += counter;
	return 0;
}
int dec_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// dec reg
	uint32_t v = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_DEC, v, 1, operand_size, &v);
	x86CPUSetRegister(cpu, reg, operand_size, v);
	cpu->eip += counter;
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
	return 0;
}
int cld(X86_CPU* cpu, uint32_t counter)
{
	// clear direction flag
	cpu->eflags.DF = 0;
	cpu->eip += counter;
	return 0;
}
int std(X86_CPU* cpu, uint32_t counter)
{
	// set direction flag
	cpu->eflags.DF = 1;
	cpu->eip += counter;
	return 0;
}
int cli(X86_CPU* cpu, uint32_t counter)
{
	// clear interrupt flag
	cpu->eflags.IF = 0;
	cpu->eip += counter;
	return 0;
}
int sti(X86_CPU* cpu, uint32_t counter)
{
	// set interrupt flag
	cpu->eflags.IF = 1;
	cpu->eip += counter;
	return 0;
}
int hlt(X86_CPU* cpu, uint32_t counter)
{
	// hlt
	cpu->hlt = 1;
	cpu->eip += counter;	
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
	return 0;
}
int jmp_far(X86_CPU* cpu, uint32_t counter)
{
	uint32_t address = x86CPUFetchDword(cpu, &counter);
	uint16_t selector = x86CPUFetchWord(cpu, &counter);

	cpu->eip = address;

	/* update CS register and reload segment descriptor */
	x86CPULoadSegmentDescriptor(cpu, selector, &cpu->segment_descriptors[SEG_CS]);

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
	
	// last 4 bits of the opcode are the conditional test.
	switch (opcode & 0x0f) {
		case CONDITIONAL_TEST_OVERFLOW:
			if (eflags.OF == 1)
				counter += offset;			
			break;
		case CONDITIONAL_TEST_NO_OVERFLOW:
			if (eflags.OF == 0)
				counter += offset;
			break;
		case CONDITIONAL_TEST_CARRY:
			if (eflags.CF == 1)
				counter += offset;
			break;
		case CONDITIONAL_TEST_NOT_CARRY:
			if (eflags.CF == 0)
				counter += offset;
			break;
		case CONDITIONAL_TEST_EQUAL_ZERO:
			if (eflags.ZF == 1)
				counter += offset;
			break;
		case CONDITIONAL_TEST_NOT_EQUAL_ZERO:
			if (eflags.ZF == 0)
				counter += offset;
			break;
		case CONDITIONAL_TEST_BELOW_OR_EQUAL:
			if (eflags.CF == 1 || eflags.ZF == 1)
				counter += offset;
			break;
		case CONDITIONAL_TEST_ABOVE:
			if (eflags.CF == 0 && eflags.ZF == 0)
				counter += offset;
			break;
		case CONDITIONAL_TEST_SIGN:
			if (eflags.SF == 1)
				counter += offset;
			break;
		case CONDITIONAL_TEST_NOT_SIGN:
			if (eflags.SF == 0)
				counter += offset;
			break;
		case CONDITIONAL_TEST_PARITY:
			if (eflags.PF == 1)
				counter += offset;
			break;
		case CONDITIONAL_TEST_NOT_PARITY:
			if (eflags.PF == 0)
				counter += offset;
			break;
		case CONDITIONAL_TEST_LESS:
			if (eflags.SF != eflags.OF)
				counter += offset;
			break;
		case CONDITIONAL_TEST_NOT_LESS:
			if (eflags.SF == eflags.OF)
				counter += offset;
			break;
		case CONDITIONAL_TEST_LESS_OR_EQUAL:
			if (eflags.ZF == 1 || eflags.SF != eflags.OF)
				counter += offset;
			break;
		case CONDITIONAL_TEST_NOT_LESS_OR_EQUAL:
			if (eflags.ZF == 0 && eflags.SF == eflags.OF)
				counter += offset;
			break;
	}

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
	return 0;
}
int in_byte_reg(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// input byte/word/dword from I/O port in DX into AL/AX/EAX.
	WORD address = (WORD)x86CPUGetRegister(cpu, REG_DX, 2);
	uint32_t value = x86CPUGetIOByte(cpu, address);
	x86CPUSetRegister(cpu, REG_EAX, operand_size, value);
	cpu->eip += counter;
	return 0;
}
int out_byte_reg(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// output byte/word/dword in AL/AX/EAX to I/O port address in DX.
	WORD address = (WORD)x86CPUGetRegister(cpu, REG_DX, 2);
	uint32_t value = x86CPUGetRegister(cpu, REG_EAX, operand_size);
	x86CPUSetIOByte(cpu, operand_size, address, value);
	cpu->eip += counter;	
	return 0;
}
int in_byte_imm(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// input byte/word/dword from imm8 I/O port address into AL/AX/EAX
	BYTE imm = x86CPUFetchByte(cpu, &counter);
	uint32_t value = x86CPUGetIOByte(cpu, imm);
	x86CPUSetRegister(cpu, REG_EAX, operand_size, value);
	cpu->eip += counter;	
	return 0;
}
int out_byte_imm(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// output byte/word/dword in AL/AX/EAX to I/O port address imm8.
	BYTE imm = x86CPUFetchByte(cpu, &counter);
	uint32_t value = x86CPUGetRegister(cpu, REG_EAX, operand_size);
	x86CPUSetIOByte(cpu, operand_size, imm, value);
	cpu->eip += counter;
	return 0;
}
int movzx(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t src_size, uint32_t dest_size, uint32_t counter)
{
	// B6 MOVZX r16, r/m8
	// B6 MOVZX r32, r/m8
	// B7 MOVZX r32, r/m16

	uint32_t value = x86CPUGetRegister(cpu, mode->rm, src_size);
	x86CPUSetRegister(cpu, mode->reg, dest_size, value);
	cpu->eip += counter;
	return 0;
}
int movsx(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t src_size, uint32_t dest_size, uint32_t counter)
{
	// BE MOVSX r16, r/m8
	// BE MOVSX r32, r/m8
	// BF MOVSX r32, r/m16

	int32_t value = x86CPUGetRegisterSigned(cpu, mode->rm, src_size);
	x86CPUSetRegister(cpu, mode->reg, dest_size, value);
	cpu->eip += counter;
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
	return 0;
}
int sub_imm_reg(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t counter)
{
	// sub reg imm
	uint32_t imm = x86CPUFetchMemory(cpu, operand_size, &counter);
	uint32_t value = x86CPUGetRegister(cpu, reg, operand_size);
	x86Alu(&cpu->eflags, INSTRUCTION_TYPE_SUB, value, imm, operand_size, &value);
	x86CPUSetRegister(cpu, reg, operand_size, value);
	cpu->eip += counter;
	return 0;
}
int lgdt(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t address_size, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter) {
	// Load the GDT register

	ADDRESSING_MODE_FIELD_STRUCT am = { 0 };//uint32_t address;
	get_addressing_mode(cpu, mode, address_size, operand_size, &am, &counter);

	uint8_t* descriptor = (uint8_t*)x86GetCPUMemoryPtr(cpu, am.address);

	// load the values in the source operand into the global descriptor table register (GDTR) 

	cpu->gdtr.limit = *(uint16_t*)descriptor;
	cpu->gdtr.base = *(uint32_t*)(descriptor + 2);
	if (operand_size == 2)
		cpu->gdtr.base &= 0x00FFFFFF;
	cpu->gdt = (uint64_t*)x86GetCPUMemoryPtr(cpu, cpu->gdtr.base);

	cpu->eip += counter;
	return 0;
}
int lidt(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t address_size, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter) {
	// Load the IDT register

	ADDRESSING_MODE_FIELD_STRUCT am = { 0 };//uint32_t address;
	get_addressing_mode(cpu, mode, address_size, operand_size, &am, &counter);

	uint8_t* descriptor = (uint8_t*)x86GetCPUMemoryPtr(cpu, am.address);

	// load the values in the source operand into the interrupt descriptor table register (IDTR).

	cpu->idtr.limit = *(uint16_t*)descriptor;
	cpu->idtr.base = *(uint32_t*)(descriptor + 2);
	if (operand_size == 2)
		cpu->idtr.base &= 0x00FFFFFF;
	cpu->idt = (uint64_t*)x86GetCPUMemoryPtr(cpu, cpu->idtr.base);
	cpu->eip += counter;

	return 0;
}
int lldt_imm(X86_CPU* cpu, uint32_t counter) {
	// Load the LDT register
	uint32_t address = x86CPUFetchMemory(cpu, 2, &counter);
	lldt(cpu, address, counter);	
	cpu->eip += counter;
	return 0;
}
int lldt_reg(X86_CPU* cpu, uint32_t reg, uint32_t counter) {
	// Load the LDT register
	uint32_t address = x86CPUGetRegister(cpu, reg, 2);
	lldt(cpu, address, counter);	
	cpu->eip += counter;
	return 0;
}
int mov_seg_r32(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t counter)
{
	// MOV seg, r32

	uint8_t sreg = mode->reg;
	uint8_t reg = mode->rm;
	uint32_t selector = x86CPUGetRegister(cpu, reg, 4);

	cpu->segment_registers[sreg] = selector;
	x86CPULoadSegmentDescriptor(cpu, selector, &cpu->segment_descriptors[sreg]);
	cpu->eip += counter;
	return 0;
}
int mov_cr_r32(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t counter)
{
	// MOV cr0-7, r32

	if (mode->rm == 1 || mode->rm == 5 ||
		mode->rm == 6 || mode->rm == 7) {
		return X86_CPU_ERROR_UD;
	}

	uint32_t value = x86CPUGetRegister(cpu, mode->reg, 4);
	cpu->control_registers[mode->rm] = value;
	cpu->eip += counter;
	return 0;
}
int mov_r32_cr(X86_CPU* cpu, X86_MOD_RM_BITS* mode, uint32_t counter)
{
	// MOV r32, cr0-7

	if (mode->rm == 1 || mode->rm == 5 ||
		mode->rm == 6 || mode->rm == 7) {
		return X86_CPU_ERROR_UD;
	}

	uint32_t value = cpu->control_registers[mode->reg];
	x86CPUSetRegister(cpu, mode->rm, 4, value);
	cpu->eip += counter;
	return 0;
}
int wrmsr(X86_CPU* cpu, uint32_t counter)
{
	//Write to Model Specific Register
	cpu->eip += counter;
	return 0;
}
int invd(X86_CPU* cpu, uint32_t counter)
{
	// Invalidate Cache
	cpu->eip += counter;
	return 0;
}
int wbinvd(X86_CPU* cpu, uint32_t counter)
{
	// Write Back and Invalidate Cache
	cpu->eip += counter;
	return 0;
}
int nop(X86_CPU* cpu, uint32_t counter)
{
	cpu->eip += counter;
	return 0;
}
int movs(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// Move Data From String to String - MOVS r8/r16/r32
	// move byte/word/dword from address ds:si/esi to es:di/edi.
	uint32_t esi = x86CPUGetRegister(cpu, REG_ESI, operand_size);
	uint32_t edi = x86CPUGetRegister(cpu, REG_EDI, operand_size);
	uint32_t value = x86CPUReadMemory(cpu, esi, operand_size);

	set_memory_value(cpu, edi, operand_size, value);
	if (cpu->eflags.DF == 0) {
		x86CPUSetRegister(cpu, REG_ESI, operand_size, esi + operand_size);
		x86CPUSetRegister(cpu, REG_EDI, operand_size, edi + operand_size);
	}
	else {
		x86CPUSetRegister(cpu, REG_ESI, operand_size, esi - operand_size);
		x86CPUSetRegister(cpu, REG_EDI, operand_size, edi - operand_size);
	}
	cpu->eip += counter;
	return 0;
}
int stos(X86_CPU* cpu, uint32_t address_size, uint32_t operand_size, uint32_t counter)
{
	// Store String - STOS r8/m16/m32
	// store al/ax/eax at address es:di/edi 
	uint32_t eax = x86CPUGetRegister(cpu, REG_EAX, operand_size);
	uint32_t edi = x86CPUGetRegister(cpu, REG_EDI, address_size);

	set_memory_value(cpu, edi, operand_size, eax);
	if (cpu->eflags.DF == 0) {
		x86CPUSetRegister(cpu, REG_EDI, operand_size, edi + operand_size);
	}
	else {
		x86CPUSetRegister(cpu, REG_EDI, operand_size, edi - operand_size);
	}

	cpu->eip += counter;
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

		if (cpu->eflags.DF == 0) {
			esi += operand_size;
			edi += operand_size;
		}
		else {
			esi -= operand_size;
			edi -= operand_size;
		}
	}

	cpu->registers[REG_ECX].r32 = ecx;
	cpu->registers[REG_ESI].r32 = esi;
	cpu->registers[REG_EDI].r32 = edi;
	cpu->eip += counter;
	return 0;
}
int loop(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// Loop According to ECX Counter -  LOOP rel8
	signed char rel8 = (signed char)x86CPUFetchByte(cpu, &counter);
	uint32_t ecx = x86CPUGetRegister(cpu, REG_ECX, operand_size);
	ecx -= 1;
	x86CPUSetRegister(cpu, REG_ECX, operand_size, ecx);
	cpu->eip += counter;
	if (ecx != 0)
		cpu->eip += rel8;
	return 0;
}
int loope(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// Loop According to ECX Counter -  LOOPE rel8
	signed char rel8 = (signed char)x86CPUFetchByte(cpu, &counter);
	uint32_t ecx = x86CPUGetRegister(cpu, REG_ECX, operand_size);
	ecx -= 1;
	x86CPUSetRegister(cpu, REG_ECX, operand_size, ecx);
	cpu->eip += counter;
	if (ecx != 0 && cpu->eflags.ZF == 1)
		cpu->eip += rel8;
	return 0;
}
int loopne(X86_CPU* cpu, uint32_t operand_size, uint32_t counter)
{
	// Loop According to ECX Counter -  LOOPNE rel8
	signed char rel8 = (signed char)x86CPUFetchByte(cpu, &counter);
	uint32_t ecx = x86CPUGetRegister(cpu, REG_ECX, operand_size);
	ecx -= 1;
	x86CPUSetRegister(cpu, REG_ECX, operand_size, ecx);
	cpu->eip += counter;
	if (ecx != 0 && cpu->eflags.ZF == 0)
		cpu->eip += rel8;
	return 0;
}
int push_reg(X86_CPU* cpu, BYTE reg, uint32_t operand_size, uint32_t counter)
{
	uint32_t esp = x86CPUGetRegister(cpu, REG_ESP, operand_size);
	uint32_t value = x86CPUGetRegister(cpu, reg, operand_size);
	x86CPUSetRegister(cpu, REG_ESP, operand_size, esp - operand_size);
	set_memory_value(cpu, esp - operand_size, operand_size, value);
	cpu->eip += counter;	
	return 0;
}
int pop_reg(X86_CPU* cpu, BYTE reg, uint32_t operand_size, uint32_t counter)
{
	uint32_t esp = x86CPUGetRegister(cpu, REG_ESP, operand_size);
	uint32_t value = x86CPUReadMemory(cpu, esp, operand_size);
	x86CPUSetRegister(cpu, reg, operand_size, value);
	x86CPUSetRegister(cpu, REG_ESP, operand_size, esp + operand_size);
	cpu->eip += counter;	
	return 0;
}
int xchg_reg(X86_CPU* cpu, BYTE reg1, BYTE reg2, uint32_t operand_size, uint32_t counter)
{
	// xchg reg, reg
	uint32_t reg1_v = x86CPUGetRegister(cpu, reg1, operand_size);
	uint32_t reg2_v = x86CPUGetRegister(cpu, reg2, operand_size);
	x86CPUSetRegister(cpu, reg1, operand_size, reg2_v);
	x86CPUSetRegister(cpu, reg2, operand_size, reg1_v);
	cpu->eip += counter;
	return 0;
}

/*DECODE*/
int decode_opcode_f3(X86_CPU* cpu, BYTE opcode, uint32_t operand_size, uint32_t counter)
{
	switch (opcode) {
		case 0xA5: // MOV WORD or DWORD
			rep_movs(cpu, operand_size, counter);
			return X86_CPU_ERROR_SUCCESS;

		case 0xA4: // MOV BYTE
			rep_movs(cpu, 1, counter);
			return X86_CPU_ERROR_SUCCESS;
	}

	error_out(cpu, counter);
	return X86_CPU_ERROR_UD;
}
int decode_opcode_0f(X86_CPU* cpu, BYTE opcode, uint32_t address_size, uint32_t operand_size, BYTE segment_override_byte, uint32_t counter)
{
	X86_MOD_RM mode;

	switch (opcode) { // 0F xx
		case 0x00: // LLDT
			mode.byte = cpu->eip_ptr[counter++];
			if (mode.bits.reg == 0b010) { // 0f 00 /2 = LLDT r/m
				if (mode.bits.mod == 0b11) {
					return lldt_reg(cpu, mode.bits.rm, counter);
				}
				else {
					return lldt_imm(cpu, counter);
				}
			} break;

		case 0x01: // LGDT / LIDT
			mode.byte = cpu->eip_ptr[counter++];
			if (mode.bits.reg == 0b010) { // 0f 01 /2 = LGDT m
				return lgdt(cpu, &mode.bits, address_size, operand_size, segment_override_byte, counter);
			}
			else if (mode.bits.reg == 0b011) { // 0f 01 /3 = LIDT m
				return lidt(cpu, &mode.bits, address_size, operand_size, segment_override_byte, counter);
			} break;

		case 0x08: // invd
			return invd(cpu, counter);

		case 0x09: // wbinvd
			return wbinvd(cpu, counter);

		case 0x20: //mov r32, cr0-7
			mode.byte = cpu->eip_ptr[counter++];
			return mov_r32_cr(cpu, &mode.bits, counter);

		case 0x22: //mov cr0-7, r32
			mode.byte = cpu->eip_ptr[counter++];
			return mov_cr_r32(cpu, &mode.bits, counter);

		case 0x30: // WRMSR
			return wrmsr(cpu, counter);

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
			return jcc(cpu, opcode, operand_size, counter);
			
		case 0xB6: // MOVZX r16/r32, r/m8
			mode.byte = cpu->eip_ptr[counter++];
			return movzx(cpu, &mode.bits, 1, operand_size, counter);

		case 0xB7: // MOVZX r32, r/m16
			mode.byte = cpu->eip_ptr[counter++];
			return movzx(cpu, &mode.bits, 2, 4, counter);

		case 0xBE: // MOVSX r16/r32, r/m8
			mode.byte = cpu->eip_ptr[counter++];
			return movsx(cpu, &mode.bits, 1, operand_size, counter);

		case 0xBF: // MOVSX r32, r/m16
			mode.byte = cpu->eip_ptr[counter++];
			return movsx(cpu, &mode.bits, 2, 4, counter);
	}

	error_out(cpu, counter);
	return X86_CPU_ERROR_UD;
}
int decode_opcode_one_byte(X86_CPU* cpu, BYTE opcode, uint32_t operand_size, uint32_t counter)
{	
	// check for one byte opcodes
	switch (opcode) {
		case 0x04:
			return add_imm_reg(cpu, REG_AL, 1, counter);
		case 0x05:
			return add_imm_reg(cpu, REG_EAX, operand_size, counter);

		case 0x0C:
			return or_imm_reg(cpu, REG_AL, 1, counter);
		case 0x0D:
			return or_imm_reg(cpu, REG_EAX, operand_size, counter);

		case 0x24:
			return and_imm_reg(cpu, REG_AL, 1, counter);			
		case 0x25:
			return and_imm_reg(cpu, REG_EAX, operand_size, counter);

		case 0x2C:
			return sub_imm_reg(cpu, REG_AL, 1, counter);
		case 0x2D:
			return sub_imm_reg(cpu, REG_EAX, operand_size, counter);

		case 0x3C:
			return cmp_imm_reg(cpu, REG_AL, 1, counter);			
		case 0x3D:
			return cmp_imm_reg(cpu, REG_EAX, operand_size, counter);

		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47: // INC 16/32bit
			return inc_reg(cpu, (opcode & 0b111), operand_size, counter);
			
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F: // DEC 16/32bit
			return dec_reg(cpu, (opcode & 0b111), operand_size, counter);
		
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57: // PUSH 16/32bit
			return push_reg(cpu, (opcode & 0b111), operand_size, counter);

		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F: // POP 16/32bit
			return pop_reg(cpu, (opcode & 0b111), operand_size, counter);

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
			return jcc(cpu, opcode, 1, counter);

		case 0x8E: {			
			X86_MOD_RM b;
			b.byte = cpu->eip_ptr[counter++];
			return mov_seg_r32(cpu, &b.bits, counter);
		}

		case 0x90:
			return nop(cpu, counter);
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
			return xchg_reg(cpu, REG_EAX, (opcode & 0b111), operand_size, counter);

		case 0xA0:
			return move_ptr_reg(cpu, REG_AL, 1, counter);
		case 0xA1:
			return move_ptr_reg(cpu, REG_EAX, operand_size, counter);
			
		case 0xA4:
			return movs(cpu, 1, counter);		
		case 0xA5:
			return movs(cpu, operand_size, counter);
			
		case 0xAA:
			return stos(cpu, 4, 1, counter);
		case 0xAB:
			return stos(cpu, 4, operand_size, counter);
		
		case 0xB0:
		case 0xB1:
		case 0xB2:
		case 0xB3:
		case 0xB4:
		case 0xB5:
		case 0xB6:
		case 0xB7: // MOV 8bit		
			return move_imm_reg(cpu, (opcode & 0b111), 1, counter);

		case 0xB8:
		case 0xB9:
		case 0xBA:
		case 0xBB:
		case 0xBC:
		case 0xBD:
		case 0xBE:
		case 0xBF: // MOV 16/32bit
			return move_imm_reg(cpu, (opcode & 0b111), operand_size, counter);

		case 0xE0:
			return loopne(cpu, operand_size, counter);
		case 0xE1:
			return loope(cpu, operand_size, counter);
		case 0xE2:
			return loop(cpu, operand_size, counter);

		case 0xE4:
			return in_byte_imm(cpu, 1, counter);			
		case 0xE5:
			return in_byte_imm(cpu, operand_size, counter);
		case 0xE6:
			return out_byte_imm(cpu, 1, counter);			
		case 0xE7:
			return out_byte_imm(cpu, operand_size, counter);

		case 0xE9:
			return jmp_imm_rel(cpu, operand_size, counter);
		case 0xEA:
			return jmp_far(cpu, counter);
		case 0xEB:
			return jmp_imm_rel(cpu, 1, counter);
		case 0xEC:
			return in_byte_reg(cpu, 1, counter);			
		case 0xED:
			return in_byte_reg(cpu, operand_size, counter);
		case 0xEE:
			return out_byte_reg(cpu, 1, counter);
		case 0xEF:
			return out_byte_reg(cpu, operand_size, counter);

		case 0xF4:
			return hlt(cpu, counter);

		case 0xFA:
			return cli(cpu, counter);
		case 0xFB:
			return sti(cpu, counter);
		case 0xFC:
			return cld(cpu, counter);			
		case 0xFD:
			return std(cpu, counter);
			
		case 0xFE: {
			X86_MOD_RM b;
			b.byte = cpu->eip_ptr[counter++];
			if (b.bits.mod == 0b11) {
				switch (b.bits.reg) {
					case 0b000: // INC
						inc_reg(cpu, b.bits.rm, 1, counter);
						break;
					case 0b001: // DEC
						dec_reg(cpu, b.bits.rm, 1, counter);
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
int decode_opcode_extended(X86_CPU* cpu, BYTE extended_opcode, ADDRESSING_MODE_STRUCT* addressing_mode, uint32_t operand_size, uint32_t* instr_result, uint32_t counter)
{
	// 0x00 - 0x07 (0b000 - 0b111) ( 8 )
	switch (extended_opcode) {
		case 0b000: // ADD
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_ADD, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b001: // OR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_OR, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b010: // ADC
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_ADC, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b011: // SBB
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_SBB, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b100: // AND
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_AND, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b101: // SUB
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_SUB, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b110: // XOR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_XOR, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b111: // CMP
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_CMP, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			cpu->eip += counter;
			return X86_CPU_ERROR_DECODED; // cmp only sets flags.

		default:
			error_out(cpu, counter);
			return X86_CPU_ERROR_UD;
	}
}
int decode_opcode(X86_CPU* cpu, BYTE opcode, ADDRESSING_MODE_STRUCT* addressing_mode, uint32_t operand_size, uint32_t* instr_result, uint32_t counter)
{
	// 0x00 - 0x3F (0b000000 - 0b111111) ( 64 )
	
	switch (opcode) {
		case 0b000000: // ADD
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_ADD, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b000010: // OR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_OR, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b001000: // AND
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_AND, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b001010: // SUB
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_SUB, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b001100: // XOR
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_XOR, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			return X86_CPU_ERROR_SUCCESS;

		case 0b001110: // CMP
			x86Alu(&cpu->eflags, INSTRUCTION_TYPE_CMP, addressing_mode->dest.value, addressing_mode->src.value, operand_size, instr_result);
			cpu->eip += counter;
			return X86_CPU_ERROR_DECODED; // cmp only sets flags.

		case 0b100001: // XCHG
			// set register or memory.
			switch (addressing_mode->src.type) {
				case INSTRUCTION_RM_INDIRECT:
					set_memory_value(cpu, addressing_mode->src.address, operand_size, addressing_mode->dest.value);
					break;
				case INSTRUCTION_RM_REGISTER:
					x86CPUSetRegister(cpu, addressing_mode->src.reg, operand_size, addressing_mode->dest.value);
					break;
			}
			*instr_result = addressing_mode->src.value;
			return X86_CPU_ERROR_SUCCESS;

		case 0b100010: // MOV
			*instr_result = addressing_mode->src.value;
			return X86_CPU_ERROR_SUCCESS;

		case 0b111111: // JMP
			cpu->eip = addressing_mode->src.value;
			return X86_CPU_ERROR_DECODED;

		default:
			error_out(cpu, counter);
			return X86_CPU_ERROR_UD;
	}
}

void x86CPUGetDefaultSize(X86_CPU* cpu, PREFIX_BYTE_STRUCT* prefix, uint8_t* operand_size, uint8_t* address_size)
{
	if (cpu->mode == CPU_PROTECTED_MODE && cpu->segment_descriptors[SEG_CS].default_size == 1) {
		if (prefix->byte_66)
			*operand_size = 2;
		else
			*operand_size = 4;

		if (prefix->byte_67)
			*address_size = 2;
		else
			*address_size = 4;
	}
	else {
		if (prefix->byte_66)
			*operand_size = 4;
		else
			*operand_size = 2;

		if (prefix->byte_67)
			*address_size = 4;
		else
			*address_size = 2;
	}
}

int x86CPUFetchPrefixBytes(X86_CPU* cpu, PREFIX_BYTE_STRUCT* prefix, X86_OPCODE* opcode, uint32_t* counter)
{
	BYTE prefix_bytes[X86_CPU_MAX_INSTRUCTION_SIZE] = { 0 };

	// fetch prefix bytes and first opcode byte.
	for (int i = 0; i < X86_CPU_MAX_INSTRUCTION_SIZE + 1; ++i) {
		if (i == X86_CPU_MAX_INSTRUCTION_SIZE) {
			return X86_CPU_ERROR_UD;
		}

		prefix_bytes[i] = cpu->eip_ptr[*counter];
		*counter += 1;

		switch (prefix_bytes[i]) {

			case 0x66:
				prefix->byte_66 = 1;
				continue;
			case 0x67:
				prefix->byte_67 = 1;
				continue;

			case 0xF3:
				prefix->byte_f3 = 1;
				continue;
			case 0x0F:
				prefix->byte_0f = 1;
				continue;

			case 0x26: // es
			case 0x2E: // cs
			case 0x36: // ss
			case 0x3E: // ds
			case 0x64: // fs
			case 0x65: // gs
				prefix->segment_override = prefix_bytes[i];
				continue;
		}

		opcode->byte = prefix_bytes[i];
		prefix_bytes[i] = 0x00;
		break;
	}

	return 0;
}

/*EXECUTE*/
int x86CPUExecute(X86_CPU* cpu)
{
	int result = 0;
	uint32_t counter = 0;

	cpu->eip_ptr = x86GetCPUMemoryPtr(cpu, cpu->eip);

	X86_OPCODE opcode = { 0 };
	PREFIX_BYTE_STRUCT prefix = { 0 };
	x86CPUFetchPrefixBytes(cpu, &prefix, &opcode, &counter);

	uint8_t operand_size = 0;
	uint8_t addressing_size = 0;
	x86CPUGetDefaultSize(cpu, &prefix, &operand_size, &addressing_size);

	if (prefix.byte_0f) {
		return decode_opcode_0f(cpu, opcode.byte, addressing_size, operand_size, prefix.segment_override, counter);
	}
	else if (prefix.byte_f3) {
		return decode_opcode_f3(cpu, opcode.byte, operand_size, counter);
	} 
	else {
		result = decode_opcode_one_byte(cpu, opcode.byte, operand_size, counter);
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
	
	ADDRESSING_MODE_STRUCT addressing_mode = { 0 };
	ADDRESSING_MODE_FIELD_STRUCT rm = { 0 };
	uint32_t reg_v = 0;
	INSTRUCTION_RM reg_type = 0;
	BYTE direction = 0;	

	if (opcode.bits.op == 0b100000) {
		//immediate instruction
		reg_type = INSTRUCTION_RM_IMM;
		direction = 0; // direction always const -> reg (cant be reg -> const)

		uint32_t const_size;
		if (opcode.bits.direction == 1)	// constant is a signed 1 byte operand
			const_size = 1;
		else // constant is the same size specified by s.
			const_size = operand_size;

		reg_v = x86CPUFetchMemory(cpu, const_size, &counter);		
	}
	else {
		// r/m or SIB
		reg_type = INSTRUCTION_RM_REGISTER;
		direction = opcode.bits.direction;

		reg_v = x86CPUGetRegister(cpu, mode.bits.reg, operand_size);
	}
	
	// figure out the addressing mode.
	get_addressing_mode(cpu, &mode.bits, addressing_size, operand_size, &rm, &counter);
	
	// figure out the addressing direction.
	if (direction == 0) {
		// reg -> r/m

		addressing_mode.src.value = reg_v;
		addressing_mode.src.reg = mode.bits.reg;
		addressing_mode.src.type = reg_type;

		addressing_mode.dest.value = rm.value;
		addressing_mode.dest.reg = mode.bits.rm;
		addressing_mode.dest.type = rm.type;
		addressing_mode.dest.address = rm.address;
	}
	else {
		// r/m -> reg

		addressing_mode.src.value = rm.value;
		addressing_mode.src.reg = mode.bits.rm;
		addressing_mode.src.type = rm.type;
		addressing_mode.src.address = rm.address;

		addressing_mode.dest.value = reg_v;
		addressing_mode.dest.reg = mode.bits.reg;
		addressing_mode.dest.type = reg_type;
	}

	// perform instruction.

	uint32_t instr_result = 0;

	if (opcode.bits.op == 0b100000) {
		// 0x80 - 0x83 ( 1 0 0 0 0 0 X X )
		result = decode_opcode_extended(cpu, mode.bits.reg, &addressing_mode, operand_size, &instr_result, counter);
		if (result != 0) {
			if (result == X86_CPU_ERROR_DECODED)
				return X86_CPU_ERROR_SUCCESS;
			else
				return result;
		}
	}
	else {
		// 0x00 - 0x3F ( X X X X X X )
		result = decode_opcode(cpu, opcode.bits.op, &addressing_mode, operand_size, &instr_result, counter);
		if (result != 0) {
			if (result == X86_CPU_ERROR_DECODED)
				return X86_CPU_ERROR_SUCCESS;
			else
				return result;
		}
	}

	// set register or memory.
	switch (addressing_mode.dest.type) {
		case INSTRUCTION_RM_INDIRECT:
			set_memory_value(cpu, rm.address, operand_size, instr_result);
			break;
		case INSTRUCTION_RM_REGISTER:		
			x86CPUSetRegister(cpu, addressing_mode.dest.reg, operand_size, instr_result);
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
