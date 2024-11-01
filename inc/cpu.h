// cpu.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef X86_CPU_H
#define X86_CPU_H

#include <stdint.h>
#include <string.h>

#include "cpu_eflags.h"
#include "cpu_alu.h"
#include "cpu_memory.h"

#include "type_defs.h"
#include "mem_tracking.h"

#define MNEMONIC_STR(_x_) sprintf _x_
#define MNEMONIC_REG(_x_) x86CPUGetRegisterMnemonic _x_
#define MNEMONIC_SEG(_x_) x86CPUGetSegmentRegisterMnemonic _x_
#define MNEMONIC_CON(_x_) x86CPUGetControlRegisterMnemonic _x_

#define MNEMONIC_SEG_OVERIDE(_x_) \
switch (_x_) { \
	case 0x26: \
		MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "es:")); \
		break; \
	case 0x2E: \
		MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "cs:")); \
		break; \
	case 0x36: \
		MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "ss:")); \
		break; \
	case 0x3E: \
		MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "ds:")); \
		break; \
	case 0x64: \
		MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "fs:")); \
		break; \
	case 0x65: \
		MNEMONIC_STR((cpu->output_str + strlen(cpu->output_str), "gs:")); \
		break; \
}

typedef enum _X86_CPU_ERROR {
	X86_CPU_ERROR_SUCCESS = 0,
	X86_CPU_ERROR_FATAL,
	X86_CPU_ERROR_HLT,
	X86_CPU_ERROR_UD,
	X86_CPU_ERROR_DECODED,
} X86_CPU_ERROR;

// register

enum X86_GENERAL_REGISTER_TYPE {
	REG_EAX = 0,
	REG_AX = 0,
	REG_AL = 0,
	REG_AH = 4,

	REG_ECX = 1,
	REG_CX = 1,
	REG_CL = 1,
	REG_CH = 5,
	
	REG_EDX = 2,
	REG_DX = 2,
	REG_DL = 2,
	REG_DH = 6,

	REG_EBX = 3,
	REG_BX = 3,
	REG_BL = 3,
	REG_BH = 7,

	REG_ESP = 4,
	REG_SP = 4,

	REG_EBP = 5,
	REG_BP = 5,
	
	REG_ESI = 6,
	REG_SI = 6,
	
	REG_EDI = 7,
	REG_DI = 7,
};

#define X86_GENERAL_REGISTER_COUNT 8

enum X86_SEGMENT_REGISTER_TYPE {
	SEG_CS = 0, // code segment
	SEG_DS,
	SEG_SS, // stack segment
	SEG_ES,
	SEG_FS,
	SEG_GS,
};

#define X86_SEGMENT_REGISTER_COUNT 6

#define X86_CONTROL_REGISTER_COUNT 8

typedef struct _X86_REG8 {
	BYTE l;
	BYTE h;
} X86_REG8;

typedef uint16_t X86_REG16;
typedef uint32_t X86_REG32;

typedef struct _X86_GENERAL_REGISTER {
	union {
		union {
			X86_REG8 r8;
			X86_REG16 r16;
		};
		X86_REG32 r32;
	};
} X86_GENERAL_REGISTER;

typedef X86_REG16 X86_SEGMENT_REGISTER;
typedef X86_REG32 X86_CONTROL_REGISTER;

typedef enum _X86_REGISTER_TYPE {
	X86_REGISTER_TYPE_GENERAL = 0,
	X86_REGISTER_TYPE_SEGMENT = 1,
	X86_REGISTER_TYPE_CONTROL = 2,
} X86_REGISTER_TYPE;

// memory

typedef struct _X86_MEMORY {
	uint32_t rom_base;
	uint32_t rom_end;
	uint32_t rom_size;
	BYTE* rom;

	uint32_t ram_base;
	uint32_t ram_end;
	uint32_t ram_size;
	BYTE* ram;
} X86_MEMORY;

