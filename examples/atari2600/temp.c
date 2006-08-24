/*
 * Describe your device here.
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdev.h"

EXPORT char dev_type[] = "generic";

/* Fill in the name of the device */
EXPORT char dev_generic_name[] = "Temporary Device";

char tmp[1000];

/* You must implement this function.
 *
 * This function initializes the device. */
EXPORT void dev_generic_reset()
{

}

/* You must implement this function.
 *  
 * This function will be executed every time data is saved into a memory 
 * position that is inside the device memory area.
 *
 * Return value: 
 *   if  0 is returned, the memory will be updated; 
 *   if -1 is returned, the memory will not be updated. */
EXPORT int dev_generic_memory_set(long pos, unsigned char data)
{
	return 0;
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
		case 0:	return "X";
		case 1: return "Y";
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
		case 0:
			sprintf(info, "%d", 1);
			break;
		case 1:
			sprintf(info, "%d", 2);
			break;
		default:
			return NULL;
	}
	return info;
}

#endif /* DEBUG */