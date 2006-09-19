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

/* Player */
static unsigned char p0_color, p1_color;/* Player color */
static int p0_enabled, p1_enabled;	/* If the player shoud be drawn */
static int p0_pos, p1_pos;		/* Player horizontal position */
static int p0_size, p1_size;		/* Player size (1..4) */
static int p0_mov, p1_mov;		/* Player horizontal speed (-7..7) */
static int p0_graphics[8], p1_graphics[8]; /* Player graphic, in bits */
static int p0_inverted, p1_inverted;	/* If the player is drawn inverted */

/* Missile */
static int m0_enabled, m1_enabled; 	/* see the player comments above */
static int m0_pos, m1_pos;
static int m0_size, m1_size;
static int m0_mov, m1_mov;

/* Playfield */
static unsigned char pf_color;
static int pf_reflect; 	/* if the playfiled will be reflected in the right
			   side of the screen */
static int pf_score;	/* if set, the left pf is the color of P0, and the
			   left if the color of P1 */
static int pf_top;	/* playfield is drawn of the top of P0, M0, P1 and M1 */
static int pf_graphics[20]; /* graphics of the playfield */

/* Background */
static unsigned char bg_color;

/* collision detection stuff */
typedef enum { P0=0, P1, M0, M1, BL, TOTAL_SPRITES, PF } SPRITE;
static int collision[160][TOTAL_SPRITES];

/*
 * AUXILIARY FUNCTIONS
 */

