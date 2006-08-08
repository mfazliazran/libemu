/*
 * Describe your processor here.
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdev.h"

EXPORT char dev_type[] = "cpu";

/* Fill in the name of the CPU */
EXPORT char dev_cpu_name[] = "x21";

/* Program Counter (also known as Instruction Pointer):
 * the memory address that contains the opcode currently being executed. */
unsigned long int PC;

/* 
 * Registers & Flags:
 *
 * Put your registers and flags here. For example, if your processor has
 * two registers, one with 16 bits and the other with 8 bits, the 
 * appropriate code would be:
 *
 * unsigned char X;
 * unsigned short int Y; 
 *
 * If your processor has a flag Z and another N, then the appropriate code 
 * would be:
 * 
 * unsgined char N, Z;
 * 
 */
unsigned char A;
unsigned char Z;

/* The following functions (inside the DEBUG directive) are used only by the
 * debugger, and will be stripped off when the final version is created. */
#ifdef DEBUG

/* This variable will be used to return the debugger data in a persistent 
 * way. */
char buffer[100];

/* You must implement this function.
 *
 * This function will return the register names. For example, if your processor
 * has two registers, X and Y, when n == 0, the function would return "X", and
 * when n == 1, it would return "Y". The funcion must return NULL for every 
 * other value of n. */
EXPORT char* dev_cpu_register_name(int n)
{
	switch(n)
	{
		case 0:	return "A";
		default: return NULL;
	}
}

/* You must implement this function.
 *
 * This function will return the register values. For example, if your processor
 * has two registers, X and Y, when n == 0, the function would return the value 
 * in X, and when n == 1, it would return the value in Y. The value of the 
 * register n must match the register n passed on the function register_name. */
EXPORT unsigned int dev_cpu_register_value(int n)
{
	switch(n)
	{
		case 0: return A;
		default: return 0;
	}
}

/* You must implement this function.
 * 
 * This function will return the flag names. For example, if your processor
 * has two flags, N and Z, when n == 0, the function would return "N", and
 * when n == 1, it would return "Z". The funcion must return NULL for every other
 * value of n. */
EXPORT char* dev_cpu_flag_name(int n)
{
	switch(n)
	{
		case 0: return "Z";
		default: return NULL;
	}
}

/* You must implement this function.
 *
 * This function will return the flag values. For example, if your processor
 * has two flags, N and Z, when n == 0, the function would return the value 
 * in N, and when n == 1, it would return the value in Z. The value of the flag
 * n must match the flag n passed on the function flag_name. 
 *
 * The flag is boolean, so the return value should be 0 for FALSE, or any
 * other value for TRUE. */
EXPORT unsigned char dev_cpu_flag_value(int n)
{
	switch(n)
	{
		case 0: return Z;
		default: return 0;
	}
}

/* This code will return the human-readable opcode from a given position in
 * memory. It'll be used bu the debugger to display the data. The data must be
 * returned in a persistent way, using the global variable buffer, or a segfault
 * might happen. The parameters are:
 *
 * - addr -> the address to be debugger
 * - num_cycles -> return the number of cycles that would be spent
 * - bytes -> the number of bytes used by the instruction. It'll be more than
 *            one if the instruction recieves a parameter. 
 *
 * The function MUST return the string "Invalid" (without quotes) in case of
 * invalid opcode. */
EXPORT char* dev_cpu_debug(unsigned long addr, int *num_cycles, int *bytes)
{
	switch(dev_mem_get(addr))
	{
		/*
		case 0: // ADD X Y
			sprintf(buffer, "ADD X Y");
			*num_cycles = 3;
			*bytes = 1;
			break;
		*/
		case 'A': // MOV
			sprintf(buffer, "MOV %d %d", dev_mem_get(addr+1), dev_mem_get(addr+2));
			*num_cycles = 2;
			*bytes = 3;
			break;
		default:
			sprintf(buffer, "");
			*num_cycles = 0;
			*bytes = 0;
	}
	return buffer;	
}

#endif

/* The next functions are going to be part of the final emulator. */

/* You must implement this function.
 *
 * Resets the emulator. Everything must be set as how the CPU would look like
 * when turned on. */
EXPORT void dev_cpu_reset()
{
	PC = 0;
	A = 0;
	Z = 0;
	dev_message("CPU has been restarted!");
}

/* This function returns the Instruction Pointer. */
EXPORT unsigned long int dev_cpu_ip()
{
	return PC; /* original_pc */
}

/* You must implement this function.
 *
 * This function performs a CPU step. Here is where the magic happens :-)
 * The function must return 0 in case of success, or -1 in case of faluire
 * (a bad opcode, for example). The num_cycles parameter is used to return
 * the number of cycles spent on the step. You must advance the Instruction
 * Pointer manually.
 *
 * The skeleton below is from a very simple CPU, and more complex CPUs might
 * have to modify the whole function. 
 *
 * This code will run hundreds of times per second, so make it run as fast as
 * possible, but remember to mantain it C ANSI and portable. */
EXPORT int dev_cpu_step(int *num_cycles)
{
	switch(dev_mem_get(PC))
	{
		/*
		case 0: // ADD X Y
			X = X + Y;
			PC++;
			*num_cycles = 3;
			break;
		*/
		case 0: // MOV
			PC++;
			*num_cycles = 2;
			break;
		default:
			*num_cycles = 0;
			return -1;
	}
	return 0;
}
