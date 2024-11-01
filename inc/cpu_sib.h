// cpu_sib.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef _CPU_SIB_H
#define _CPU_SIB_H

#include <stdint.h>

#include "cpu.h"

int addressing_mode_reg_ptr(X86_CPU* cpu, uint32_t reg, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type);
int addressing_mode_reg(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t operand_size, uint32_t* value, INSTRUCTION_RM* type);
int addressing_mode_reg_ptr_disp(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t displacement_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter);
int addressing_mode_sib_reg_reg_disp8(X86_CPU* cpu, uint32_t address_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter);
int addressing_mode_sib_reg_reg_disp32(X86_CPU* cpu, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter);
int addressing_mode_sib_reg_reg(X86_CPU* cpu, uint32_t displacement_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter);
int addressing_mode_sib_disp(X86_CPU* cpu, uint32_t address_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter);
int addressing_mode_disp32(X86_CPU* cpu, uint32_t* address, uint32_t* value, INSTRUCTION_RM* type, uint32_t* counter);
int get_addressing_mode(X86_CPU* cpu, X86_MOD_RM* mode, uint32_t address_size, uint32_t operand_size, uint32_t* address, uint32_t* value, INSTRUCTION_TYPE* type, uint32_t* counter);
#endif
