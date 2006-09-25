#include "libemu.h"
#include "other.h"

#define jc joystick_count

typedef struct
{
	int joynumber;
	JOYBUTTON joybutton;
	void* callback;
} DATA;

/*
 * EVENT HANDLERS
 */

/* When a joystick button is clicked */
static void joystick_button_clicked(GtkWidget *widget, gpointer data)
{
	DATA *dt = data;

	void (*callback)(KEYEVENT_TYPE event_type, int joynumber, JOYBUTTON joybutton);

	callback = dt->callback;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		callback(PRESSED, dt->joynumber, dt->joybutton);
	else
		callback(RELEASED, dt->joynumber, dt->joybutton);
}

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

/*
 * PRIVATE FUNCTIONS
 */

static void add_button(GtkWidget *table, const char *label, int define, int x, int y, int joynumber, void *callback)
{
	GtkWidget *button;
	DATA *dt = g_malloc(sizeof(DATA));
	
	dt->joynumber = joynumber;
	dt->joybutton = define;
	dt->callback = callback;

	button = gtk_toggle_button_new_with_label(label);
	g_signal_connect(button, "toggled", G_CALLBACK(joystick_button_clicked), (gpointer)dt);
	gtk_table_attach_defaults(GTK_TABLE(table), button, x, x+1, y, y+1);

	joy_button[joynumber][define] = button;
}

/*
 * API CALLS
 */

/* Initialize a new joystick */
int emu_joystick_init(void (*callback)(KEYEVENT_TYPE event_type, int joynumber, JOYBUTTON joybutton))
{
	GtkWidget *joystick_window, *debug_item, *table, *button;

	if(jc >= MAX_JOYSTICK)
	{
		g_error("There is a limit of %d joysticks.", jc);
		return 0;
	}

	/* Add a new menu option */
	debug_item = button_with_pixmap_image(g_strdup_printf("Joystick %d", jc), P_JOYSTICK, TRUE);
	gtk_box_pack_start_defaults(GTK_BOX(external_hbox), debug_item);

	/* Create window */
	joystick_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(joystick_window), g_strdup_printf("Joystick %d", jc));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(joystick_window), TRUE);
	g_signal_connect(debug_item, "toggled", G_CALLBACK(joystick_show_hide), joystick_window);
	g_signal_connect(joystick_window, "delete_event", G_CALLBACK(joystick_hide), debug_item);

	/* Create table */
	table = gtk_table_new(4, 9, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 12);

	/* Create buttons */
	add_button(table, "UP", UP, 1, 1, jc, callback);
	add_button(table, "LE", LEFT, 0, 2, jc, callback);
	add_button(table, "RI", RIGHT, 2, 2, jc, callback);
	add_button(table, "DO", DOWN, 1, 3, jc, callback);
	add_button(table, "S0", S0, 4, 0, jc, callback);
	add_button(table, "S1", S1, 5, 0, jc, callback);
	add_button(table, "S2", S2, 6, 0, jc, callback);
	add_button(table, "S3", S3, 7, 0, jc, callback);
	add_button(table, "S4", S4, 8, 0, jc, callback);
	add_button(table, "B0", B0, 4, 2, jc, callback);
	add_button(table, "B1", B1, 5, 2, jc, callback);
	add_button(table, "B2", B2, 6, 2, jc, callback);
	add_button(table, "B3", B3, 7, 2, jc, callback);
	add_button(table, "B4", B4, 8, 2, jc, callback);
	add_button(table, "B5", B5, 4, 3, jc, callback);
	add_button(table, "B6", B6, 5, 3, jc, callback);
	add_button(table, "B7", B7, 6, 3, jc, callback);
	add_button(table, "B8", B8, 7, 3, jc, callback);
	add_button(table, "B9", B9, 8, 3, jc, callback);

	gtk_container_add(GTK_CONTAINER(joystick_window), table);
	gtk_widget_show_all(table);

	jc++;
	return jc-1;
}
