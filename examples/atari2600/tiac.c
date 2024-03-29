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

/* These two variables set the number of horizontal and vertical cols of
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
	PLAYFIELD = 0x1,
	PLAYER_0  = 0x2,
	PLAYER_1  = 0x4,
	MISSILE_0 = 0x8,
	MISSILE_1 = 0x10,
	BALL      = 0x20,
	NUM_TYPES = 6
} TYPE;

typedef struct
{
	unsigned long pos;
	unsigned char data;
	int cycles;
} DATASET;
DATASET dataset, dataset2;

/* synchronization */
static int vsync, vblank;

/* playfield */
static int pf[20];
static int pf_enabled;
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
static int p_enabled[2];
static unsigned char p_grp[2];
static unsigned char p_grpa[2]; // used for 6-digit trick
static unsigned char p_graph[2];
static int p_delay[2];

/* missiles */
static int m_pos[2];
static int m_mov[2];
static int m_size[2];
static int m_enabled[2];
static int m_lock[2];

/* ball */
static int b_pos;
static int b_mov;
static int b_size;
static int b_enabled;
static int b_graphic[8];

/* collision detection */
typedef struct
{
	int bg;
	int p[2];
	int m[2];
	int pf;
	int bl;
	unsigned char px;
} PIXEL;
PIXEL line[160];

/*
 * INLINE FUNCTIONS
 */
inline int x() { return (dev_video_pos_x - 68); }
inline int y() { return (dev_video_pos_y - 40); }

/* You must implement this function.
 *
 * This function initializes the device. */
EXPORT void dev_video_reset()
{
	int i;

	dev_video_create_palette(256);
	for(i=0; i<256; i++)
		dev_video_palette_set_color(i, 
				(colortable[i] >> 16) & 0xff,
				(colortable[i] >> 8) & 0xff,
				colortable[i] & 0xff);

	/* set joystick as unpressed */
	dev_mem_set_direct(INPT4, 0x80);

	/* clear collisions */
	dev_mem_set_direct(CXM0P, 0x0);
	dev_mem_set_direct(CXM1P, 0x0);
	dev_mem_set_direct(CXP0FB, 0x0);
	dev_mem_set_direct(CXP1FB, 0x0);
	dev_mem_set_direct(CXM0FB, 0x0);
	dev_mem_set_direct(CXM1FB, 0x0);
	dev_mem_set_direct(CXBLPF, 0x0);
	dev_mem_set_direct(CXPPMM, 0x0);

	/* initialize variables */
	vsync = vblank = 0;
	pf_color = pf_score = pf_priority = pf_reflect = 0;
	for(i=0; i<20; i++)
		pf[i] = 0;
	for(i=0; i<2; i++)
	{
		p_color[i] = 0;
		p_pos[i] = 80;
		p_mov[i] = 0;
		p_grp[i] = 0;
		p_grpa[i] = 0;
		p_graph[i] = 0;
		p_delay[i] = 0;
		p_size[i] = 0;
		p_reflect[i] = 0;
		p_enabled[i] = 0;
		m_pos[i] = 80;
		m_mov[i] = 0;
		m_size[i] = 0;
		m_enabled[i] = 0;
		m_lock[i] = 0;
	}
	b_pos = 80;
	b_mov = 0;
	b_size = 1;
	b_enabled = 0;
	b_graphic[0] = 1;
	for(i=0; i<160; i++)
	{
		line[i].bg   = -1;
		line[i].pf   = -1;
		line[i].bl   = -1;
		line[i].m[0] = -1;
		line[i].m[1] = -1;
		line[i].p[0] = -1;
		line[i].p[1] = -1;
		line[i].px   =  0;
	}
}