int x() { return (dev_video_pos_x - 68); }
int y() { return (dev_video_pos_y - 40); }

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

	p0_color = 0;    p1_color = 0;
	p0_pos = 80;     p1_pos = 80;
	p0_size = 1;     p1_size = 1;
	p0_enabled = 0;  p1_enabled = 0;
	p0_mov = 0;      p1_mov = 0;
	p0_inverted = 0; p1_inverted = 0;
	for(i=0; i<8; i++)
		p0_graphics[i] = p1_graphics[i] = 0;

	m0_pos = 80;    m1_pos = 80;
	m0_size = 1;    m1_size = 1;
	m0_enabled = 0; m1_enabled = 0;
	m0_mov = 0;     m1_mov = 0;

	pf_reflect = pf_score = pf_top = 0;

	bg_color = 0;

	/* set joystick as unpressed */
	dev_mem_set_direct(INPT4, 0x80);
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
	int i, j;
	int hmp0, hmp1, hmm0, hmm1;

	switch(pos)
	{
		/*
		 * Screen operations
		 */
		case VSYNC: /* Vertical sync */
			if(data & 0x2)
				dev_video_wait_vsync = -1;
			break;

		case WSYNC: /* Wait for sync */
			dev_video_wait_hsync = -1;
			break;

		case COLUBK: /* background color */
			bg_color = data;
			break;

		/*
		 * Player & Missile format
		 */
		case COLUP0: /* player 0 color */
			p0_color = data;
			break;

		case COLUP1: /* player 1 color */
			p1_color = data;
			break;

		case ENAM0: /* Enable missile 0 */
			m0_enabled = (data & 0x2);
			break;

		case ENAM1: /* Enable missile 1 */
			m1_enabled = (data & 0x2);
			break;

		case NUSIZ0: /* Player and missile 0 size */
			switch((data & 0x30) >> 4)
			{
				case 0: m0_size = 1; break;
				case 1: m0_size = 2; break;
				case 2: m0_size = 4; break;
				case 4: m0_size = 8; break;
			}
			break;

		case NUSIZ1: /* Player and missile 1 size */
			switch((data & 0x30) >> 4)
			{
				case 0: m1_size = 1; break;
				case 1: m1_size = 2; break;
				case 2: m1_size = 4; break;
				case 4: m1_size = 8; break;
			}
			break;

		case REFP0: /* invert image (reflect) player 0 */
			p0_inverted = (data & 0x8);
			break;

		case REFP1: /* invert image (reflect) player 1 */
			p1_inverted = (data & 0x8);
			break;

		case GRP0: /* Player 0 graphics */
			j = 0;
			for(i=0; i<8; i++)
			{
				p0_graphics[i] = ((data & (1 << i)) != 0);
				j += p0_graphics[i];
			}
			p0_enabled = (j>0);
			break;

		case GRP1: /* Player 1 graphics */
			j = 0;
			for(i=0; i<8; i++)
			{
				p1_graphics[i] = ((data & (1 << i)) != 0);
				j += p1_graphics[i];
			}
			p1_enabled = (j>0);
			break;

		/*
		 * Player & Missile movement
		 */
		case HMOVE: /* request horizontal movement of players,
			       missiles and ball */
			m0_pos += m0_mov;
			m1_pos += m1_mov;
			p0_pos += p0_mov;
			p1_pos += p1_mov;

			/* verify if they're falling out of the screen */
			if (m0_pos >= 160)
				m0_pos = 0;
			else if (m0_pos < 0)
				m0_pos = 159;
			if (m1_pos >= 160)
				m1_pos = 0;
			else if (m1_pos < 0)
				m1_pos = 159;
			if (p0_pos >= 160)
				p0_pos = 0;
			else if (p0_pos < 0)
				p0_pos = 159;
			if (p1_pos >= 160)
				p1_pos = 0;
			else if (p1_pos < 0)
				p1_pos = 159;
			break;

		case HMP0: /* horizontal movement of player 0 */
			hmp0 = data >> 4;
			if(hmp0 >= 1 && hmp0 <= 7)
				p0_mov = -hmp0;
			else if(hmp0 >= 8 && hmp0 <= 15)
				p0_mov = (16 - hmp0);
			else
				p0_mov = 0;
			break;

		case HMP1: /* horizontal movement of player 1 */
			hmp1 = data >> 4;
			if(hmp1 >= 1 && hmp1 <= 7)
				p1_mov = -hmp1;
			else if(hmp1 >= 8 && hmp1 <= 15)
				p1_mov = (16 - hmp1);
			else
				p1_mov = 0;
			break;

		case HMM0: /* horizontal movement of missile 0 */
			hmm0 = data >> 4;
			if(hmm0 >= 1 && hmm0 <= 7)
				m0_mov = -hmm0;
			else if(hmm0 >= 8 && hmm0 <= 15)
				m0_mov = (16 - hmm0);
			else
				m0_mov = 0;
			break;

		case HMM1: /* horizontal movement of missile 1 */
			hmm1 = data >> 4;
			if(hmm1 >= 1 && hmm1 <= 7)
				m1_mov = -hmm1;
			else if(hmm1 >= 8 && hmm1 <= 15)
				m1_mov = (16 - hmm1);
			else
				m1_mov = 0;
			break;

		case HMCLR: /* horizontal movement clear */
			p0_mov = 0;
			p1_mov = 0;
			m0_mov = 0;
			m1_mov = 0; 
			/* TODO - bl_mov = 0; */
			break;

		case RESP0: /* reset player 0 */
			p0_pos = dev_video_pos_x;
			break;

		case RESP1: /* reset player 1 */
			p1_pos = dev_video_pos_x;
			break;

		case RESM0: /* reset missile 0 */
			m0_pos = dev_video_pos_x;
			break;

		case RESM1: /* reset missile 1 */
			m1_pos = dev_video_pos_x;
			break;

		/*
		 * Playfield registers
		 */
		case COLUPF:
			pf_color = data;
			break;

		case CTRLPF:
			pf_reflect = (data & 0x1);
			pf_score = (data & 0x2);
			pf_top = (data & 0x4);
			/* TODO - ball size */
			break;

		case PF0:
			for(i=4; i<8; i++)
				pf_graphics[i-4] = ((data & (1 << i)) != 0);
			break;

		case PF1:
			for(i=0; i<8; i++)
				pf_graphics[i+4] = ((data & (0x80 >> i)) != 0);
			break;

		case PF2:
			for(i=0; i<8; i++)
				pf_graphics[i+12] = ((data & (1 << i)) != 0);
			break;

		/*
		 * Collision detection
		 */
		case CXCLR:
			dev_mem_set_direct(CXM0P, 0x0);
			dev_mem_set_direct(CXM1P, 0x0);
			dev_mem_set_direct(CXP0FB, 0x0);
			dev_mem_set_direct(CXP1FB, 0x0);
			dev_mem_set_direct(CXM0FB, 0x0);
			dev_mem_set_direct(CXM1FB, 0x0);
			dev_mem_set_direct(CXBLPF, 0x0);
			dev_mem_set_direct(CXPPMM, 0x0);
			break;
	}
	return 0;
}

