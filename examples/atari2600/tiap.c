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
EXPORT SYNC_TYPE dev_video_sync_type = EXACT_SYNC;

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
 * PROTOTYPES
 */
EXPORT void dev_video_step(int cycles);

/*
 * LOCAL VARIABLES
 */

typedef enum
{
	PLAYFIELD,
	PLAYER_0,
	PLAYER_1,
	NUM_TYPES
} TYPE;

/* synchronization */
static int vsync, vblank;

/* background */
static int bg_color;

/* playfield */
static int pf[20];
static int pf_color;
static int pf_score;
static int pf_priority;
static int pf_reflect;

/* players */
static int p_color[2];
static int p_pos[2];
static int p_mov[2];
static int p_size[2];
static int p_reflect[2];
static int p_grp[2];
static int p_graphic[2][72];

/* collision */
static unsigned char pixel[20];

/*
 * INLINE FUNCTIONS
 */
inline int x() { return (dev_video_pos_x - 68); }
inline int y() { return (dev_video_pos_y - 40); }
inline int x_right(int x) { 
	if(x > 160)
		return 160;
	else
		return x;
}
inline int x_left(int x) { 
	if(x < 0)
		return 0;
	else
		return x;
}

/* You must implement this function.
 *
 * This function initializes the device. */
EXPORT void dev_video_reset()
{
	int i, j;

	dev_video_create_palette(256);
	for(i=0; i<256; i++)
		dev_video_palette_set_color(i, 
				colortable[i] & 0xff,
				(colortable[i] / 0x100) & 0xff,
				(colortable[i] / 0x10000) & 0xff);	

	/* set joystick as unpressed */
	dev_mem_set_direct(INPT4, 0x80);

	/* initialize variables */
	vsync = vblank = 0;
	bg_color = 0;
	pf_color = pf_score = pf_priority = pf_reflect = 0;
	for(i=0; i<20; i++)
		pf[i] = 0;
	for(i=0; i<2; i++)
	{
		p_color[i] = 0;
		p_pos[i] = 80;
		p_mov[i] = 0;
		p_grp[i] = 0;
		p_size[i] = 0;
		p_reflect[i] = 0;
		for(j=0; j<72; j++)
			p_graphic[i][j] = 0;
	}
}

