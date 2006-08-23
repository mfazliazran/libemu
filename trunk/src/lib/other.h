#ifndef _OTHER_H_
#define _OTHER_H_

#include <gtk/gtk.h>

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

/* other functions */
void generic_update();

#endif
