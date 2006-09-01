#include <gtk/gtk.h>
#include "SDL.h"
#include "libemu.h"
#include "other.h"

/*
 * EVENT HANDLERS
 */
void run_clicked(GtkButton* cpu_run_pause, gpointer data)
{
	run();
}

static gboolean close_clicked (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	SDL_Quit();
	gtk_main_quit();
	return FALSE;
}

/*
 * API
 */
void emu_init(const char* name, int argc, char** argv)
{
	GtkWidget *vbox, *hbox, *controls;
	GtkWidget *run, *pause, *memory;
	
	debug_menu = gtk_menu_new(); // off

	has_cpu = has_video = has_ram = 0;
	generic_count = 1;
	
	gtk_init(&argc, &argv);
	
	/* Main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), name);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(window), 12);
	g_signal_connect(G_OBJECT(window), "delete_event",
			 G_CALLBACK(close_clicked), NULL);

	vbox = gtk_vbox_new(FALSE, 6);

	/* Emulator controls */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Emulator Main Controls"), FALSE, FALSE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), gtk_hseparator_new());
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	controls = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(controls), GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(controls), 6);
	run   = button_with_stock_image("Run", "gtk-media-play", TRUE);
	pause = button_with_stock_image("Pause", "gtk-media-pause", TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pause), TRUE);
	gtk_box_pack_start_defaults(GTK_BOX(controls), run);
	gtk_box_pack_start_defaults(GTK_BOX(controls), pause);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), controls);

	/* Internal devices */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Internal Devices"), FALSE, FALSE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), gtk_hseparator_new());
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	internal_hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(internal_hbox), GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(internal_hbox), 6);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), internal_hbox);

	/* External devices */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("External (I/O) Devices"), FALSE, FALSE, 0);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), gtk_hseparator_new());
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	external_hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(external_hbox), GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(external_hbox), 6);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), external_hbox);

	gtk_container_add(GTK_CONTAINER(window), vbox);
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
