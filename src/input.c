// input.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include "input.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef CPU_INPUT
#include <stdlib.h>
#include <conio.h>
#endif

#include "cpu.h"

extern X86_CPU cpu;
extern BREAKPOINT cpu_breakpoint;
extern char* output_buffer;
void flush_output_buffer();

#ifdef CPU_INPUT
int kb_frames = 0;
#endif

#ifdef CPU_INPUT
int get_num(char* ch, uint32_t* num) {
	int base;
	if ((ch[0] == '0' && (ch[1] == 'x' || ch[1] == 'X')) || ch[0] == 'x' || ch[0] == 'X')
		base = 16;
	else if ((ch[0] == '0' && (ch[1] == 'b' || ch[1] == 'B')) || ch[0] == 'b' || ch[0] == 'B')
		base = 2;
	else
		base = 10;

	*num = (uint32_t)strtoul(ch, NULL, base);

	return 0;
}
void wait_for_enter(char* ch, uint32_t size) {
	int c, base = 0;
	uint32_t i = 0;
	while ((c = getchar()) != '\n' && c != EOF && cpu.hlt == 0) {
		if (i > size - 1)
		{
			ch[i - 1] = '\0';
			break;
		}
		ch[i++] = c;
	}
}
int input()
{
	if (cpu.eflags.TF == 0) {
		if (kb_frames < 1000) {
			kb_frames++;
			return 0;
		}
		kb_frames = 0;
	}

	if (!_kbhit())
		return 0;

	char ch = _getch();

	switch (ch) {
		case 'x': case 'X':
			cpu.eflags.TF = !cpu.eflags.TF;
			return 2;

		case 13: // enter
			if (cpu.eflags.TF == 1)
				return 2;
			break;

		case 'd': case 'D':
			if (cpu.eflags.TF == 1) {
				x86CPUDumpRegisters(&cpu);
				printf("\t%08x: ", cpu.eip);
			} break;

		case '.':
			cpu.hlt = 1;
			break;

		case 'z': case 'Z':
			cpu.eip = 0x1000;
			printf("\n\t%08x: ", cpu.eip);
			break;

		case 'j': case 'J': {
			printf("\neip: ");
			char ch[32] = { 0 };
			uint32_t c;
			int rel = 0;
			wait_for_enter(ch, 32);
			if (ch[0] == '\0')
				break;
			if (ch[0] == '-' || ch[0] == '+')
				rel = 1;

			get_num(ch, &c);

			if (rel)
				cpu.eip += (int)c;
			else
				cpu.eip = c;
			printf("\t%08x: ", cpu.eip);
		} break;

		case 'e': case 'E': {
			printf("\naddress: ");
			char ch[32] = { 0 };
			int address;
			int value;
			int rel = 0;
			wait_for_enter(ch, 32);
			if (ch[0] == '\0')
				break;

			if (ch[0] == '-' || ch[0] == '+')
				rel = 1;

			get_num(ch, &address);

			memset(ch, 0, 32);
			printf("value: ");
			wait_for_enter(ch, 32);
			if (ch[0] == '\0')
				break;

			get_num(ch, &value);

			if (rel)
				address += cpu.eip;

			void* ptr = x86GetCPUMemoryPtr(&cpu, address);
			*(char*)ptr = (char)value;

			printf("\t%08x: ", cpu.eip);
		} break;

		case 'b': case 'B': {
			printf("\nset breakpoint. on, off\naddress: ");
			char ch[32] = { 0 };
			int address;
			int rel = 0;
			wait_for_enter(ch, 32);
			if (ch[0] == '\0')
				break;

			if (strcmp("off", ch) == 0) {
				cpu_breakpoint.set = false;
				printf("Breakpoint OFF\n");
			}
			else if (strcmp("on", ch) == 0) {
				cpu_breakpoint.set = true;
				printf("Breakpoint ON %08x\n", cpu_breakpoint.address);
			}
			else
			{
				if (ch[0] == '-' || ch[0] == '+')
					rel = 1;

				get_num(ch, &address);

				if (rel)
					address += cpu.eip;

				cpu_breakpoint.set = true;
				cpu_breakpoint.address = address;
				printf("Breakpoint ON %08x\n", cpu_breakpoint.address);
			}

			printf("\t%08x: ", cpu.eip);
		} break;
	}

	return 0;
}
void single_step() {
	if (cpu.eflags.TF == 1) {
		if (cpu.output_str[0] != '\0')
			printf("%s\n\t%08x: ", cpu.output_str, cpu.eip);
		else
			printf("\n\t%08x: ", cpu.eip);
	}
}
#endif

int input_loop()
{
#ifndef OUTPUT_MNEMONIC
	single_step();
#endif

	do {
		if (input() != 0)
			break;

#ifdef OUTPUT_BUFFER
		if (cpu.eflags.TF == 1) {
			flush_output_buffer();
		}
#endif

		if (cpu_breakpoint.set && cpu.eflags.TF == 0) {
			if (cpu_breakpoint.address == cpu.eip) {
				cpu.eflags.TF = 1;
#ifdef OUTPUT_BUFFER
				sprintf(output_buffer+strlen(output_buffer), "Breakpoint hit\n\t%08x: ", cpu.eip);
#else
				printf("Breakpoint hit\n\t%08x: ", cpu.eip);
#endif
			}
		}
	} while (cpu.eflags.TF == 1 && cpu.hlt == 0);

	return cpu.hlt;
}