inline void redraw_player(int p)
{
	/* NUSIZ?
	 * COLUP?
	 * REFP?
	 * GRP? */
	
	unsigned char grp = (p_delay[p] == 0 ? p_grp[p] : p_grpa[p]);

	inline void draw_player(int pos)
	{
		int init, step, i, j;
		init = p_reflect[p] ? 0 : 7;
		step = p_reflect[p] ? 1 : -1;
		for(i=pos, j=init; i<pos+8; i++, j+=step)
			if((grp >> j & 1) == 1)
				line[i%160].p[p] = p_color[p];
	}

	int i;
	for(i=0; i<160; i++)
		line[i].p[p] = -1;

	if(grp == 0)
		return;
	
	switch(p_size[p])
	{
		case 0x0:
			draw_player(p_pos[p]);
			break;
		case 0x1:
			draw_player(p_pos[p]);
			draw_player(p_pos[p]+16);
			break;
		case 0x2:
			draw_player(p_pos[p]);
			draw_player(p_pos[p]+32);
			break;
		case 0x3:
			draw_player(p_pos[p]);
			draw_player(p_pos[p]+16);
			draw_player(p_pos[p]+32);
			break;
		case 0x4:
			draw_player(p_pos[p]);
			draw_player(p_pos[p]+64);
			break;
		case 0x5:
			/* TODO */
			break;
		case 0x6:
			draw_player(p_pos[p]);
			draw_player(p_pos[p]+32);
			draw_player(p_pos[p]+64);
			break;
		case 0x7:
			/* TODO */
			break;
	}
}

inline void redraw_missile(int p)
{
	int i;
	for(i=0; i<160; i++)
		line[i].m[p] = -1;
	if(m_enabled[p])
		switch(m_size[p])
		{
			case 3: line[(m_pos[p]+4)%160].m[p] = p_color[p];
				line[(m_pos[p]+5)%160].m[p] = p_color[p];
				line[(m_pos[p]+6)%160].m[p] = p_color[p];
				line[(m_pos[p]+7)%160].m[p] = p_color[p];
			case 2: line[(m_pos[p]+2)%160].m[p] = p_color[p];
				line[(m_pos[p]+3)%160].m[p] = p_color[p];
			case 1: line[(m_pos[p]+1)%160].m[p] = p_color[p];
			case 0: line[m_pos[p]%160].m[p]     = p_color[p];
		}
}

