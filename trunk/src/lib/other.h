#ifndef _OTHER_H_
#define _OTHER_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "libemu.h"

typedef enum {
	EXACT_SYNC = 0,
	HORIZONTAL_SYNC,
	VERTICAL_SYNC
} SYNC_TYPE;

int connect_callbacks(GModule* mod);
GtkWidget* button_with_stock_image(gchar* mnemonic, gchar* stock);
unsigned long hex2long(char* hex);

GtkWidget* window;
GtkWidget* menu;
GtkWidget* debug_menu;
GtkWidget* screen;
GtkWidget* statusbar;
static gboolean running = FALSE;

int has_cpu, has_video, has_ram, generic_count;

SYNC_TYPE emu_generic_sync[MAX_GENERIC];
double emu_generic_cycles[MAX_GENERIC];
SYNC_TYPE emu_video_sync;
double emu_video_cycles;

GdkPixmap *buffer;
GdkGC *gc;

/* other functions */
void generic_update();
void video_update();

#endif
