EXTRA_DIST = 6502.c tia.c tia.h pia.c
DIST_SUBDIRS = rom

AM_CPPFLAGS = -I../../src/lib `pkg-config --cflags gtk+-2.0`

bin_PROGRAMS = atari2600
atari2600_SOURCES = atari2600.c
atari2600_LDADD = -lemu -L../../src/lib
atari2600_LDFLAGS = `pkg-config --libs gtk+-2.0`
