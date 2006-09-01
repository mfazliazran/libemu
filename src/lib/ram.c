#include "libemu.h"
#include "other.h"

typedef struct {
	int device;
	unsigned long int initial;
	unsigned long int final;
} MEMORY_MAP;

unsigned char* ram = NULL;
unsigned long int size = 0;
glong first, last;
static GtkWidget *mem_window, *mem_reference, *mem_debugger;
static GtkListStore *store;
static gchar* previous_reference;
glong last_mem, previous_mem;
GSList *memory_map = NULL;

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
static void mem_show_hide(GtkToggleButton *item, gpointer data)
{
	if(gtk_toggle_button_get_active(item))
	{
		gtk_window_present(GTK_WINDOW(mem_window));
		emu_mem_set_reference(emu_mem_get_reference());
	}
	else
		gtk_widget_hide(mem_window);
}

/* When the close button is clicked on the memory window */
static gboolean mem_hide(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), FALSE);
	gtk_widget_hide(mem_window);
	return TRUE;
}

/* When a new reference is set */
static void mem_reference_changed(GtkEntry* entry, gpointer data)
{
	if(emu_mem_get_reference() < 0)
	{
		gtk_entry_set_text(GTK_ENTRY(mem_reference), previous_reference);
		return;
	}

	emu_mem_set_reference(emu_mem_get_reference());
}

/*
 * API CALLS
 */

unsigned long emu_mem_get_reference()
{
	gchar* hex;
	if(size == 0) 
		return -1;
	hex = gtk_entry_get_text(GTK_ENTRY(mem_reference));
	return hex2long(hex);	
}

void emu_mem_set_reference(unsigned long initial_pos)
{
	GtkTreeIter iter;
	int i=0, j, cycles, bytes;
	unsigned long pos;

	initial_pos &= 0xFFFFFFF0;
	pos = first = initial_pos;

	if(size == 0) 
		return;

	if(!GTK_WIDGET_VISIBLE(mem_window))
		return;

	previous_reference = gtk_entry_get_text(GTK_ENTRY(mem_reference));
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(mem_debugger), NULL);
	gtk_list_store_clear(store);
	while(i<16 && pos < size)
	{
		gchar *data[16];
		for(j=0; j<16; j++)
			if(pos+j < size)
				data[j] = g_strdup_printf("%02X", ram[pos+j]);
			else
				data[j] = NULL;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				ADDRESS, g_strdup_printf("%03X_", pos / 0x10),
				BYTE_0, data[0], BYTE_1, data[1],
				BYTE_2, data[2], BYTE_3, data[3],
				BYTE_4, data[4], BYTE_5, data[5],
				BYTE_6, data[5], BYTE_7, data[7],
				BYTE_8, data[6], BYTE_9, data[9],
				BYTE_A, data[10], BYTE_B, data[11],
				BYTE_C, data[12], BYTE_D, data[13],
				BYTE_E, data[14], BYTE_F, data[15],
				LONG_ADDRESS, pos,
				-1);
		if(previous_mem >= pos && previous_mem < pos+16)
			gtk_list_store_set(store, &iter, previous_mem - pos + 17, NULL, -1);
		if(last_mem >= pos && last_mem < pos+16)
		{
			/*
			GtkTreePath *path;
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(mem_debugger),
					path, NULL, FALSE, 0.5, 0);
			gtk_tree_path_free(path);
			*/
			gtk_list_store_set(store, &iter, 
					last_mem - pos + 17, "Yellow", 
					-1);
		}
		for(j=0; j<16; j++)
			g_free(data[j]);
		pos += 16;
		i++;
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(mem_debugger), GTK_TREE_MODEL(store));

	last = MIN(pos, size);

	gtk_entry_set_text(GTK_ENTRY(mem_reference), g_strdup_printf("%04X", initial_pos));
}

