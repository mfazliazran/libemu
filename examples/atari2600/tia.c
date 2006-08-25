/*
 * Describe your video card here.
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdev.h"

EXPORT char dev_type[] = "video";

/* Fill in the name of the device */
EXPORT char dev_video_name[] = "TIA1A";

/* Change this variable to -1 to update the video image to the user. The
 * variable will be automaticly set to 0. */
EXPORT int dev_video_draw_frame = 0;

/* This variable sets the type of synchronization this device will have with
 * the rest of the computer. A setting of EXACT_SYNC means that dev_video_step
 * will be executed for each cpu step. HORIZONTAL_SYNC means that
 * dev_video_step will be execute every time a line scanline is completed on
 * the display. A setting of VERTICAL_SYNC means that dev_video step will only
 * be after a new frame is displayed. */
EXPORT SYNC_TYPE dev_video_sync_type = HORIZONTAL_SYNC;

/* This variable sets the number of cycles spent to draw a scanline. It only
 * needs to be set if dev_video_sync_type is EXACT_SYNC or HORIZONTAL_SYNC. */
EXPORT int dev_video_scanline_cycles = 228;

/* These two variables set how many scanlines are drawn before, during and after
 * the frame is drawn on the screen, respectively. These need to be set if
 * dev_video_sync_type is EXACT_SYNC or HORIZONTAL_SYNC. */
EXPORT int dev_video_vblank_scanlines   = 40;
EXPORT int dev_video_picture_scanlines  = 192;
EXPORT int dev_video_overscan_scanlines = 30;

char tmp[1000];

/* You must implement this function.
 *
 * This function initializes the device. */
EXPORT void dev_video_reset()
{

}

/* You must implement this function.
 *  
 * This function will be executed every time data is saved into a memory 
 * position that is inside the device memory area.
 *
 * Return value: 
 *   if  0 is returned, the memory will not be updated; 
 *   if -1 is returned, the memory will be updated. */
EXPORT int dev_video_memory_set(long pos, unsigned char data)
{
	return -1;
}

/* Executes one step. Read the info on dev_video_sync_type above to understand
 * how this function works. [cycles] is the number of cycles that must be 
 * executed, and it'll be 0 if dev_video_sync_type is VERTICAL_SYNC. */
EXPORT void dev_video_step(int cycles)
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
 * This function will return the register names. For example, if your device
 * has two registers, X and Y, when n == 0, the function would return "X", and
 * when n == 1, it would return "Y". The funcion must return NULL for every 
 * other value of n. */
EXPORT char* dev_video_debug_name(int n)
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
 * This function will return the register values. For example, if your device
 * has two registers, X and Y, when n == 0, the function would return the value 
 * in X, and when n == 1, it would return the value in Y. The value of the 
 * register n must match the register n passed on the function register_name. */
EXPORT char* dev_video_debug(int n)
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
