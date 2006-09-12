/*
 * Describe your video card here.
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdev.h"
#include "tia.h"

EXPORT char dev_type[] = "video";

/* Fill in the name of the device */
EXPORT char dev_video_name[] = "TIA1A";

/* These two variables set the number of horizontal and vertical pixels of
 * this video card. */
EXPORT int dev_video_pixels_x = 160;
EXPORT int dev_video_pixels_y = 192;

/* This variable sets the type of synchronization this device will have with
 * the rest of the computer. A setting of EXACT_SYNC means that dev_video_step
 * will be executed for each cpu step. HORIZONTAL_SYNC means that
 * dev_video_step will be execute every time a line scanline is completed on
 * the display. A setting of VERTICAL_SYNC means that dev_video step will only
 * be after a new frame is displayed. */
EXPORT SYNC_TYPE dev_video_sync_type = HORIZONTAL_SYNC;

/* The number of video cycles it takes to the device to draw a whole scanline
 * in the screen */
EXPORT int dev_video_scanline_cycles = 228;

/* The VBLANK is the number of scanlines before the image begins to be drawn
 * (this includes the VSYNC time too) and OVERSCAN is the number of scanlines
 * after the picture is drawn on the screen */
EXPORT int dev_video_scanlines_vblank = 40;
EXPORT int dev_video_scanlines_overscan = 30;

EXPORT int dev_video_pos_x = 0;
EXPORT int dev_video_pos_y = 0;
EXPORT int dev_video_wait_vsync = 0;
EXPORT int dev_video_wait_hsync = 0;

char tmp[1000];

/*
 * LOCAL VARIABLES
 */
static int m0_pos;
static int m0_size;

/*
 * AUXILIARY FUNCTIONS
 */

static inline int x() { return dev_video_pos_x - 68; }
static inline int y() { return dev_video_pos_y - 40; }

/*
 * STANDARD FUNCTIONS
 */

/* You must implement this function.
 *
 * This function initializes the device. */
EXPORT void dev_video_reset()
{
	int i;

	dev_video_create_palette(256);
	for(i=0; i<256; i++)
		dev_video_palette_set_color(i, 
				colortable[i] & 0xff,
				(colortable[i] / 0x100) & 0xff,
				(colortable[i] / 0x10000) & 0xff);	
	m0_pos = 80;
	m0_size = 1;
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
	switch(pos)
	{
		case VSYNC: /* Vertical sync */
			if(data & 0x2)
				dev_video_wait_vsync = -1;
			break;

		case WSYNC: /* Wait for sync */
			dev_video_wait_hsync = -1;
			break;

		case HMOVE: /* Horizontal movement of players and missiles */
			{
				int hmm0 = dev_mem_get(HMM0) >> 4;
				if(hmm0 >= 1 && hmm0 <= 7)
					m0_pos -= hmm0;
				else if(hmm0 >= 8 && hmm0 <= 15)
					m0_pos += (16 - hmm0);
				if (m0_pos >= 160)
					m0_pos = 0;
				else if (m0_pos < 0)
					m0_pos = 159;
			}
			break;
			/* todo - other players and missiles */

		case NUSIZ0: /* Player and missile 0 size */
			switch((data & 0x30) >> 4)
			{
				case 0: m0_size = 1; break;
				case 1: m0_size = 2; break;
				case 2: m0_size = 4; break;
				case 4: m0_size = 8; break;
			}
			break;
	}
	return -1;
}

/* Executes one step. Read the info on dev_video_sync_type above to understand
 * how this function works. [cycles] is the number of cycles that must be 
 * executed, and it'll be 0 if dev_video_sync_type is VERTICAL_SYNC. */
EXPORT void dev_video_step(int cycles)
{
	if(y() < 0 || y() > dev_video_pixels_y)
		return;

	/* draw background */
	dev_video_draw_hline(0, 159, y(), dev_mem_get(COLUBK));

	/* draw missile */
	if(dev_mem_get(ENAM0) & 0x2)
		dev_video_draw_hline(m0_pos, m0_pos+m0_size, y(), dev_mem_get(COLUP0));
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
		case 0:	return "X";
		case 1: return "Y";
		case 2: return "m0_size";
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
		case 0:
			sprintf(info, "%d", x());
			break;
		case 1:
			sprintf(info, "%d", y());
			break;
		case 2:
			sprintf(info, "%d", m0_size);
			break;
		default:
			return NULL;
	}
	return info;
}

#endif /* DEBUG */
