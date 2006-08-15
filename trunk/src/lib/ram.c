#include "libemu.h"
#include "other.h"

unsigned char* ram = NULL;
unsigned long int size = 0;
glong first, last;
static GtkWidget *ram_window, *ram_debugger;
static GtkListStore *store;

enum
{
	ADDRESS = 0,
	BYTE_0, BYTE_1, BYTE_2, BYTE_3, BYTE_4, BYTE_5, BYTE_6, BYTE_7,
	BYTE_8, BYTE_9, BYTE_A, BYTE_B, BYTE_C, BYTE_D, BYTE_E, BYTE_F,
	BYTE_0_BG, BYTE_1_BG, BYTE_2_BG, BYTE_3_BG, 
	BYTE_4_BG, BYTE_5_BG, BYTE_6_BG, BYTE_7_BG,
	BYTE_8_BG, BYTE_9_BG, BYTE_A_BG, BYTE_B_BG, 
	BYTE_C_BG, BYTE_D_BG, BYTE_E_BG, BYTE_F_BG,
	LONG_ADDRESS,
	N_COLUMNS
};

/*
 * PRIVATE FUNCTIONS
 */



/*
 * EVENT HANDLERS
 */

/* When the RAM menu item is clicked on the main window */
static void ram_show_hide(GtkCheckMenuItem *item, gpointer data)
{
	if(item->active)
		gtk_window_present(GTK_WINDOW(ram_window));
	else
		gtk_widget_hide(ram_window);
}

/* When the close button is clicked on the memory window */
static gboolean ram_hide(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data), FALSE);
	gtk_widget_hide(ram_window);
	return TRUE;
}

/* When a new reference is set */
static void ram_reference_changed(GtkEntry* entry, gpointer data)
{
	/*
	if(emu_cpu_get_debugger_reference() < 0)
	{
		gtk_entry_set_text(GTK_ENTRY(cpu_reference), previous_reference);
		return;
	}

	emu_cpu_set_debugger_reference(emu_cpu_get_debugger_reference());
	*/
}

/*
 * API CALLS
 */

/* Create the memory */
void emu_mem_init(unsigned long sz)
{
	GtkWidget *debug_item;
	GtkWidget *vbox, *hbox2, *reference_label, *ram_reference,
		  *scroll_debugger;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	gint i;

	if(size > 0)
	{
		g_error("A memory was already set");
		return;
	}

	ram = g_malloc(sz);
	if(!ram)
	{
		g_error("Not enough free memory when reseving memory for RAM.");
		return;
	}
	size = sz;

	/* Add a new menu option */
	debug_item = gtk_check_menu_item_new_with_label(g_strdup_printf("RAM - %dk", size / 1024));
	gtk_menu_shell_append(GTK_MENU_SHELL(debug_menu), debug_item);
	g_signal_connect(debug_item, "toggled", G_CALLBACK(ram_show_hide), NULL);

	/* Create a new window, hidden */
	ram_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ram_window), g_strdup_printf("RAM - %dk", size / 1024));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(ram_window), TRUE);
	g_signal_connect(ram_window, "delete_event", G_CALLBACK(ram_hide), debug_item);

	vbox = gtk_vbox_new(FALSE, 6);

	/* Reference */
	hbox2 = gtk_hbox_new(FALSE, 6);
	reference_label = gtk_label_new("Reference");
	gtk_box_pack_start(GTK_BOX(hbox2), reference_label, FALSE, FALSE, 6);
	ram_reference = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(ram_reference), 8);
	gtk_box_pack_start(GTK_BOX(hbox2), ram_reference, FALSE, FALSE, 0);
	g_signal_connect_swapped(ram_reference, "activate",
			G_CALLBACK(ram_reference_changed), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

	/* Memory table */
	store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_LONG);
	ram_debugger = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ram_debugger), TRUE);
	
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Address");
	gtk_tree_view_append_column(GTK_TREE_VIEW(ram_debugger), column);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	
	for(i=1; i<17; i++)
	{
		column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(column, g_strdup_printf("...%1X", i-1));
		gtk_tree_view_append_column(GTK_TREE_VIEW(ram_debugger), column);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", i);
		gtk_tree_view_column_add_attribute(column, renderer, "cell-background", i+16);
	}
	//g_signal_connect_swapped(ram_debugger, "button_press_event",
	//		G_CALLBACK(ram_debugger_clicked), NULL);

	scroll_debugger = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroll_debugger), ram_debugger);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_debugger, TRUE, TRUE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(ram_window), 6);
	gtk_container_add(GTK_CONTAINER(ram_window), vbox);
	gtk_widget_show_all(ram_window);
	gtk_window_present(GTK_WINDOW(ram_window));
}

/* Create the memory, using kilobytes as argument */
void emu_mem_init_k(unsigned int sz)
{
	emu_mem_init(sz * 1024);
}

/* Sets the memory directly, without passing data to the components */
void emu_mem_set_direct(unsigned long int pos, unsigned char data)
{
	if(pos > (size-1) || pos < 0)
		g_critical("Trying to write outside memory bounds, in position 0x%x!", pos);
	else
		ram[pos] = data;
}

/* Sets a byte into the memory */
void emu_mem_set(unsigned long int pos, unsigned char data)
{
	emu_mem_set_direct(pos, data);
}

/* Gets a bytes from the memory */
unsigned char emu_mem_get(unsigned long int pos)
{
	if(pos > (size-1) || pos < 0)
		g_critical("Trying to read outside memory bounds, from position 0x%x!", pos);
	else
		return ram[pos];
}

/* Returns the size of the memory, in bytes */
unsigned long int emu_mem_size()
{
	return size;
}