inline void redraw_pf()
{
	int i;
	int pf1_color = pf_priority ? pf_color : (pf_score ? p_color[0] : pf_color);
	int pf2_color = pf_priority ? pf_color : (pf_score? p_color[1]: pf_color);

	/* draw playfield */
	for(i=0; i<20; i++)
	{
		if(pf[i])
			line[(i*4)].pf = line[(i*4)+1].pf = line[(i*4)+2].pf = line[(i*4)+3].pf = pf1_color;
		else
			line[(i*4)].pf = line[(i*4)+1].pf = line[(i*4)+2].pf = line[(i*4)+3].pf = -1;
		if(pf[pf_reflect ? 19-i : i])
			line[((i+20)*4)].pf = line[((i+20)*4)+1].pf = line[((i+20)*4)+2].pf = line[((i+20)*4)+3].pf = pf2_color;
		else
			line[((i+20)*4)].pf = line[((i+20)*4)+1].pf = line[((i+20)*4)+2].pf = line[((i+20)*4)+3].pf = -1;
	}

	/* draw ball */
	unsigned char b_color = pf_priority ? pf_color : (pf_score ? p_color[1] : pf_color);
	for(i=0; i<160; i++)
		line[i].bl = -1;
	if(b_enabled)
		switch(b_size)
		{
			case 3: line[(b_pos+4)%160].bl = b_color;
				line[(b_pos+5)%160].bl = b_color;
				line[(b_pos+6)%160].bl = b_color;
				line[(b_pos+7)%160].bl = b_color;
			case 2: line[(b_pos+2)%160].bl = b_color;
				line[(b_pos+3)%160].bl = b_color;
			case 1: line[(b_pos+1)%160].bl = b_color;
			case 0: line[b_pos%160].bl   = b_color;
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
EXPORT int dev_video_memory_set(long pos, unsigned char data, int cycles)
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
		 * PLAYER POSITION
		 */
		case RESP0: /* Reset player 0 */
			p_pos[0] = x() + cycles + 5;
			if(p_pos[0] < 3)
				p_pos[0] = 3;
			if(p_pos[0] > 160)
				p_pos[0] = 160;
			redraw_player(0);
			break;

		case RESP1: /* Reset player 1 */
			p_pos[1] = x() + cycles + 5;
			if(p_pos[1] < 3)
				p_pos[1] = 3;
			if(p_pos[1] > 160)
				p_pos[1] = 160;
			redraw_player(1);
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

		/* 
		 * MISSILE POSITION
		 */
		case RESM0: /* Reset missile 0 */
			m_pos[0] = x() + cycles + 5;
			if(m_pos[0] < 2)
				m_pos[0] = 2;
			if(m_pos[0] > 160)
				m_pos[0] = 160;
			redraw_missile(0);
			break;

		case RESM1: /* Reset missile 1 */
			m_pos[1] = x() + cycles + 5;
			if(m_pos[1] < 2)
				m_pos[1] = 2;
			if(m_pos[1] < 160)
				m_pos[1] = 160;
			redraw_missile(1);
			break;

		case RESMP0: /* Reset missile 0 to player 0 */
			m_lock[0] = (dataset.data & 0x2) != 0;
			redraw_missile(0);
			break;

		case RESMP1: /* Reset missile 1 to player 1 */
			m_lock[1] = (dataset.data & 0x2) != 0;
			redraw_missile(1);
			break;

		case HMM0: /* Horizontal movement of missile 0 */
			{
				int hmm0 = data >> 4;
				if(hmm0 >= 1 && hmm0 <= 7)
					m_mov[0] = -hmm0;
				else if(hmm0 >= 8 && hmm0 <= 15)
					m_mov[0] = (16 - hmm0);
				else
					m_mov[0] = 0;
			}
			break;

		case HMM1: /* Horizontal movement of missile 1 */
			{
				int hmm0 = data >> 4;
				if(hmm0 >= 1 && hmm0 <= 7)
					m_mov[1] = -hmm0;
				else if(hmm0 >= 8 && hmm0 <= 15)
					m_mov[1] = (16 - hmm0);
				else
					m_mov[1] = 0;
			}
			break;

		/* 
		 * BALL POSITION
		 */
		case RESBL: /* Reset ball */
			b_pos = x() + cycles + 4;
			if(b_pos < 2)
				b_pos = 2;
			if(b_pos > 160)
				b_pos = 160;
			redraw_pf();
			break;

		case HMBL: /* Horizontal movement of ball */
			{
				int hmbl = data >> 4;
				if(hmbl >= 1 && hmbl <= 7)
					b_mov = -hmbl;
				else if(hmbl >= 8 && hmbl <= 15)
					b_mov = (16 - hmbl);
				else
					b_mov = 0;
			}
			break;

		/*
		 * SPRITES MOVEMENT
		 */
		case HMOVE: /* request horizontal movement of players,
			       missiles and ball */
			for(i=0; i<2; i++)
			{
				p_pos[i] += p_mov[i];
				m_pos[i] += m_mov[i];
				if (p_pos[i] >= 160)
					p_pos[i] = 0;
				else if (p_pos[i] < 0)
					p_pos[i] = 159;
				if(m_lock[i])
				{
					switch(p_size[i])
					{
						case 0x5: /* double size */
							m_pos[i] = p_pos[i] + 6;
							break;
						case 0x7: /* quad size */
							m_pos[i] = p_pos[i] + 10;
							break;
						default: /* regular size */
							m_pos[i] = p_pos[i] + 3;
					}
					break;
				}
				else
				{
					if (m_pos[i] >= 160)
						m_pos[i] = 0;
					else if (m_pos[i] < 0)
						m_pos[i] = 159;
				}
			}
			b_pos += b_mov;
			if (b_pos >= 160)
				b_pos = 0;
			else if (b_pos < 0)
				b_pos = 159;
			if(p_mov[0])
				redraw_player(0);
			if(p_mov[1])
				redraw_player(1);
			if(m_mov[0])
				redraw_missile(0);
			if(m_mov[1])
				redraw_missile(1);
			if(b_mov)
				redraw_pf();
			break;

		case HMCLR: /* horizontal movement clear */
			for(i=0; i<2; i++)
			{
				p_mov[i] = 0;
				m_mov[i] = 0;
			}
			b_mov = 0;
			break;

		/*
		 * COLLISIONS
		 */
		case CXCLR: /* clear collisions */
			dev_mem_set_direct(CXM0P, 0x0);
			dev_mem_set_direct(CXM1P, 0x0);
			dev_mem_set_direct(CXP0FB, 0x0);
			dev_mem_set_direct(CXP1FB, 0x0);
			dev_mem_set_direct(CXM0FB, 0x0);
			dev_mem_set_direct(CXM1FB, 0x0);
			dev_mem_set_direct(CXBLPF, 0x0);
			dev_mem_set_direct(CXPPMM, 0x0);
			break;

		default:
			dataset2.pos = pos;
			dataset2.data = data;
			dataset2.cycles = cycles;
	}
	return 0;
}

