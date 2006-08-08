#ifndef _LIBEMU_H_
#define _LIBEMU_H_

/* libemu API */

/* General functions */
void emu_init(int argc, char** argv);
void emu_main();
void emu_message(char* message);

/* CPU functions */
int emu_cpu_init(char* filename);
void emu_cpu_set_breakpoint(unsigned long int pos, int one_time_only);
char* emu_cpu_name;
char* (*emu_cpu_register_name)(int n);
unsigned int (*emu_cpu_register_value)(int n);
char* (*emu_cpu_flag_name)(int n);
unsigned char (*emu_cpu_flag_value)(int n);
char* (*emu_cpu_debug)(unsigned long addr, int* num_cycles, int* bytes);
void (*emu_cpu_reset)();
unsigned long int (*emu_cpu_ip)();
int (*emu_cpu_step)(int* cycles);

/* libemu RAM API */
void emu_mem_init(unsigned long sz);
void emu_mem_init_k(unsigned int sz);
void emu_mem_set_direct(unsigned long int pos, unsigned char data);
void emu_mem_set(unsigned long int pos, unsigned char data);
unsigned char emu_mem_get(unsigned long int pos);

/* libemu ROM API */
void emu_rom_load(char* filename, long pos);

#endif
