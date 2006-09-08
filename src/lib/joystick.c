#include "libemu.h"
#include "other.h"

#define jc joystick_count

GtkWindow *joystick_window[MAX_JOYSTICK];

/* When the joystick menu item is clicked on the main window */
static void joystick_show_hide(GtkToggleButton *item, gpointer data)
{
	GtkWindow* window = GTK_WINDOW(data);
	if(gtk_toggle_button_get_active(item))
	{
		gtk_window_present(window);
		generic_update();
	}
	else
		gtk_widget_hide(GTK_WIDGET(data));
}

/* When the close button is clicked on the joystick */
static gboolean joystick_hide(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), FALSE);
	gtk_widget_hide(widget);
	return TRUE;
}

/* Initialize a new joystick */
int emu_joystick_init(void (*callback)(KEYEVENT_TYPE event_type, JOYBUTTON joybutton))
{
	GtkWidget *debug_item, *table;

	if(jc >= MAX_JOYSTICK)
	{
		g_error("There is a limit of %d joysticks.", jc);
		return 0;
	}

	/* Add a new menu option */
	debug_item = button_with_pixmap_image(g_strdup_printf("Joystick %d", jc), P_JOYSTICK, TRUE);
	gtk_box_pack_start_defaults(GTK_BOX(external_hbox), debug_item);

	/* Create window */
	joystick_window[jc] = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(joystick_window[jc]), g_strdup_printf("Joystick %d", jc));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(joystick_window[jc]), TRUE);
	g_signal_connect(debug_item, "toggled", G_CALLBACK(joystick_show_hide), joystick_window[jc]);
	g_signal_connect(joystick_window[jc], "delete_event", G_CALLBACK(joystick_hide), debug_item);

	/* Create table */
	table = gtk_table_new(1, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 12);

	gtk_container_add(GTK_CONTAINER(joystick_window[jc]), table);
	gtk_widget_show_all(table);

	jc++;
	return jc-1;
}
