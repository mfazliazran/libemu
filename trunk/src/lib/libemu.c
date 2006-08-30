#include <gtk/gtk.h>
#include "libemu.h"
#include "other.h"

void emu_init(const char* name, int argc, char** argv)
{
	GtkWidget *vbox; 
	GtkWidget *machine_menu, *machine_item, *run_item, *debug_item;
	
	has_cpu = has_video = has_ram = 0;
	generic_count = 1;
	
	gtk_init(&argc, &argv);
	
	/* Main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), name);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(gtk_main_quit), NULL);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* Menu */
	menu = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);
	
	machine_menu = gtk_menu_new();
	machine_item = gtk_menu_item_new_with_label("Machine");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(machine_item), machine_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu), machine_item);
	
	debug_menu = gtk_menu_new();
	debug_item = gtk_menu_item_new_with_label("Debug");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(debug_item), debug_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu), debug_item);

	run_item = gtk_menu_item_new_with_label("Run");
	gtk_menu_shell_append(GTK_MENU_SHELL(machine_menu), run_item);
	gtk_widget_set_sensitive(run_item, FALSE);

	/* Screen */
	screen = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(vbox), screen, TRUE, TRUE, 0);
	
	/* Status Bar */
	statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);
}

void emu_main()
{
	gtk_widget_show_all(window);

	/* Create screen */
	buffer = gdk_pixmap_new(screen->window, *emu_video_pixels_x, *emu_video_pixels_y, -1);
	gtk_widget_set_size_request(screen, *emu_video_pixels_x, *emu_video_pixels_y);
	gc = gdk_gc_new(GDK_DRAWABLE(buffer));
	gdk_draw_rectangle(buffer, screen->style->black_gc, TRUE, 0, 0,
			*emu_video_pixels_x, *emu_video_pixels_y);

	gtk_main();
}

void emu_message(char* message)
{
	g_message(message);
}

void emu_error(char* message)
{
	g_error(message);
}
