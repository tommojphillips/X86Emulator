// main.c

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "cpu.h"
#include "cpu_instruction.h"
#include "input.h"

#include "type_defs.h"
#include "mem_tracking.h"
#include "file.h"

X86_CPU cpu;
BREAKPOINT cpu_breakpoint = { true, 0x0 };

#ifdef OUTPUT_BUFFER
#define OUTPUT_BUFFER_SIZE 4096
char* output_buffer = NULL;
#endif

void load_rom(const uint32_t ROM_BASE, const uint32_t ROM_END);
void load_test_instructions(X86_CPU* cpu);

int replicate_buffer(uint32_t from, uint32_t to, uint8_t* buffer, uint32_t buffersize);
void output_cpu_mnemonic(int result);

int main(int argc, char* argv[]);

#ifdef OUTPUT_BUFFER
void flush_output_buffer() {
	if (output_buffer != NULL && strlen(output_buffer) > 0) {
		printf(output_buffer);
		output_buffer[0] = '\0';
	}
}
#endif

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

void load_test_instructions(X86_CPU* cpu) 
{
	int i = 0;
	BYTE* ram;
	BYTE* rom;

	rom = (BYTE*)x86GetCPUMemoryPtr(cpu, 0xfff0);

	/* ROM */

	rom[i++] = 0xEA; // jmp 0x08:0x0
	rom[i++] = 0x00;
	rom[i++] = 0x00;
	rom[i++] = 0x00;
	rom[i++] = 0x00;
	rom[i++] = 0x08;
	rom[i++] = 0x00;

	/* RAM */
	i = 0;
	ram = (BYTE*)x86GetCPUMemoryPtr(cpu, 0);

	/* enable protected mode */

	ram[i++] = 0x0F; // mov eax, cr0
	ram[i++] = 0x20;
	ram[i++] = 0xC0;

	ram[i++] = 0x0C; // or al, 0x1
	ram[i++] = 0x01;

	ram[i++] = 0x0F; // mov cr0, eax
	ram[i++] = 0x22;
	rom[i++] = 0xC0;

	ram[i++] = 0xEA; // jmp 0x08:0x0 ; jmp to RAM 0x1000.
	ram[i++] = 0x00;
	ram[i++] = 0x10;
	ram[i++] = 0x00;
	ram[i++] = 0x00;
	ram[i++] = 0x08;
	ram[i++] = 0x00;

	/* in protected mode */

	i = 0x1000;
	ram = cpu->mem.ram;


	ram[i++] = 0x80; // ADD r/m8, imm8
	ram[i++] = 0xC4;
	ram[i++] = 0x02;

	ram[i++] = 0x83; // ADD reg32, imm8
	ram[i++] = 0xC0;
	ram[i++] = 0x02;

	ram[i++] = 0x66; // ADD AX, imm16
	ram[i++] = 0x05;
	ram[i++] = 0x04;
	ram[i++] = 0x00;

	ram[i++] = 0x05; // ADD EAX, imm32
	ram[i++] = 0x00;
	ram[i++] = 0x10;
	ram[i++] = 0x00;
	ram[i++] = 0x00;

	ram[i++] = 0xF4; // hlt ; hlt the cpu.
}
void load_rom(const uint32_t ROM_BASE, const uint32_t ROM_END)
{
	/*int code_offset = 0x1000;
	load_test_instructions(cpu.mem.ram, code_offset);
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

	filename = "Q:\\CPP\\XboxBiosTool\\bin\\bios\\og_1_0\\4627.bin";//"D:\\builds\\fre\\boot\\xboxrom.bin";
	if (readFileIntoBuffer(filename, (cpu.mem.rom), cpu.mem.rom_size, &file_size, 0) == 0) {
		printf("loaded %s into ROM at 0x%x\n", filename, ROM_BASE);
	}
	// replicate bios across space
	if (file_size < cpu.mem.rom_size) {
		replicate_buffer(file_size, cpu.mem.rom_size, cpu.mem.rom, cpu.mem.rom_size);
	}

	filename = "mcpx_1.0.bin";
	if (readFileIntoBuffer(filename, (cpu.mem.rom + cpu.mem.rom_size - 512), 512, &file_size, 0) == 0) {
		printf("loaded %s into ROM at 0x%x\n", filename, ROM_END - 512 + 1);
	}
}

void output_cpu_mnemonic(int result)
{
	const char* addr_fmt = "\n\t%08x: ";
	uint32_t address = x86GetEffectiveAddress(&cpu, cpu.eip);

#ifdef OUTPUT_MNEMONIC
#ifdef OUTPUT_BUFFER // BUFFERED OUTPUT
	if (strlen(output_buffer) >= OUTPUT_BUFFER_SIZE - sizeof(cpu.output_str)) {
		flush_output_buffer();
	}
	if (cpu.output_str[0] != '\0') {
		sprintf(output_buffer + strlen(output_buffer), cpu.output_str);
	}
	sprintf(output_buffer + strlen(output_buffer), addr_fmt, address);
#else // !OUTPUT_BUFFER
	if (cpu.output_str[0] != '\0') {
		printf(cpu.output_str);
	}
	printf(addr_fmt, address);
#endif // OUTPUT_BUFFER
#else // !OUTPUT_MNEMONIC
	if (result != 0) {
		if (cpu.output_str[0] != '\0') {
			printf(cpu.output_str);
		}
		printf(addr_fmt, address);
	}
#endif // OUTPUT_MNEMONIC
}

int main(int argc, char* argv[])
{
	const uint32_t ROM_BASE = 0xf0000000;
	const uint32_t ROM_END = 0xffffffff;
	const uint32_t MEM_SIZE = 0x07ffffff;
	int result = 0;

#ifdef OUTPUT_BUFFER
	output_buffer = (char*)malloc(OUTPUT_BUFFER_SIZE);
	if (output_buffer == NULL) {
		printf("Error: Out of Memory\n");
		return 1;
	}
	output_buffer[0] = '\0';
#endif

	result = x86InitCPU(&cpu, ROM_BASE, ROM_END, 0, MEM_SIZE);
	if (result != 0) {
		printf("Error: Out of Memory\n");
		return 1;
	}

	// enable TRAP FLAG; single step program.
	cpu.eflags.TF = 1;

	//load_rom(ROM_BASE, ROM_END);
	load_test_instructions(&cpu);
		
	cpu_breakpoint.set = true;
	//cpu_breakpoint.address = 0xfffffebc;	// end of xcode interpreter
	//cpu_breakpoint.address = 0xfffffedd;	// start of rc4 decryption
	cpu_breakpoint.address = 0xfffffefb;	// end of rc4 key init. (loop of 256).
	//cpu_breakpoint.address = 0xffffff7d;	// 1 decryption loop. (rom->ram)
	
	while (result == 0) {
		output_cpu_mnemonic(result);

		result = input_loop();
		if (result != 0)
			break;
		
		result = x86CPUExecute(&cpu);
	}

#ifdef OUTPUT_BUFFER
	flush_output_buffer();
	if (output_buffer != NULL) {
		free(output_buffer);
	}
#endif

	switch (result) {
		case X86_CPU_ERROR_FATAL:
			printf("\nfatal error\n");
			break;
		case X86_CPU_ERROR_UD:
			printf("\n\t%08x: ud: %s\n", cpu.eip, cpu.output_str);
			break;
		case X86_CPU_ERROR_HLT:
			printf("%s\n", cpu.output_str);
			break;
	}

	x86CPUDumpRegisters(&cpu);
	x86FreeCPU(&cpu);

	memtrack_report();

	return 0;
}
