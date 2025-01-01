// cpu_memory.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef _CPU_MEMORY_H
#define _CPU_MEMORY_H

#include <stdint.h>
#include "type_defs.h"

uint32_t x86GetEffectiveAddress(X86_CPU* cpu, uint32_t address);

/* READ MEMORY */
void* x86GetCPUMemoryPtr(X86_CPU* cpu, uint32_t address);
BYTE x86CPUReadByte(X86_CPU* cpu, uint32_t address);
WORD x86CPUReadWord(X86_CPU* cpu, uint32_t address);
DWORD x86CPUReadDword(X86_CPU* cpu, uint32_t address);
uint32_t x86CPUReadMemory(X86_CPU* cpu, uint32_t address, uint32_t operand_size);

/* WRITE MEMORY */
void x86CPUWriteByte(X86_CPU* cpu, uint32_t address, BYTE value);
void x86CPUWriteWord(X86_CPU* cpu, uint32_t address, WORD value);
void x86CPUWriteDword(X86_CPU* cpu, uint32_t address, DWORD value);


/* FETCH MEMORY AT EIP */
BYTE x86CPUFetchByte(X86_CPU* cpu, uint32_t* counter);
WORD x86CPUFetchWord(X86_CPU* cpu, uint32_t* counter);
DWORD x86CPUFetchDword(X86_CPU* cpu, uint32_t* counter);
uint32_t x86CPUFetchMemory(X86_CPU* cpu, uint32_t operand_size, uint32_t* counter);

/* IO read / write */
BYTE x86CPUGetIOByte(X86_CPU* cpu, uint32_t address);
void x86CPUSetIOByte(X86_CPU* cpu, uint32_t operand_size, uint32_t address, uint32_t value);

#endif
