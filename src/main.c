// main.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cpu.h"
#include "cpu_instruction.h"
#include "cpu_memory.h"
#include "cpu_mnemonics.h"
#include "input.h"

#include "type_defs.h"
#include "mem_tracking.h"
#include "file.h"

X86_CPU cpu;
BREAKPOINT* cpu_breakpoint = NULL;
uint32_t cpu_breakpoint_index = 0;
uint32_t cpu_breakpoint_count = 0;

void load_rom(const uint32_t ROM_BASE, const uint32_t ROM_END);
int replicate_buffer(uint32_t from, uint32_t to, uint8_t* buffer, uint32_t buffersize);
int output_cpu_mnemonic();
int main(int argc, char* argv[]);

int replicate_buffer(uint32_t from, uint32_t to, uint8_t* buffer, uint32_t buffersize)
{
	// replicate buffer based

	uint32_t offset = 0;
	uint32_t new_size = 0;

	if (from >= to || buffersize < to)
		return 1;

	offset = from;
	while (offset < to) {
		new_size = offset * 2;
		if (offset < new_size && to >= new_size) {
			memcpy(buffer + offset, buffer, offset);
		}

		offset = new_size;
	}

	return 0;
}

void load_rom(const uint32_t ROM_BASE, const uint32_t ROM_END)
{
	/*int code_offset = 0x1000;
	if (argc == 3) {
		if (strcmp(argv[1], "-in") == 0) {
			const char* filename = argv[2];
			if (readFileIntoBuffer(filename, cpu.mem.ram, cpu.mem.ram_size, NULL, 0) == 0) {
				printf("loaded %s into RAM at 0x%x\n", filename, 0);
				code_offset = 0;
			}
		}
	}*/

	const char* filename;
	uint32_t file_size;

#if 1
	//filename = "Q:\\CPP\\XboxBiosTool\\bin\\bios\\og_1_1\\4817.bin";
	filename = "Q:\\CPP\\XboxBiosTool\\bin\\bios\\og_1_0\\3944.bin";
	//filename = "Q:\\CPP\\XboxBiosTool\\bin\\bios\\custom\\cromwell.bin";
	//filename = "Q:\\CPP\\XboxBiosTool\\bin\\bios\\custom\\cerbios_v2.3.1.bin";
	//filename = "D:\\builds\\fre\\boot\\xboxrom.bin";
	//filename = "chihiro_xbox_bios.bin";
	if (readFileIntoBuffer(filename, (cpu.mem.rom), cpu.mem.rom_size, &file_size, 0) == 0) {
		printf("loaded %s into ROM at 0x%x\n", filename, ROM_BASE);
	}
	// replicate bios across space
	if (file_size < cpu.mem.rom_size) {
		replicate_buffer(file_size, cpu.mem.rom_size, cpu.mem.rom, cpu.mem.rom_size);
	}

	filename = "mcpx_1.0.bin";
	//filename = "mouse_rev0.bin";
	if (readFileIntoBuffer(filename, (cpu.mem.rom + cpu.mem.rom_size - 512), 512, &file_size, 0) == 0) {
		printf("loaded %s into ROM at 0x%x\n", filename, ROM_END - 512 + 1);
	}
#else
	filename = "Q:/ASM/bldr16/bldr16.bin";
	if (readFileIntoBuffer(filename, (cpu.mem.rom + cpu.mem.rom_size - 512), 512, &file_size, 0) == 0) {
		printf("loaded %s into ROM\n", filename);
	}

	filename = "Q:/ASM/bldr32/bldr32.bin";
	if (readFileIntoBuffer(filename, (cpu.mem.rom + cpu.mem.rom_size - 512), 512, &file_size, 0) == 0) {
		printf("loaded %s into ROM\n", filename);
	}
#endif
}

#define OUTPUT_MNEMONIC

int output_cpu_mnemonic()
{
#ifndef OUTPUT_MNEMONIC
	if (cpu.eflags.TF == 0) {
		return 0;
	}
#endif

	if (cpu.eip_ptr != NULL) {
		if (x86CPUGetMnemonic(&cpu, NULL) != 0) {
			printf("failed to get mnemonic\n");
			return 1;
		}
	}
	uint32_t address = x86GetEffectiveAddress(&cpu, cpu.eip);
	printf("%s\n\t%08x: ", cpu.output_str, address);
	return 0;
}