inline void drln(SPRITE sprite, int x, int x2, int color)
{
	int i;
	for(i=x; i<x2; i++)
		collision[i][sprite] = 1;
	dev_video_draw_hline(x, x2, y(), color);
}

inline void draw_playfield()
{
	int i;
	for(i=0; i<20; i++)
		if(pf_graphics[i])
			drln(PF, i*4, i*4+4, pf_score ? p0_color : pf_color);
	for(i=20; i<39; i++)
		if(pf_graphics[pf_reflect ? 20 - (i-20) : (i-20)])
			drln(PF, i*4, i*4+4, pf_score ? p1_color : pf_color);
}

/* Executes one step. Read the info on dev_video_sync_type above to understand
 * how this function works. [cycles] is the number of cycles that must be 
 * executed, and it'll be 0 if dev_video_sync_type is VERTICAL_SYNC. */
EXPORT void dev_video_step(int cycles)
{
	int i, j;

	if(y() < 0 || y() > dev_video_pixels_y)
		return;

	/* clear collision detection */
	for(i=0; i<160; i++)
		for(j=0; j<TOTAL_SPRITES; j++)
			collision[i][j] = 0;

	/* draw background */
	dev_video_draw_hline(0, 159, y(), bg_color);

	/* draw playfield */
	if(!pf_top)
		draw_playfield();

	/* draw missile & player 1 */
	if(m1_enabled)
		drln(M1, m1_pos, m1_pos+m1_size, p1_color);
	if(p1_enabled)
	{
		int i, x = p1_pos;
		for(i=0; i<8; i++)
		{
			if(p1_graphics[p1_inverted ? i : 7-i])
				drln(P1, x, x+p1_size, p1_color);
			x += p1_size;
		}
	}

	/* draw missile & player 0 */
	if(m0_enabled)
		drln(M0, m0_pos, m0_pos+m0_size, p0_color);
	if(p0_enabled)
	{
		int i, x = p0_pos;
		for(i=0; i<8; i++)
		{
			if(p0_graphics[p0_inverted ? i : 7-i])
				drln(P0, x, x+p0_size, p0_color);
			x += p0_size;
		}
	}

	/* draw playfield */
	if(pf_top)
		draw_playfield();

	/* check collisions */
	for(i=0; i<160; i++)
	{
		if(collision[i][M0] && collision[i][P1])
			dev_mem_set_direct(CXM0P, dev_mem_get(CXM0P) | 0x80);
		if(collision[i][M0] && collision[i][P0])
			dev_mem_set_direct(CXM0P, dev_mem_get(CXM0P) | 0x40);
		if(collision[i][M1] && collision[i][P0])
			dev_mem_set_direct(CXM1P, dev_mem_get(CXM1P) | 0x80);
		if(collision[i][M1] && collision[i][P1])
			dev_mem_set_direct(CXM1P, dev_mem_get(CXM1P) | 0x40);

		if(collision[i][P0] && collision[i][P1])
			dev_mem_set_direct(CXPPMM, dev_mem_get(CXPPMM) | 0x80);
		if(collision[i][M0] && collision[i][M1])
			dev_mem_set_direct(CXPPMM, dev_mem_get(CXPPMM) | 0x40);
	}
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
 * has two registers, x() and y(), when n == 0, the function would return "x()", and
 * when n == 1, it would return "y()". The funcion must return NULL for every 
 * other value of n. */
EXPORT char* dev_video_debug_name(int n)
{
	switch(n)
	{
		case 0:	return "x()";
		case 1: return "y()";
		case 2: return "INPT4";
		case 3: return "BG Color";
		case 4: return "m1_enabled";
		case 5: return "p1_color";
		default: return NULL;
	}
}

/* You must implement this function.
 *
 * This function will return the register values. For example, if your device
 * has two registers, x() and y(), when n == 0, the function would return the value 
 * in x(), and when n == 1, it would return the value in y(). The value of the 
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
			sprintf(info, "0x%02x", dev_mem_get(INPT4));
			break;
		case 3:
			sprintf(info, "0x%02x", bg_color);
			break;
		case 4:
			sprintf(info, "0x%02x", m1_enabled);
			break;
		case 5:
			sprintf(info, "0x%02x", p1_color);
			break;
		default:
			return NULL;
	}
	return info;
}

#endif /* DEBUG */
