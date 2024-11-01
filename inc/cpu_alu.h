// alu.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef ALU_H
#define ALU_H

#include <stdint.h>

#include "type_defs.h"
#include "mem_tracking.h"

#include "cpu_eflags.h"
#include "cpu_instruction.h"

int x86Alu(X86_EFLAGS* eflags, INSTRUCTION_TYPE type, uint32_t operand1, uint32_t operand2, uint32_t operand_size, uint32_t* result);

#endif