/* This function set the variable one cycles *after* it was set */
inline void set_registers()
{
	inline void check_playfield() { 
		int j;
		for(j=0; j<20; j++)
			if(pf[j] != 0)
			{
				pf_enabled = 1;
				return;
			}
		pf_enabled = 0;
	}

	int i;

	switch(dataset.pos)
	{
		/*
		 * BACKGROUND
		 */
		case COLUBK: /* Background color */
			for(i=0; i<160; i++)
				line[i].bg = dataset.data;
			break;

		/*
		 * PLAYFIELD
		 */
		case COLUPF: /* Playfield color */
			pf_color = dataset.data;
			redraw_pf();
			break;

		case CTRLPF: /* Control playfield */
			pf_reflect = dataset.data & 0x1;
			pf_score = dataset.data & 0x2;
			pf_priority = dataset.data & 0x4;
			b_size = (dataset.data & 0x30) >> 4;
			redraw_pf();
			break;

		case PF0: /* Playfield graphics 0 */
			for(i=4; i<8; i++)
				pf[i-4] = (dataset.data & (1 << i)) >> i;
			check_playfield();
			redraw_pf();
			break;

		case PF1: /* Playfield graphics 1 */
			for(i=0; i<8; i++)
				pf[i+4] = (dataset.data & (0x80 >> i)) != 0;
			check_playfield();
			redraw_pf();
			break;

		case PF2: /* Playfield graphics 2 */
			for(i=0; i<8; i++)
				pf[i+12] = (dataset.data & (1 << i)) >> i;
			check_playfield();
			redraw_pf();
			break;

		/*
		 * PLAYER APPEARENCE
		 */
		case COLUP0:
			p_color[0] = dataset.data;
			if(p_enabled[0])
				redraw_player(0);
			if(m_enabled[0])
				redraw_missile(0);
			break;

		case COLUP1:
			p_color[1] = dataset.data;
			if(p_enabled[1])
				redraw_player(1);
			if(m_enabled[1])
				redraw_missile(1);
			break;

		case GRP0:
			p_grpa[0] = p_grp[0];
			p_grp[0] = dataset.data;
			p_grpa[1] = p_grp[1];
			redraw_player(0);
			if(p_delay[1])
				redraw_player(1);
			break;

		case GRP1:
			p_grpa[1] = p_grp[1];
			p_grp[1] = dataset.data;
			p_grpa[0] = p_grp[0];
			redraw_player(1);
			if(p_delay[0])
				redraw_player(0);
			break;

		case NUSIZ0:
			p_size[0] = dataset.data & 0x7;
			redraw_player(0);
			m_size[0] = ((dataset.data & 0x30) >> 4);
			redraw_missile(0);
			break;

		case NUSIZ1:
			p_size[1] = dataset.data & 0x7;
			redraw_player(1);
			m_size[1] = ((dataset.data & 0x30) >> 4);
			redraw_missile(1);
			break;

		case REFP0:
			p_reflect[0] = (dataset.data & 0x8) != 0;
			redraw_player(0);
			break;

		case REFP1:
			p_reflect[1] = (dataset.data & 0x8) != 0;
			redraw_player(1);
			break;

		/*
		 * MISSILE & BALL APPEARENCE
		 */
		case ENAM0:
			m_enabled[0] = (dataset.data & 0x2) != 0;
			redraw_missile(0);
			break;

		case ENAM1:
			m_enabled[1] = (dataset.data & 0x2) != 0;
			redraw_missile(1);
			break;

		case ENABL:
			b_enabled = (dataset.data & 0x2) != 0;
			redraw_pf();
			break;

		/*
		 * VERTICAL DELAY
		 */
		case VDELP0:
			p_delay[0] = (dataset.data & 0x1);
			redraw_player(0);
			redraw_player(1);
			break;

		case VDELP1:
			p_delay[1] = (dataset.data & 0x1);
			redraw_player(0);
			redraw_player(1);
			break;

	}
	dataset.pos = dataset2.pos;
	dataset.data = dataset2.data;
	dataset.cycles = dataset2.cycles;
}


