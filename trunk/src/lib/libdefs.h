#ifndef LIBDEFS_H
#define LIBDEFS_H

typedef enum {
	EXACT_SYNC = 0,
	HORIZONTAL_SYNC,
	VERTICAL_SYNC
} SYNC_TYPE;

typedef enum {
	PRESSED = 0,
	RELEASED
} KEYEVENT_TYPE;

typedef enum {
	UP = 0, DOWN, LEFT, RIGHT,
	B1, B2, B3, B4, B5, B6, B7, B8, B9, B0,
	S1, S2, S3, S4, S5
} JOYBUTTON;

#endif