inline int x86InitMemory(X86_MEMORY* mem, uint32_t rom_base, uint32_t rom_end, uint32_t ram_base, uint32_t ram_end) {
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
};
inline int x86ClearMemory(X86_MEMORY* mem) {
	// ram
	if (mem->ram != NULL)
		memset(mem->ram, 0, mem->ram_size);

	// rom
	if (mem->rom != NULL)
		memset(mem->rom, 0, mem->rom_size);

	return 0;
};
inline int x86FreeMemory(X86_MEMORY* mem) {
	// ram
	if (mem->ram != NULL)
	{
		free(mem->ram);
		mem->ram = NULL;
	}

	// rom
	if (mem->rom != NULL)
	{
		free(mem->rom);
		mem->rom = NULL;
	}

	return 0;
};

// cpu

#define CPU_REAL_MODE 0
#define CPU_PROTECTED_MODE 1

typedef struct _X86_GLOBAL_DESCRIPTOR {
	uint16_t limit;
	uint32_t base;
} X86_GLOBAL_DESCRIPTOR;

typedef struct _X86_CPU {
	X86_GENERAL_REGISTER registers[X86_GENERAL_REGISTER_COUNT];
	X86_SEGMENT_REGISTER segment_registers[X86_SEGMENT_REGISTER_COUNT];
	X86_CONTROL_REGISTER control_registers[X86_CONTROL_REGISTER_COUNT];
	X86_EFLAGS eflags;
	union {
		X86_REG16 ip;
		X86_REG32 eip;
	};
	union {
		X86_REG16 prev_ip;
		X86_REG32 prev_eip;
	};
	
	X86_MEMORY mem;
	int hlt;

	X86_GLOBAL_DESCRIPTOR gdt;
	X86_GLOBAL_DESCRIPTOR idt;

	int mode;
	
	char output_str[32];
	char addressing_str[32];
} X86_CPU;

inline int x86ResetCPU(X86_CPU* cpu) {
	int i;
	for (i = 0; i < X86_GENERAL_REGISTER_COUNT; ++i) {
		cpu->registers[i].r32 = 0;
	}
	for (i = 0; i < X86_SEGMENT_REGISTER_COUNT; ++i) {
		cpu->segment_registers[i] = 0;
	}
	for (i = 0; i < X86_CONTROL_REGISTER_COUNT; ++i) {
		cpu->control_registers[i] = 0;
	}

	x86InitEflags(&cpu->eflags);
	
	cpu->mode = CPU_REAL_MODE;
	cpu->segment_registers[SEG_CS] = 0xf000;
	cpu->eip = 0xfff0;
	cpu->prev_eip = 0x0;

	cpu->gdt.limit = 0;
	cpu->gdt.base = 0;

	cpu->idt.limit = 0;
	cpu->idt.base = 0;

	cpu->hlt = 0;

	memset(cpu->output_str, 0, sizeof(cpu->output_str));
	memset(cpu->addressing_str, 0, sizeof(cpu->addressing_str));

	return 0;
};
inline int x86InitCPU(X86_CPU* cpu, uint32_t rom_base, uint32_t rom_end, uint32_t ram_base, uint32_t ram_end) {
	
	if (x86InitMemory(&cpu->mem, rom_base, rom_end, ram_base, ram_end) != 0)
		return 1;

	x86ClearMemory(&cpu->mem);

	x86ResetCPU(cpu);
	
	return 0;
};
inline int x86FreeCPU(X86_CPU* cpu) {
	x86ResetCPU(cpu);
	x86FreeMemory(&cpu->mem);
	return 0;
};

