#include <gtk/gtk.h>
#include "libemu.h"
#include "other.h"

void run_clicked(GtkButton* cpu_run_pause, gpointer data)
{
	run();
}

void emu_init(const char* name, int argc, char** argv)
{
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
	vbox_main = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox_main);

	/* Menu */
	menu = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox_main), menu, FALSE, FALSE, 0);
	
	machine_menu = gtk_menu_new();
	machine_item = gtk_menu_item_new_with_label("Machine");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(machine_item), machine_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu), machine_item);
	
	debug_menu = gtk_menu_new();
	debug_item = gtk_menu_item_new_with_label("Debug");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(debug_item), debug_menu);
	gtk_menu_bar_append(GTK_MENU_BAR(menu), debug_item);

	run_item = gtk_check_menu_item_new_with_label("Run");
	gtk_menu_shell_append(GTK_MENU_SHELL(machine_menu), run_item);
	// gtk_widget_set_sensitive(run_item, FALSE);
	g_signal_connect(run_item, "toggled", G_CALLBACK(run_clicked), NULL);
}

void emu_main()
{
	gtk_widget_show_all(window);
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