inline void redraw_player(int p)
{
	int i, j;

	for(i=0; i<72; i++)
		p_graphic[p][i] = 0;

	switch(p_size[p])
	{
		case 0x0:
			for(i=0; i<8; i++)
				p_graphic[p][!p_reflect[p]? 7-i: i] = ((p_grp[p] & (1 << i)) != 0);
			break;
		case 0x1:
			for(i=0; i<8; i++)
			{
				p_graphic[p][!p_reflect[p]? 7-i: i] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)+16] = ((p_grp[p] & (1 << i)) != 0);
			}
			break;
		case 0x2:
			for(i=0; i<8; i++)
			{
				p_graphic[p][!p_reflect[p]? 7-i: i] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)+32] = ((p_grp[p] & (1 << i)) != 0);
			}
			break;
		case 0x3:
			for(i=0; i<8; i++)
			{
				p_graphic[p][!p_reflect[p]? 7-i: i] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)+16] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)+32] = ((p_grp[p] & (1 << i)) != 0);
			}
			break;
		case 0x4:
			for(i=0; i<8; i++)
			{
				p_graphic[p][!p_reflect[p]? 7-i: i] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)+64] = ((p_grp[p] & (1 << i)) != 0);
			}
			break;
		case 0x5:
			for(i=0; i<8; i++)
			{
				p_graphic[p][(!p_reflect[p]? 7-i: i)*2] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)*2+1] = ((p_grp[p] & (1 << i)) != 0);
			}
			break;
		case 0x6:
			for(i=0; i<8; i++)
			{
				p_graphic[p][!p_reflect[p]? 7-i: i] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)+32] = ((p_grp[p] & (1 << i)) != 0);
				p_graphic[p][(!p_reflect[p]? 7-i: i)+64] = ((p_grp[p] & (1 << i)) != 0);
			}
			break;
		case 0x7:
			for(i=0; i<8; i++)
				for(j=0; j<4; j++)
					p_graphic[p][(!p_reflect[p]? 7-i: i)*4+j] = ((p_grp[p] & (1 << i)) != 0);
			break;
	}
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
	int i;

	switch(pos)
	{
		/*
		 * SYNCRONIZATION
		 */
		case VSYNC: /* Vertical Sync */
			vsync = ((data & 0x2) == 0x2);
			if(vsync)
				dev_video_pos_y = 0;
			break;

		case VBLANK: /* Vertical Blank */
			vblank = ((data & 0x2) == 0x2);
			break;

		case WSYNC: /* Wait for horizontal sync */
			dev_video_step(228); /* go to the end of line */
			dev_video_wait_hsync = -1;
			break;

		/*
		 * BACKGROUND
		 */
		case COLUBK: /* Background color */
			bg_color = data;
			break;

		/*
		 * PLAYFIELD
		 */
		case COLUPF: /* Playfield color */
			pf_color = data;
			break;

		case CTRLPF: /* Control playfield */
			pf_reflect = data & 0x1;
			pf_score = data & 0x2;
			pf_priority = data & 0x4;
			/* todo - ball size */
			break;

		case PF0: /* Playfield graphics 0 */
			for(i=4; i<8; i++)
				pf[i-4] = (data & (1 << i)) >> i;
			break;

		case PF1: /* Playfield graphics 1 */
			for(i=0; i<8; i++)
				pf[i+4] = (data & (0x80 >> i)) != 0;
			break;

		case PF2: /* Playfield graphics 2 */
			for(i=0; i<8; i++)
				pf[i+12] = (data & (1 << i)) >> i;
			break;

		/*
		 * PLAYER APPEARENCE
		 */
		case COLUP0:
			p_color[0] = data;
			break;

		case COLUP1:
			p_color[1] = data;
			break;

		case GRP0:
			p_grp[0] = data;
			redraw_player(0);
			break;

		case GRP1:
			p_grp[1] = data;
			redraw_player(1);
			break;

		case NUSIZ0:
			p_size[0] = data & 0x7;
			redraw_player(0);
			break;

		case NUSIZ1:
			p_size[1] = data & 0x7;
			redraw_player(1);
			break;

		case REFP0:
			p_reflect[0] = (data & 0x8) != 0;
			redraw_player(0);
			break;

		case REFP1:
			p_reflect[1] = (data & 0x8) != 0;
			redraw_player(1);
			break;

		/* 
		 * PLAYER POSITION
		 */
		case RESP0: /* Reset player 0 */
			/* where did I got these numbers from??? */
			if(x() < 0)
				p_pos[0] = 3;
			else
				p_pos[0] = x_left(x() + 14) + 3;
			break;

		case RESP1: /* Reset player 1 */
			if(x() < 0)
				p_pos[1] = 3;
			else
				p_pos[1] = x_left(x() + 14) + 3;
			break;

		case HMP0: /* Horizontal movement of player 1 */
			{
				int hmp0 = data >> 4;
				if(hmp0 >= 1 && hmp0 <= 7)
					p_mov[0] = -hmp0;
				else if(hmp0 >= 8 && hmp0 <= 15)
					p_mov[0] = (16 - hmp0);
				else
					p_mov[0] = 0;
			}
			break;

		case HMP1: /* Horizontal movement of player 1 */
			{
				int hmp0 = data >> 4;
				if(hmp0 >= 1 && hmp0 <= 7)
					p_mov[1] = -hmp0;
				else if(hmp0 >= 8 && hmp0 <= 15)
					p_mov[1] = (16 - hmp0);
				else
					p_mov[1] = 0;
			}
			break;

		case HMOVE: /* request horizontal movement of players,
			       missiles and ball */
			for(i=0; i<2; i++)
			{
				p_pos[i] += p_mov[i];
				if (p_pos[i] >= 160)
					p_pos[i] = 0;
				else if (p_pos[i] < 0)
					p_pos[i] = 159;
			}
			break;

		case HMCLR: /* horizontal movement clear */
			for(i=0; i<2; i++)
			{
				p_mov[i] = 0;
			}
			break;
	}
	return 0;
}

inline void drln(int x1, int x2, int color, TYPE type)
{
	dev_video_draw_hline(x1, x2, y(), color);
	/* todo - treat collision */
}

inline void draw_pf(int x1, int x2)
{
	inline int xs_right(int x)  { if(x < x1) return x1; else return x; }
	inline int xs_left(int x) { if(x > x2) return x2; else return x; }

	int i;
	
	// left side of the screen (optimize!)
	for(i=0; i<20; i++)
		if(pf[i])
			if((i)*4 >= x1-4 && (i+1)*4 <= x2+4)
				drln(
						xs_left(i*4), 
						xs_right(i*4+4), 
						pf_score? p_color[0]: pf_color,
						PLAYFIELD);
	// right side of the screen (optimize!)
	for(i=0; i<20; i++)
		if(pf[pf_reflect? 19-i : i])
			if((i+20)*4 >= x1-4 && (i+21)*4 <= x2+4)
				drln(
						xs_left((i+20)*4),
						xs_right((i+20)*4+4), 
						pf_score? p_color[1]: pf_color,
						PLAYFIELD);
}

