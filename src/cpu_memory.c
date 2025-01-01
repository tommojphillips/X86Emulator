// cpu_memory.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>
#include <stdbool.h>

#include "cpu.h"
#include "cpu_memory.h"

#include "type_defs.h"
#include "mem_tracking.h"

uint32_t x86GetEffectiveAddress(X86_CPU* cpu, uint32_t address)
{
	if (cpu->mode == CPU_REAL_MODE) {
		// In real mode, use segmented addressing
		uint32_t base_address = cpu->segment_descriptors[SEG_CS].base << 4;
		return cpu->mem.rom_end - 0xFFFFF | base_address | (address & 0xFFFF);
	}
	else {
		// In protected mode, use the base address from the segment descriptor
		uint32_t base_address = cpu->segment_descriptors[SEG_CS].base;
		return base_address | address;
	}
}

/* READ MEMORY */
void* x86GetCPUMemoryPtr(X86_CPU* cpu, uint32_t address)
{
	address = x86GetEffectiveAddress(cpu, address);
	if (address >= cpu->mem.ram_base && address <= cpu->mem.ram_end) {
		return (cpu->mem.ram + address - cpu->mem.ram_base);
	}
	if (address >= cpu->mem.rom_base && address <= cpu->mem.rom_end) {
		return cpu->mem.rom + address - cpu->mem.rom_base;
	}
	return NULL;
}
uint32_t x86CPUReadMemory(X86_CPU* cpu, uint32_t address, uint32_t operand_size)
{
	switch (operand_size) {
		case 1:
			return x86CPUReadByte(cpu, address);
		case 2:
			return x86CPUReadWord(cpu, address);
		case 4:
			return x86CPUReadDword(cpu, address);
	}
	return 0;
}
BYTE x86CPUReadByte(X86_CPU* cpu, uint32_t address)
{
	BYTE* ptr = (BYTE*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		return *ptr;
	return 0;
}
WORD x86CPUReadWord(X86_CPU* cpu, uint32_t address)
{
	WORD* ptr = (WORD*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		return *ptr;
	return 0;
}
DWORD x86CPUReadDword(X86_CPU* cpu, uint32_t address)
{
	DWORD* ptr = (DWORD*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		return *ptr;
	return 0;
}

/* WRITE MEMORY */
void x86CPUWriteByte(X86_CPU* cpu, uint32_t address, BYTE value)
{
	BYTE* ptr = (BYTE*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		*ptr = value;
}
void x86CPUWriteWord(X86_CPU* cpu, uint32_t address, WORD value)
{
	WORD* ptr = (WORD*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		*ptr = value;
}
void x86CPUWriteDword(X86_CPU* cpu, uint32_t address, DWORD value)
{
	DWORD* ptr = (DWORD*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		*ptr = value;
}

/* FETCH MEMORY AT EIP */
uint32_t x86CPUFetchMemory(X86_CPU* cpu, uint32_t operand_size, uint32_t* counter)
{
	switch (operand_size) {
		case 1:
			return x86CPUFetchByte(cpu, counter);
		case 2:
			return x86CPUFetchWord(cpu, counter);
		case 4:
			return x86CPUFetchDword(cpu, counter);
	}
	return 0;
}
BYTE x86CPUFetchByte(X86_CPU* cpu, uint32_t* counter)
{
	BYTE* ptr = (BYTE*)x86GetCPUMemoryPtr(cpu, cpu->eip + *counter);
	*counter += 1;
	if (ptr != NULL)
		return *ptr;
	return 0;
}
WORD x86CPUFetchWord(X86_CPU* cpu, uint32_t* counter)
{
	WORD* ptr = (WORD*)x86GetCPUMemoryPtr(cpu, cpu->eip + *counter);
	*counter += 2;
	if (ptr != NULL)
		return *ptr;
	return 0;
}
DWORD x86CPUFetchDword(X86_CPU* cpu, uint32_t* counter)
{
	DWORD* ptr = (DWORD*)x86GetCPUMemoryPtr(cpu, cpu->eip + *counter);
	*counter += 4;
	if (ptr != NULL)
		return *ptr;
	return 0;
}

/* IO read / write */
BYTE x86CPUGetIOByte(X86_CPU* cpu, uint32_t address) {
	switch (address) {
		case 0xC000:
			return 0x10;
		case 0x0cfc:
			return 0x10;
	}
	return 0;
}
void x86CPUSetIOByte(X86_CPU* cpu, uint32_t operand_size, uint32_t address, uint32_t value) {

}
