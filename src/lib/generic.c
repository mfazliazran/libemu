#include <string.h>
#include "libemu.h"
#include "other.h"

#define MAX_REGISTERS 255
#define MAX_VERTICAL 12

static GtkWidget* generic_window[MAX_GENERIC];
static GtkWidget* generic_register[MAX_GENERIC][MAX_REGISTERS];
static gint num_registers[MAX_GENERIC];

/*
 * EVENT HANDLERS
 */

/* When the CPU menu item is clicked on the main window */
static void generic_show_hide(GtkCheckMenuItem *item, gpointer data)
{
	GtkWindow* window = GTK_WINDOW(data);
	if(item->active)
	{
		gtk_window_present(window);
		generic_update();
	}
	else
		gtk_widget_hide(GTK_WIDGET(data));
}

/* When the close button is clicked on the debugger */
static gboolean generic_hide(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data), FALSE);
	gtk_widget_hide(widget);
	return TRUE;
}

/*
 * PUBLIC FUNCTIONS
 */

/* Update the debugger on the screen */
void generic_update()
{
	int i, n;
	for(n=0; n<generic_count; n++)
		if(GTK_WIDGET_VISIBLE(generic_window[n]))
			for(i=0; i<num_registers[n]; i++)
				gtk_entry_set_text(GTK_ENTRY(generic_register[n][i]), emu_generic_debug[n](i));
}

/*
 * API
 */

/* Create a new generic device, and return its number */
int emu_generic_init(char* filename)
{
	GModule *generic_mod;
	gchar *path, *type;
	GtkWidget *debug_item;
	GtkWidget *table, *label;
	gint row, col, i;

	if(generic_count >= MAX_GENERIC)
	{
		g_error("There is a limit of %d devices.", generic_count);
		return 0;
	}

	if(emu_mem_size() == 0)
	{
		emu_error("The memory size wasn't set yet!");
		return 0;
	}

	/* create path (g_module_open requires a full path) */
	if(filename[0] == '.' || filename[0] == '/' || filename[0] == '~')
		path = g_strdup_printf("%s", filename);
	else
		path = g_strdup_printf("./%s", filename);

	/* open module */
	generic_mod = g_module_open(path, G_MODULE_BIND_LAZY);
	if(!generic_mod)
	{
		g_error("%s: invalid Generic Device file (%s)", path, g_module_error());
		return 0;
	}

	/* check if it's really a CPU */
	if(!g_module_symbol(generic_mod, "dev_type", (gpointer*)&type))
		g_error("variable dev_type not defined in %s", path);
	if(strcmp(type, "generic"))
		g_error("%s not a Generic Device file.", path);

	/* Connect functions */
	if(!g_module_symbol(generic_mod, "dev_generic_name", (gpointer*)&emu_generic_name[generic_count]))
		g_error("variable dev_generic_name not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_reset", (void*)&emu_generic_reset[generic_count]))
		g_error("variable dev_generic_reset not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_memory_set", (void*)&emu_generic_memory_set[generic_count]))
		g_error("variable dev_generic_memory_set not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_debug_name", (void*)&emu_generic_debug_name[generic_count]))
		g_error("variable dev_generic_debug_name not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_debug", (void*)&emu_generic_debug[generic_count]))
		g_error("variable dev_generic_debug not defined in %s", path);

	/* Connect callbacks */
	if(!connect_callbacks(generic_mod))
		g_error("Generic Device callbacks couldn't be connected in %s", path);

	g_message("Generic Device %s loaded from %s", emu_generic_name[generic_count], path);
	
	/* Add a new menu option */
	debug_item = gtk_check_menu_item_new_with_label(g_strdup_printf("%s (generic)", emu_generic_name[generic_count]));
	gtk_menu_shell_append(GTK_MENU_SHELL(debug_menu), debug_item);

	/* Create window */
	generic_window[generic_count] = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(generic_window[generic_count]), emu_generic_name[generic_count]);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(generic_window[generic_count]), TRUE);
	g_signal_connect(debug_item, "toggled", G_CALLBACK(generic_show_hide), generic_window[generic_count]);
	g_signal_connect(generic_window[generic_count], "delete_event", G_CALLBACK(generic_hide), debug_item);

	/* Create table */
	table = gtk_table_new(1, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	/* Add registers to table */
	row = 0; col = 0; i = 0 ;
	while(emu_generic_debug_name[generic_count](i))
	{
		gtk_table_attach(GTK_TABLE(table),
				gtk_label_new(emu_generic_debug_name[generic_count](i)),
				col, col+1, row, row+1,
				GTK_SHRINK, GTK_SHRINK, 0, 0);
		generic_register[generic_count][i] = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(generic_register[generic_count][i]), 4);
		gtk_table_attach_defaults(GTK_TABLE(table),
				generic_register[generic_count][i],
				col+1, col+2, row, row+1);

		row++;
		if(row >= MAX_VERTICAL)
		{
			row = 0;
			col += 2;
		}
		i++;
	}
	num_registers[generic_count] = i;

	gtk_container_add(GTK_CONTAINER(generic_window[generic_count]), table);
	gtk_widget_show_all(table);

	generic_count++;
	return generic_count - 1;
}