/* Create the memory */
void emu_mem_init(unsigned long sz)
{
	GtkWidget *memory_button;
	GtkWidget *vbox, *hbox2, *reference_label, *scroll_debugger;
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
	/*
	debug_item = gtk_check_menu_item_new_with_label(g_strdup_printf("RAM - %dk", size / 1024));
	gtk_menu_shell_append(GTK_MENU_SHELL(debug_menu), debug_item);
	*/
	memory_button = button_with_pixmap_image(g_strdup_printf("Memory - %dk", size / 1024), P_MEMORY, TRUE);
	gtk_box_pack_end_defaults(GTK_BOX(internal_hbox), memory_button);
	g_signal_connect(memory_button, "toggled", G_CALLBACK(mem_show_hide), NULL);

	/* Create a new window, hidden */
	mem_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(mem_window), g_strdup_printf("RAM - %dk", size / 1024));
	gtk_window_set_default_size(GTK_WINDOW(mem_window), 440, 275);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(mem_window), TRUE);
	g_signal_connect(mem_window, "delete_event", G_CALLBACK(mem_hide), memory_button);

	vbox = gtk_vbox_new(FALSE, 6);

	/* Reference */
	hbox2 = gtk_hbox_new(FALSE, 6);
	reference_label = gtk_label_new("Reference");
	gtk_box_pack_start(GTK_BOX(hbox2), reference_label, FALSE, FALSE, 6);
	mem_reference = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(mem_reference), 8);
	gtk_entry_set_text(GTK_ENTRY(mem_reference), "0000");
	gtk_box_pack_start(GTK_BOX(hbox2), mem_reference, FALSE, FALSE, 0);
	g_signal_connect_swapped(mem_reference, "activate",
			G_CALLBACK(mem_reference_changed), NULL);
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
	mem_debugger = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mem_debugger), TRUE);
	
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "Address");
	gtk_tree_view_append_column(GTK_TREE_VIEW(mem_debugger), column);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", ADDRESS);
	
	for(i=1; i<17; i++)
	{
		column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(column, g_strdup_printf("__%1X", i-1));
		gtk_tree_view_append_column(GTK_TREE_VIEW(mem_debugger), column);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", i);
		gtk_tree_view_column_add_attribute(column, renderer, "cell-background", i+16);
	}

	scroll_debugger = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroll_debugger), mem_debugger);
	gtk_box_pack_start(GTK_BOX(vbox), scroll_debugger, TRUE, TRUE, 0);

	emu_mem_set_reference(0);

	gtk_container_set_border_width(GTK_CONTAINER(mem_window), 6);
	gtk_container_add(GTK_CONTAINER(mem_window), vbox);
	gtk_widget_show_all(vbox);
	//gtk_window_present(GTK_WINDOW(mem_window));
}

/* Create the memory, using kilobytes as argument */
void emu_mem_init_k(unsigned int sz)
{
	emu_mem_init(sz * 1024);
}

/* Sets the memory directly, without passing data to the components */
void emu_mem_set_direct(unsigned long int pos, unsigned char data)
{
	GtkTreeIter iter;
	gboolean found = FALSE;

	if(pos > (size-1) || pos < 0)
	{
		g_critical("Trying to write outside memory bounds, in position 0x%x!", pos);
		return;
	}
	
	ram[pos] = data;
	previous_mem = last_mem;
	last_mem = pos;
}

/* Sets a byte into the memory */
void emu_mem_set(unsigned long int pos, unsigned char data)
{
	GSList *list;
	gboolean set_memory = TRUE;

	/* check the memory maps */
	list = memory_map;

	while(list)
	{
		if(((MEMORY_MAP*)(list->data))->initial <= pos && 
		   ((MEMORY_MAP*)(list->data))->final >= pos)
		{
			switch(((MEMORY_MAP*)(list->data))->device)
			{
				case VIDEO:
					if(!emu_video_memory_set(pos, data))
						set_memory = FALSE;
					break;
				default:
					if(!emu_generic_memory_set[((MEMORY_MAP*)(list->data))->device](pos, data))
						set_memory = FALSE;
			}
		}
		list = list->next;
	}

	/* set the memory byte */
	if(set_memory)
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

/* Add a new memory map. Returns -1 on success, 0 on faliure. */
int emu_mem_map_add(int device, unsigned long int initial, unsigned long int final)
{
	MEMORY_MAP *mm;

	if(device > generic_count || (device <= 0 && device != VIDEO))
	{
		emu_error("Invalid device.");
		return 0;
	}

	mm = g_malloc(sizeof(MEMORY_MAP));
	mm->device = device;
	mm->initial = initial;
	mm->final = final;
	memory_map = g_slist_append(memory_map, mm);
}