inline void draw_player(int p, int x1, int x2)
{
	inline int xs_right(int x)  { if(x < x1) return x1; else return x; }
	inline int xs_left(int x) { if(x > x2) return x2; else return x; }

	int i;
	
	// draw player
	for(i=0; i<72; i++)
		if(p_graphic[p][i])
			if((i + p_pos[p]) % 160 >= x1 && (i + p_pos[p]) % 160 <= x2)
				drln(
						xs_left((i + p_pos[p]) % 160), 
						xs_right((i + p_pos[p]) % 160 + 1), 
						p_color[p],
						p == 0? PLAYER_0: PLAYER_1);
}

/* Executes one step. Read the info on dev_video_sync_type above to understand
 * how this function works. [cycles] is the number of cycles that must be 
 * executed, and it'll be 0 if dev_video_sync_type is VERTICAL_SYNC. */
EXPORT void dev_video_step(int cycles)
{
	/* exit if not in the frame */
	if(y() < 0 || y() > 191 || x()+cycles < 0)
		return;

	/* draw black if vblank is on */
	if(vblank)
	{
		dev_video_draw_hline(x_left(x()), x_right(x()+cycles), y(), 0);
		return;
	}

	/* draw background */
	dev_video_draw_hline(x_left(x()), x_right(x()+cycles), y(), bg_color);

	/* draw playfield if priority low */
	if(!pf_priority)
		draw_pf(x_left(x()), x_right(x()+cycles));

	

	/* draw players */
	draw_player(0, x_left(x()), x_right(x()+cycles));
	draw_player(1, x_left(x()), x_right(x()+cycles));

	/* draw playfield if priority low */
	if(pf_priority)
		draw_pf(x_left(x()), x_right(x()+cycles));
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
		case  0: return "x";
		case  1: return "y";
		case  2: return "VSYNC";
		case  3: return "VBLANK";
		case  4: return "COLUBK";
		case  5: return "PF0";
		case  6: return "PF1";
		case  7: return "PF2";
		case  8: return "COLUPF";
		case  9: return "PF Reflect";
		case 10: return "PF Score";
		case 11: return "PF Priority";
		case 12: return "COLUP0";
		case 13: return "NUSIZ0";
		case 14: return "REFP0";
		case 15: return "GRP0";
		case 16: return "P Pos 0";
		case 17: return "P Mov 0";
		case 18: return "P Enabl 0";
		case 19: return "COLUP1";
		case 20: return "NUSIZ1";
		case 21: return "REFP1";
		case 22: return "GRP1";
		case 23: return "P Pos 1";
		case 24: return "P Mov 1";
		case 25: return "P Enabl 1";
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
	int i;

	switch(n)
	{
		case 0:
			sprintf(info, "%d", x());
			break;
		case 1:
			sprintf(info, "%d", y());
			break;
		case 2:
			sprintf(info, "%d", vsync);
			break;
		case 3:
			sprintf(info, "%d", vblank);
			break;
		case 4:
			sprintf(info, "0x%02x", bg_color);
			break;
		case 5:
			for(i=0; i<4; i++)
				info[i] = pf[i] + '0';
			info[i] = '\0';
			break;
		case 6:
			for(i=0; i<8; i++)
				info[i] = pf[i+4] + '0';
			info[i] = '\0';
			break;
		case 7:
			for(i=0; i<8; i++)
				info[i] = pf[i+12] + '0';
			info[i] = '\0';
			break;
		case 8:
			sprintf(info, "0x%02x", pf_color);
			break;
		case 9:
			sprintf(info, "%d", pf_reflect);
			break;
		case 10:
			sprintf(info, "%d", pf_score);
			break;
		case 11:
			sprintf(info, "%d", pf_priority);
			break;
		case 12:
			sprintf(info, "0x%02x", p_color[0]);
			break;
		case 13:
			sprintf(info, "%d", p_size[0]);
			break;
		case 14:
			sprintf(info, "%d", p_reflect[0]);
			break;
		case 15:
			sprintf(info, "0x%02x", p_grp[0]);
			break;
		case 16:
			sprintf(info, "%d", p_pos[0]);
			break;
		case 17:
			sprintf(info, "%d", p_mov[0]);
			break;
		case 18:
			sprintf(info, "%d", p_grp[0] != 0);
			break;
		case 19:
			sprintf(info, "0x%02x", p_color[1]);
			break;
		case 20:
			sprintf(info, "%d", p_size[1]);
			break;
		case 21:
			sprintf(info, "%d", p_reflect[1]);
			break;
		case 22:
			sprintf(info, "0x%02x", p_grp[1]);
			break;
		case 23:
			sprintf(info, "%d", p_pos[1]);
			break;
		case 24:
			sprintf(info, "%d", p_mov[1]);
			break;
		case 25:
			sprintf(info, "%d", p_grp[1] != 0);
			break;
		default:
			return NULL;
	}
	return info;
}

#endif /* DEBUG */
