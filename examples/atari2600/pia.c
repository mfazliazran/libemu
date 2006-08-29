/*
 * Describe your device here.
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdev.h"

#define INPT0  0x38
#define INPT1  0x39
#define INPT2  0x3A
#define INPT3  0x3B
#define INPT4  0x3C
#define INPT5  0x3D
#define SWCHA  0x280
#define SWACNT 0x281
#define SWCHB  0x282
#define SWBCNT 0x283
#define INTIM  0x284
#define INSTAT 0x285
#define TIM1T  0x294
#define TIM8T  0x295
#define TIM64T 0x296
#define T1024T 0x297
int timer = 0;

EXPORT char dev_type[] = "generic";

/* Fill in the name of the device */
EXPORT char dev_generic_name[] = "PIA 6532";

/* There are three types of sync:
 * - EXACT_SYNC: this type of sync will execute one device step for each
 * cpu step
 * - HORIZONTAL_SYNC: this will execute one device step for each line that is
 * shown on the monitor
 * - VERTICAL_SYNC: this will execute one device step for each frame that is
 *   shown on the screen */
EXPORT SYNC_TYPE dev_generic_sync_type = EXACT_SYNC;

char tmp[1000];

/* You must implement this function.
 *
 * This function initializes the device. */
EXPORT void dev_generic_reset()
{

}

/* Executes one step. Read the info on dev_generic_sync_type above to understand
 * how this function works. [cycles] is the number of cycles that must be 
 * executed, and it'll be 0 if dev_generic_sync_type is VERTICAL_SYNC. */
EXPORT void dev_generic_step(int cycles)
{
	
}

/* You must implement this function.
 *  
 * This function will be executed every time data is saved into a memory 
 * position that is inside the device memory area.
 *
 * Return value: 
 *   if 0 is returned, the memory will not be updated; 
 *   if -1 is returned, the memory will be updated. */
EXPORT int dev_generic_memory_set(long pos, unsigned char data)
{
	return -1;
}

/* The following functions (inside the DEBUG directive) are used only by the
 * debugger, and will be stripped off when the final version is created. */
#ifdef DEBUG

/* This variable will be used to return the debugger data in a persistent 
 * way. */
char info[100];

/* You must implement this function.
 *
 * This function will return the register names. For example, if your device
 * has two registers, X and Y, when n == 0, the function would return "X", and
 * when n == 1, it would return "Y". The funcion must return NULL for every 
 * other value of n. */
EXPORT char* dev_generic_debug_name(int n)
{
	switch(n)
	{
		case 0:	 return "INPT0";
		case 1:	 return "INPT1";
		case 2:	 return "INPT2";
		case 3:	 return "INPT3";
		case 4:	 return "INPT4";
		case 5:	 return "INPT5";
		case 6:	 return "SWCHA";
		case 7:	 return "SWCHB";
		case 8:	 return "SWACNT";
		case 9:	 return "SWBCNT";
		case 10: return "INTIM";
		case 11: return "INSTAT";
		case 12: return "TIM1T";
		case 13: return "TIM8T";
		case 14: return "TIM64T";
		case 15: return "T1024T";
		case 16: return "Timer";
		default: return NULL;
	}
}

/* You must implement this function.
 *
 * This function will return the register values. For example, if your device
 * has two registers, X and Y, when n == 0, the function would return the value 
 * in X, and when n == 1, it would return the value in Y. The value of the 
 * register n must match the register n passed on the function register_name. */
EXPORT char* dev_generic_debug(int n)
{
	switch(n)
	{
		case 0:  sprintf(info, "%d", dev_mem_get(INPT0));  break;
		case 1:  sprintf(info, "%d", dev_mem_get(INPT1));  break;
		case 2:  sprintf(info, "%d", dev_mem_get(INPT2));  break;
		case 3:  sprintf(info, "%d", dev_mem_get(INPT3));  break;
		case 4:  sprintf(info, "%d", dev_mem_get(INPT4));  break;
		case 5:  sprintf(info, "%d", dev_mem_get(INPT5));  break;
		case 6:  sprintf(info, "%d", dev_mem_get(SWCHA));  break;
		case 7:  sprintf(info, "%d", dev_mem_get(SWCHB));  break;
		case 8:  sprintf(info, "%d", dev_mem_get(SWACNT)); break;
		case 9:  sprintf(info, "%d", dev_mem_get(SWBCNT)); break;
		case 10: sprintf(info, "%d", dev_mem_get(INTIM));  break;
		case 11: sprintf(info, "%d", dev_mem_get(INSTAT)); break;
		case 12: sprintf(info, "%d", dev_mem_get(TIM1T));  break;
		case 13: sprintf(info, "%d", dev_mem_get(TIM8T));  break;
		case 14: sprintf(info, "%d", dev_mem_get(TIM64T)); break;
		case 15: sprintf(info, "%d", dev_mem_get(T1024T)); break;
		case 16: sprintf(info, "%d", timer);  break;
		default: return NULL;
	}
	return info;
}

#endif /* DEBUG */