/* check for collisions */
inline void collisions(int x1, int x2)
{
	int i;
	for(i=x1; i<x2; i++)
		if(i >= 0 && i < 160)
		{
			if(line[i].m[0] != -1 && line[i].p[1] != -1)
				dev_mem_set_direct(CXM0P, dev_mem_get(CXM0P) | 0x80);
			if(line[i].m[0] != -1 && line[i].p[0] != -1)
				dev_mem_set_direct(CXM0P, dev_mem_get(CXM0P) | 0x40);
			if(line[i].m[1] != -1 && line[i].p[0] != -1)
				dev_mem_set_direct(CXM1P, dev_mem_get(CXM1P) | 0x80);
			if(line[i].m[1] != -1 && line[i].p[1] != -1)
				dev_mem_set_direct(CXM1P, dev_mem_get(CXM1P) | 0x40);
			if(line[i].p[0] != -1 && line[i].pf != -1)
				dev_mem_set_direct(CXP0FB, dev_mem_get(CXP0FB) | 0x80);
			if(line[i].p[0] != -1 && line[i].bl != -1)
				dev_mem_set_direct(CXP0FB, dev_mem_get(CXP0FB) | 0x40);
			if(line[i].p[1] != -1 && line[i].pf != -1)
				dev_mem_set_direct(CXP1FB, dev_mem_get(CXP1FB) | 0x80);
			if(line[i].p[1] != -1 && line[i].bl != -1)
				dev_mem_set_direct(CXP1FB, dev_mem_get(CXP1FB) | 0x40);
			if(line[i].m[0] != -1 && line[i].pf != -1)
				dev_mem_set_direct(CXM0FB, dev_mem_get(CXM0FB) | 0x80);
			if(line[i].m[0] != -1 && line[i].bl != -1)
				dev_mem_set_direct(CXM0FB, dev_mem_get(CXM0FB) | 0x40);
			if(line[i].m[1] != -1 && line[i].pf != -1)
				dev_mem_set_direct(CXM1FB, dev_mem_get(CXM1FB) | 0x80);
			if(line[i].m[1] != -1 && line[i].bl != -1)
				dev_mem_set_direct(CXM1FB, dev_mem_get(CXM1FB) | 0x40);
			if(line[i].bl != -1 && line[i].pf != -1)
				dev_mem_set_direct(CXBLPF, dev_mem_get(CXBLPF) | 0x80);
			if(line[i].p[0] != -1 && line[i].p[1] != -1)
				dev_mem_set_direct(CXPPMM, dev_mem_get(CXPPMM) | 0x80);
			if(line[i].m[0] && line[i].m[1])
				dev_mem_set_direct(CXPPMM, dev_mem_get(CXPPMM) | 0x40);
		}
}

