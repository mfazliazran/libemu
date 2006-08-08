/*
 * Describe your video card here.
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "extern.c"

char tmp[1000];

/* You must implement this function.
 *
 * This function initializes the video card. */
EXPORT void init_video()
{

}

/* You must implement this function.
 *  
 * This function will be executed every time data is saved into a memory 
 * position that is inside the video memory area.
 *
 * Return value: 
 *   if  0 is returned, the memory will be updated; 
 *   if -1 is returned, the memory will not be updated. */
EXPORT int video_memory_set(long pos, unsigned char data)
{
	return 0;
}

/* TODO */
/* Walk [cycles] steps on video. Each CPU cycle = 3 video cycles */
EXPORT void video_step(int cycles)
{

}

/* The following functions (inside the DEBUG directive) are used only by the
 * debugger, and will be stripped off when the final version is created. */
#ifdef DEBUG

/* This variable will be used to return the debugger data in a persistent 
 * way. */
char info[100];

/* You must implement this function.
 *
 * This function will return the register names. For example, if your video card
 * has two registers, X and Y, when n == 0, the function would return "X", and
 * when n == 1, it would return "Y". The funcion must return NULL for every 
 * other value of n. */
EXPORT char* video_debug_name(int n)
{
	switch(n)
	{
		/*
		case 0:	return "X";
		case 1: return "Y";
		*/
		default: return NULL;
	}
}

/* You must implement this function.
 *
 * This function will return the register values. For example, if your video card
 * has two registers, X and Y, when n == 0, the function would return the value 
 * in X, and when n == 1, it would return the value in Y. The value of the 
 * register n must match the register n passed on the function register_name. */
EXPORT char* video_debug(int n)
{
	switch(n)
	{
		/*
		case 0:
			sprintf(info, "%d", X);
			break;
		case 1:
			sprintf(info, "%d", Y);
			break;
		*/
		default:
			return NULL;
	}
	return info;
}

#endif /* DEBUG */
