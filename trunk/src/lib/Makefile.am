lib_LTLIBRARIES = libemu.la

libemu_la_SOURCES = libemu.c cpu.c ram.c rom.c other.c other.h
libemu_la_LIBADD = $(DEPS_LIBS)
libemu_la_LDFLAGS = -no-undefined
AM_CPPFLAGS = $(DEPS_CFLAGS) -DGLADE_DIR=\""$(datadir)/glade"\" @MMS_BITFIELDS@

include_HEADERS = libemu.h libdev.h

libemu_la_LDFLAGS = -version-info 0:0:0
