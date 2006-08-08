EXTRA_DIST = x21.c

AM_CPPFLAGS = -I../../src/lib `pkg-config --cflags gtk+-2.0`

bin_PROGRAMS = libemu
libemu_SOURCES = libemu.c
libemu_LDADD = -lemu -L../../src/lib
libemu_LDFLAGS = `pkg-config --libs gtk+-2.0`