inline uint32_t x86GetEffectiveAddress(X86_CPU* cpu, uint32_t address) {
	if (cpu->mode == CPU_REAL_MODE) {
		address = 0xFFFF0000 + (address & 0x0000FFFF);
	}
	return address;
}
inline void* x86GetCPUMemoryPtr(X86_CPU* cpu, uint32_t address) {
	address = x86GetEffectiveAddress(cpu, address);
	if (address >= cpu->mem.ram_base && address <= cpu->mem.ram_end) {
		return (cpu->mem.ram + address - cpu->mem.ram_base);
	}
	if (address >= cpu->mem.rom_base && address <= cpu->mem.rom_end) {
		return cpu->mem.rom + address - cpu->mem.rom_base;
	}	
	return NULL;
};

/* READ MEMORY */
inline BYTE x86CPUReadByte(X86_CPU* cpu, uint32_t address) {
	BYTE* ptr = (BYTE*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		return *ptr;
	return 0;
};
inline WORD x86CPUReadWord(X86_CPU* cpu, uint32_t address) {
	WORD* ptr = (WORD*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		return *ptr;
	return 0;
};
inline DWORD x86CPUReadDword(X86_CPU* cpu, uint32_t address) {
	DWORD* ptr = (DWORD*)x86GetCPUMemoryPtr(cpu, address);
	if (ptr != NULL)
		return *ptr;
	return 0;
};
inline uint32_t x86CPUReadMemory(X86_CPU* cpu, uint32_t address, uint32_t operand_size)
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

/* FETCH MEMORY AT EIP */
inline BYTE x86CPUFetchByte(X86_CPU* cpu, uint32_t* counter) {
	BYTE* ptr = (BYTE*)x86GetCPUMemoryPtr(cpu, cpu->eip + *counter);
	*counter += 1;
	if (ptr != NULL)
		return *ptr;
	return 0;
};
inline WORD x86CPUFetchWord(X86_CPU* cpu, uint32_t* counter) {
	WORD* ptr = (WORD*)x86GetCPUMemoryPtr(cpu, cpu->eip + *counter);
	*counter += 2;
	if (ptr != NULL)
		return *ptr;
	return 0;
};
inline DWORD x86CPUFetchDword(X86_CPU* cpu, uint32_t* counter) {
	DWORD* ptr = (DWORD*)x86GetCPUMemoryPtr(cpu, cpu->eip + *counter);
	*counter += 4;
	if (ptr != NULL)
		return *ptr;
	return 0;
};
inline uint32_t x86CPUFetchMemory(X86_CPU* cpu, uint32_t operand_size, uint32_t* counter)
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

/* Register read / write */
inline uint32_t x86CPUGetRegister(X86_CPU* cpu, uint32_t reg, uint32_t operand_size)
{
	uint32_t rm_v = 0;
	switch (operand_size) {
		case 1:
			if (reg <= 3)
				rm_v = cpu->registers[reg].r8.l;
			else
				rm_v = cpu->registers[reg - 4].r8.h;
			break;
		case 2:
			rm_v = cpu->registers[reg].r16;
			break;
		case 4:
			rm_v = cpu->registers[reg].r32;
			break;
	}

	return rm_v;
}
inline void x86CPUSetRegister(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t value)
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

/* IO read / write */
inline BYTE x86CPUGetIOByte(X86_CPU* cpu, uint32_t address) {

	if (address == 0xC000) {
		return 0x10;
	}
	return 0;
}
inline void x86CPUSetIOByte(X86_CPU* cpu, uint32_t operand_size, uint32_t address, uint32_t value) {

}

void x86CPUDumpRegisters(X86_CPU* cpu);

int x86CPUGetMemonic(X86_CPU* cpu, char* str);
void x86CPUGetRegisterMnemonic(char* buf, const char* fmt, uint32_t reg, uint32_t operand_size);
void x86CPUGetSegmentRegisterMnemonic(char* buf, const char* fmt, uint32_t reg);
void x86CPUGetControlRegisterMnemonic(char* buf, uint32_t reg);
void x86CPULoadSegmentDescriptor(X86_CPU* cpu, int segment, uint16_t selector);

int x86CPUExecute(X86_CPU* cpu);

#endif
