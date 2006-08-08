SUBDIRS = templates
DIST_SUBDIRS = templates

bin_SCRIPTS = emutouch emucc

bin_PROGRAMS = emucheck
emucheck_SOURCES = emucheck.c
emucheck_LDADD = $(DEPS_LIBS)
AM_CPPFLAGS = $(DEPS_CFLAGS) -DPIXMAPS_DIR=\""$(datadir)/pixmaps"\" @MMS_BITFIELDS@
