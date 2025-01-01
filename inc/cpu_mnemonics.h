// cpu_mnemonics.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef CPU_MNEMONICS
#define CPU_MNEMONICS

void x86CPUGetRegisterMnemonic(char* buf, const char* fmt, uint32_t reg, uint32_t operand_size);
void x86CPUGetSegmentRegisterMnemonic(char* buf, const char* fmt, uint32_t reg);
void x86CPUGetControlRegisterMnemonic(char* buf, uint32_t reg);

int x86CPUGetMnemonic(X86_CPU* cpu, char* str);

#endif