int init_breakpoints()
{
	cpu_breakpoint_count = 128;
	cpu_breakpoint_index = 0;
	cpu_breakpoint = (BREAKPOINT*)malloc(sizeof(BREAKPOINT) * cpu_breakpoint_count);
	if (cpu_breakpoint == NULL) {
		return 1;
	}
	memset(cpu_breakpoint, 0, sizeof(BREAKPOINT) * cpu_breakpoint_count);
	return 0;
}
void free_breakpoints()
{
	if (cpu_breakpoint != NULL) {
		free(cpu_breakpoint);
		cpu_breakpoint = NULL;
		cpu_breakpoint_count = 0;
		cpu_breakpoint_index = 0;
	}
}
void load_breakpoints()
{
	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	//cpu_breakpoint[cpu_breakpoint_index++].address = 0xfffffebc;	// end of xcode interpreter

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	//cpu_breakpoint[cpu_breakpoint_index++].address = 0xfffffed2;	// end of wrmsr loop (enable cache)

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	cpu_breakpoint[cpu_breakpoint_index++].address = 0xfffffedd;	// rc4_key init

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	cpu_breakpoint[cpu_breakpoint_index++].address = 0xfffffefb;	// rc4_key init key

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	//cpu_breakpoint[cpu_breakpoint_index++].address = 0xffffff3c;	// rc4

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	cpu_breakpoint[cpu_breakpoint_index++].address = 0xffffff7f;	// 1 decryption loop. (rom->ram)

	cpu_breakpoint[cpu_breakpoint_index].set = true;
	cpu_breakpoint[cpu_breakpoint_index++].address = 0xffffff7f + 2;	// end of decryption . (rom->ram)

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	//cpu_breakpoint[cpu_breakpoint_index++].address = 0xffffff6c;	// mov encrypted byte from rom

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	//cpu_breakpoint[cpu_breakpoint_index++].address = 0xffffff77;	// mov decrypted byte to ram

	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	//cpu_breakpoint[cpu_breakpoint_index++].address = 0xffffff26;	// invalid addressing mode..  mov [bh+dh*1-1], al
	//cpu_breakpoint[cpu_breakpoint_index].set = true;
	//cpu_breakpoint[cpu_breakpoint_index++].address = 0xffffff19;	// invalid addressing mode..  mov al, [ch+cl*1+0]


	/* PCI_WRITE xcode
		fffffe4a: cmp ebx, 0x80000880
		fffffe50: jnz 3
		fffffe55: mov eax, ebx
		fffffe57: mov dx, 0xcf8
		fffffe5b: out dx, eax
		fffffe5c: add dl, 0x4
		fffffe5f: mov eax, ecx
		fffffe61: out dx, eax*/
	cpu_breakpoint[cpu_breakpoint_index].set = true;
	cpu_breakpoint[cpu_breakpoint_index++].address = 0xfffffe4a;
}

int main(int argc, char* argv[])
{
	const uint32_t ROM_BASE = 0xf0000000;
	const uint32_t ROM_END = 0xffffffff;
	const uint32_t MEM_SIZE = 0x07ffffff;
	int result = 0;

	result = x86InitCPU(&cpu, ROM_BASE, ROM_END, 0, MEM_SIZE);
	if (result != 0) {
		printf("error: Out of Memory\n");
		goto Cleanup;
	}

	result = init_breakpoints();
	if (result != 0) {
		printf("error: Out of Memory\n");
		goto Cleanup;
	}

	// enable TRAP FLAG; single step program.
	cpu.eflags.TF = 1;

	load_rom(ROM_BASE, ROM_END);

	load_breakpoints();
		
	while (result == 0) {

		result = output_cpu_mnemonic();
		//if (result != 0)
		//	break;

		result = input_loop();
		if (result != 0)
			break;
		
		result = x86CPUExecute(&cpu);
		if (result != 0)
			break;

	}

	switch (result) {
		case X86_CPU_ERROR_UD: {
			uint32_t address = x86GetEffectiveAddress(&cpu, cpu.eip);
			printf("\n\t%08x: ud: %s\n", address, cpu.output_str);
		} break;
		case X86_CPU_ERROR_HLT:
			printf("%s\n", cpu.output_str);
			break;
	}

	x86CPUDumpRegisters(&cpu);

Cleanup:
	x86FreeCPU(&cpu);	
	free_breakpoints();
	memtrack_report();

	return result;
}
