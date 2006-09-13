#ifndef _OTHER_H_
#define _OTHER_H_

#include <gtk/gtk.h>
#include "libemu.h"
#include "libdefs.h"

/* Pixbuf images */
#define P_CPU      1
#define P_VIDEO    2
#define P_DEVICE   3
#define P_MONITOR  4
#define P_JOYSTICK 5
#define P_MEMORY   6

int connect_callbacks(GModule* mod);
GtkWidget* button_with_stock_image(gchar* mnemonic, gchar* stock, gboolean toggle);
GtkWidget* button_with_pixmap_image(gchar* mnemonic, gint image_number, gboolean toggle);
unsigned long hex2long(char* hex);

GtkWidget *window;
GtkWidget *controls, *internal_hbox, *external_hbox;
GtkWidget *debug_menu;
GtkWidget *statusbar;
gboolean running;
GtkWidget *run_b, *pause;
GtkWidget *usage_bar;
GtkWidget *joy_button[MAX_JOYSTICK][NUM_JOY_BUTTONS];
guint run_signal, pause_signal;

int has_cpu, has_video, has_ram, generic_count, joystick_count;

SYNC_TYPE emu_generic_sync[MAX_GENERIC];
double emu_generic_cycles[MAX_GENERIC];
SYNC_TYPE emu_video_sync;
double emu_video_cycles;

/* other functions */
void generic_update();
void video_update();

#endif