/* Executes one step. Read the info on dev_video_sync_type above to understand
 * how this function works. [cycles] is the number of cycles that must be 
 * executed, and it'll be 0 if dev_video_sync_type is VERTICAL_SYNC. */
EXPORT void dev_video_step(int cycles)
{
	/* set the registers data */
	set_registers();

	/* exit if not in the frame */
	if(y() < 0 || y() > 191 || x()+cycles < 0)
		return;

	int i, x1, x2;
	if(x() < 0)
		x1 = 0;
	else
		x1 = x();
	if(x()+cycles > 160)
		x2 = 160;
	else
		x2 = x() + cycles;
	for(i=x1; i<x2; i++)
		if(i >= 0 && i < 160)
		{
			line[i].px = line[i].bg;
			if(!pf_priority)
			{
				if(line[i].pf != -1)
					line[i].px = line[i].pf;
				if(line[i].bl != -1)
					line[i].px = line[i].bl;
			}

			
			if(line[i].m[1] != -1)
				line[i].px = line[i].m[1];
			if(line[i].p[1] != -1)
				line[i].px = line[i].p[1];

			if(line[i].m[0] != -1)
				line[i].px = line[i].m[0];
			if(line[i].p[0] != -1)
				line[i].px = line[i].p[0];

			if(pf_priority)
			{
				if(line[i].pf != -1)
					line[i].px = line[i].pf;
				if(line[i].bl != -1)
					line[i].px = line[i].bl;
			}
		}

	/* check collisions */
	collisions(x1, x2);

}

EXPORT void dev_video_scanline(int cycles)
{
	int i;
	int cc, cx, cl;
	cx = 0;
	cl = 0;
	cc = -1;
	if(y()>=0 && y() < 192)
	{
		for(i=0; i<160; i++)
		{
			if(line[i].px != cc)
			{
				dev_video_draw_hline(cx, cx+cl, y(), cc);
				cx = i;
				cc = line[i].px;
			}
			cl++;
		}
		if(cx < 159)
			dev_video_draw_hline(cx, 160, y(), cc);
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

		case 26: return "CXM0P";
		case 27: return "CXM1P";
		case 28: return "CXP0FB";
		case 29: return "CXP1FB";
		case 30: return "CXM0FB";
		case 31: return "CXM1FB";
		case 32: return "CXBLPF";
		case 33: return "CXPPMM";
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
			sprintf(info, "N/A");
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
		case 26:
			sprintf(info, "0x%02x", dev_mem_get(CXM0P));
			break;
		case 27:
			sprintf(info, "0x%02x", dev_mem_get(CXM1P));
			break;
		case 28:
			sprintf(info, "0x%02x", dev_mem_get(CXP0FB));
			break;
		case 29:
			sprintf(info, "0x%02x", dev_mem_get(CXP1FB));
			break;
		case 30:
			sprintf(info, "0x%02x", dev_mem_get(CXM0FB));
			break;
		case 31:
			sprintf(info, "0x%02x", dev_mem_get(CXM1FB));
			break;
		case 32:
			sprintf(info, "0x%02x", dev_mem_get(CXBLPF));
			break;
		case 33:
			sprintf(info, "0x%02x", dev_mem_get(CXPPMM));
			break;
		default:
			return NULL;
	}
	return info;
}

#endif /* DEBUG */
