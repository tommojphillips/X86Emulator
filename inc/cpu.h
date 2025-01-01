// cpu.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef X86_CPU_H
#define X86_CPU_H

#include <stdint.h>
#include <string.h>

#include "cpu_eflags.h"
#include "cpu_alu.h"

#include "type_defs.h"
#include "mem_tracking.h"

#define X86_GENERAL_REGISTER_COUNT 8
#define X86_SEGMENT_REGISTER_COUNT 6
#define X86_CONTROL_REGISTER_COUNT 8
#define CPU_REAL_MODE 0
#define CPU_PROTECTED_MODE 1

#define X86_CPU_MAX_INSTRUCTION_SIZE 15

typedef enum _X86_CPU_ERROR {
	X86_CPU_ERROR_SUCCESS = 0,
	X86_CPU_ERROR_FATAL,
	X86_CPU_ERROR_HLT,
	X86_CPU_ERROR_UD,
	X86_CPU_ERROR_DECODED,
} X86_CPU_ERROR;

/*32bit GENERAL REGISTER*/
enum {
	REG_EAX = 0,
	REG_ECX = 1,
	REG_EDX = 2,
	REG_EBX = 3,
	REG_ESP = 4,
	REG_EBP = 5,
	REG_ESI = 6,
	REG_EDI = 7,
};

/*16bit GENERAL REGISTER*/
enum {
	REG_AX = 0,
	REG_CX = 1,
	REG_DX = 2,
	REG_BX = 3,
	REG_SP = 4,
	REG_BP = 5,
	REG_SI = 6,
	REG_DI = 7,
};

/*8bit GENERAL REGISTER*/
enum {
	REG_AL = 0,
	REG_CL = 1,
	REG_DL = 2,
	REG_BL = 3,
	REG_AH = 4,
	REG_CH = 5,
	REG_DH = 6,
	REG_BH = 7,
};

/*SEGMENT*/
enum {
	SEG_ES = 0,
	SEG_CS = 1,
	SEG_SS = 2,
	SEG_DS = 3,
	SEG_FS = 4,
	SEG_GS = 5,
};

/*REGISTER TYPE*/
typedef struct _X86_REG8 {
	BYTE l;
	BYTE h;
} X86_REG8;
typedef uint16_t X86_REG16;
typedef uint32_t X86_REG32;

/*REGISTERS*/
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

/*DESCRIPTORS*/
typedef struct _X86_SEGMENT_DESCRIPTOR {
	uint32_t base;
	uint16_t limit;
	uint8_t access;
	uint8_t flags;
	uint8_t default_size;
} X86_SEGMENT_DESCRIPTOR;
typedef struct _X86_GLOBAL_DESCRIPTOR {
	uint32_t base; // linear base address
	uint16_t limit; // table limit (num of bytes in table)
} X86_GLOBAL_DESCRIPTOR;
typedef struct _X86_LOCAL_DESCRIPTOR {
	uint32_t base; // linear base address ( linear address of byte 0 of the LDT segment )
	uint16_t limit; // segment limit (num of bytes in segment)
	uint16_t selector; // segment selector
} X86_LOCAL_DESCRIPTOR;

/*CPU MEMORY*/
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

/*CPU*/
typedef struct _X86_CPU {
	X86_GENERAL_REGISTER registers[X86_GENERAL_REGISTER_COUNT];
	X86_SEGMENT_REGISTER segment_registers[X86_SEGMENT_REGISTER_COUNT];
	X86_SEGMENT_DESCRIPTOR segment_descriptors[X86_SEGMENT_REGISTER_COUNT];
	X86_CONTROL_REGISTER control_registers[X86_CONTROL_REGISTER_COUNT];
	X86_EFLAGS eflags;
	union {
		X86_REG16 ip;
		X86_REG32 eip;
	};
	uint8_t* eip_ptr;
	
	X86_MEMORY mem;
	int hlt;

	X86_GLOBAL_DESCRIPTOR gdtr;
	X86_GLOBAL_DESCRIPTOR idtr;
	X86_LOCAL_DESCRIPTOR ldtr;

	uint64_t* gdt;
	uint64_t* idt;
	uint64_t* ldt;

	int mode;
	
	char output_str[32];
	char addressing_str[32];
} X86_CPU;

typedef struct _PREFIX_BYTE_STRUCT {
	BYTE segment_override;
	BYTE byte_66 : 1;
	BYTE byte_67 : 1;
	BYTE byte_f3 : 1;
	BYTE byte_0f : 1;
} PREFIX_BYTE_STRUCT;

int x86CPUFetchPrefixBytes(X86_CPU* cpu, PREFIX_BYTE_STRUCT* prefix, X86_OPCODE* opcode, uint32_t* counter);

/*CPU MEMORY*/
int x86InitMemory(X86_MEMORY* mem, uint32_t rom_base, uint32_t rom_end, uint32_t ram_base, uint32_t ram_end);
int x86ClearMemory(X86_MEMORY* mem);
int x86FreeMemory(X86_MEMORY* mem);

/*CPU*/
int x86ResetCPU(X86_CPU* cpu);
int x86InitCPU(X86_CPU* cpu, uint32_t rom_base, uint32_t rom_end, uint32_t ram_base, uint32_t ram_end);
int x86FreeCPU(X86_CPU* cpu);

/* Register read / write */
uint32_t x86CPUGetRegister(X86_CPU* cpu, uint32_t reg, uint32_t operand_size);
int32_t x86CPUGetRegisterSigned(X86_CPU* cpu, uint32_t reg, uint32_t operand_size);
void x86CPUSetRegister(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t value);

void x86CPULoadSegmentDescriptor(X86_CPU* cpu, uint16_t selector, X86_SEGMENT_DESCRIPTOR* descriptor);

void x86CPUDumpRegisters(X86_CPU* cpu);
void x86CPUGetDefaultSize(X86_CPU* cpu, PREFIX_BYTE_STRUCT* prefix, uint8_t* operand_size, uint8_t* address_size);

int x86CPUExecute(X86_CPU* cpu);

#endif
