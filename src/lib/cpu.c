#include <string.h>
#include "libemu.h"
#include "other.h"

typedef struct
{
	unsigned long int pos;
	gboolean one_time_only;
} BKP;

static GtkWidget *cpu_window, *cpu_debugger, *cpu_reference;
static int num_registers, num_flags;
static GtkWidget *registers[255], *flags[255];
static GtkListStore *store;
static long int ip, previous_ip;
static GSList* breakpoints;

/* Debugger columns */
enum
{
	ADDRESS = 0,  /* The hexa address (F432)                       */
	BYTES,        /* The bytes that for the instruction (F4 B2 02) */
	INSTRUCTION,  /* The instruction (MOV A 42)                    */
	BG_COLOR,     /* The color of the cell background              */
	LONG_ADDRESS, /* The numeric address, for consulting           */
	N_COLUMNS     /* The number of columns                         */
};

/*
 * STATIC FUNCTIONS
 */

/* Adjust the reference (first instruction to be decoded) on the debugger */
static void set_debugger_reference(unsigned long initial_pos)
{
	GtkTreeIter iter;
	int i=0, j, cycles, bytes;
	unsigned long pos = initial_pos;
	
	gtk_list_store_clear(store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(cpu_debugger), NULL);
	while(i<255)
	{
		gchar* inst;
		gchar* data;
		gchar* buf = emu_cpu_debug(pos, &cycles, &bytes);
		if(buf == NULL)
		{
			unsigned char bt;
			bytes = 1;
			bt = emu_mem_get(pos);
			if (bt < 32 || bt > 128)
				inst = g_strdup("DATA");
			else
				inst = g_strdup_printf("DATA (%c)", bt);
			data = g_strdup_printf("%02X", bt);
		}
		else
		{
			inst = g_strdup(buf);
			data = g_strdup_printf("%02X", emu_mem_get(pos));
			for(j=1; j<bytes; j++)
				data = g_strdup_printf("%s %02X", data, emu_mem_get(pos+j));
		}
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				ADDRESS, g_strdup_printf("%04X", pos),
				BYTES, data,
				INSTRUCTION, inst,
				BG_COLOR, NULL,
				LONG_ADDRESS, pos,
				-1);
		g_free(inst);
		g_free(data);
		pos += bytes;
		i++;
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(cpu_debugger), GTK_TREE_MODEL(store));

	gtk_entry_set_text(GTK_ENTRY(cpu_reference), g_strdup_printf("%04X", initial_pos));
}

/* Sets the IP on the correct line on the debugger (yellow line) */
static void set_debugger_ip()
{
	GtkTreeIter iter;
	gboolean found = FALSE;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	do
	{
		long pos;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, LONG_ADDRESS, &pos, -1);
		if(pos == previous_ip)
			gtk_list_store_set(store, &iter, BG_COLOR, NULL, -1);
		if(pos == ip)
		{
			gtk_list_store_set(store, &iter, BG_COLOR, "Yellow", -1);
			found = TRUE;
		}
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));

	if(!found)
		set_debugger_reference(ip);
}

/* Set the correct values on the flags and the registers on the debugger */
static void update_flags_and_registers()
{
	int i;
	for(i=0; i<num_registers; i++)
		gtk_entry_set_text(GTK_ENTRY(registers[i]),
			g_strdup_printf("%02X", emu_cpu_register_value(i)));
	for(i=0; i<num_flags; i++)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(flags[i]),
				(emu_cpu_flag_value(i) > 0));
}

/*
 * FUNCTION HANDLERS
 */

/* Handle when the debugger is right clicked */
static gboolean cpu_debugger_clicked(GtkWidget *widget, GdkEvent *event)
{
	GtkMenu *menu;
	GdkEventButton *event_button;

	menu = GTK_MENU(widget);
	event_button = (GdkEventButton*)event;
	if(event_button->button == 3)
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
				event_button->button, event_button->time);
	return FALSE;
}

/* Handle when "set breakpoint" is clicked */
static gboolean bp_item_clicked(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gulong pos;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(cpu_debugger));
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LONG_ADDRESS, &pos, -1);
		emu_cpu_set_breakpoint(pos, (data == 1));
	}
	gtk_tree_selection_unselect_all(selection);
	return FALSE;
}

/*
 * API CALLS
 */

/* Set a new breakpoint */
void emu_cpu_set_breakpoint(unsigned long int pos, int one_time_only)
{
	GtkTreeIter iter;
	GSList *list;
	BKP *bkp;

	/* check if there's already a breakpoint here */
	list = breakpoints;
	while(list)
	{
		if(((BKP*)(list->data))->pos == pos)
			return;
		list = list->next;
	}

	/* add the new breakpoint */
	bkp = g_malloc(sizeof(BKP));
	bkp->pos = pos;
	bkp->one_time_only = (one_time_only != 0);
	g_slist_append(breakpoints, bkp);

	/* update the debugger */
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	do
	{
		long c;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, LONG_ADDRESS, &c, -1);
		if(c == pos)
			gtk_list_store_set(store, &iter, BG_COLOR, "Red", -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
}

/* Initialize a new CPU, loading the filename (dll or shared object) */
int emu_cpu_init(char* filename)
{
	GtkWidget *debug_item;
	GModule* cpu_mod;
	gchar* path;

	int i;
	GtkWidget *toolitem[5];
	GtkWidget *vbox1, 
		    *handlebox,
		      *toolbar,
		          *cpu_run_pause,
			  *cpu_step,
			  *cpu_vblank,
			  *reference_label,
		    *hbox1,
		      *scroll_debugger,
		      *vbox2,
		        *frame_reg,
			  *cpu_registers,
		        *frame_flag,
			  *cpu_flags,
		    *cpu_statusbar;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *popup_menu, 
		  *bp_item, *bp_once_item;

	/* create path (g_module_open requires a full path) */
	if(filename[0] == '.' || filename[0] == '/' || filename[0] == '~')
		path = g_strdup_printf("%s", filename);
	else
		path = g_strdup_printf("./%s", filename);

	/* open module */
	cpu_mod = g_module_open(path, G_MODULE_BIND_LAZY);
	if(!cpu_mod)
	{
		g_error("%s: invalid CPU file (%s)", path, g_module_error());
		return 0;
	}

	/* TODO - check if it's really a CPU */

	/* Connect functions */
	if(!g_module_symbol(cpu_mod, "dev_cpu_name", (gpointer*)&emu_cpu_name))
		g_error("variable dev_cpu_name not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_register_name", (void*)&emu_cpu_register_name))
		g_error("variable dev_cpu_register_name not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_register_value", (void*)&emu_cpu_register_value))
		g_error("variable dev_cpu_register_value not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_flag_name", (void*)&emu_cpu_flag_name))
		g_error("variable dev_cpu_flag_name not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_flag_value", (void*)&emu_cpu_flag_value))
		g_error("variable dev_cpu_flag_value not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_debug", (void*)&emu_cpu_debug))
		g_error("variable dev_cpu_debug not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_reset", (void*)&emu_cpu_reset))
		g_error("variable dev_cpu_reset not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_ip", (void*)&emu_cpu_ip))
		g_error("variable dev_cpu_ip not defined in %s", path);
	if(!g_module_symbol(cpu_mod, "dev_cpu_step", (void*)&emu_cpu_step))
		g_error("variable dev_cpu_step not defined in %s", path);

	/* Connect callbacks */
	if(!connect_callbacks(cpu_mod))
		g_error("CPU callbacks couldn't be connected in %s", path);

	g_message("CPU %s loaded from %s", emu_cpu_name, path);
	
	/* Add a new menu option */
	debug_item = gtk_menu_item_new_with_label(emu_cpu_name);
	gtk_menu_shell_append(GTK_MENU_SHELL(debug_menu), debug_item);

	/* Create a new window, hidden */
	cpu_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(cpu_window), emu_cpu_name);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(cpu_window), TRUE);

	/* Window */
	vbox1 = gtk_vbox_new(FALSE, 0);
	handlebox = gtk_handle_box_new();
	hbox1 = gtk_hbox_new(FALSE, 0);
	cpu_statusbar = gtk_statusbar_new();

	/* Handle Box */
	toolbar = gtk_toolbar_new();
	for(i=0; i<=4; i++)
	{
		toolitem[i] = gtk_tool_item_new();
		gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem[i]), i);
		gtk_tool_item_set_is_important(GTK_TOOL_ITEM(toolitem[i]), TRUE);
	}
	cpu_run_pause = button_with_stock_image("_Run", GTK_STOCK_MEDIA_PLAY);
	gtk_container_add(GTK_CONTAINER(toolitem[0]), cpu_run_pause);
	cpu_step = button_with_stock_image("_Step", GTK_STOCK_MEDIA_NEXT);
	gtk_container_add(GTK_CONTAINER(toolitem[1]), cpu_step);
	cpu_vblank = button_with_stock_image("Go to Next _Frame", GTK_STOCK_JUSTIFY_FILL);
	gtk_container_add(GTK_CONTAINER(toolitem[2]), cpu_vblank);
	reference_label = gtk_label_new("Reference");
	gtk_misc_set_padding(GTK_MISC(reference_label), 6, 6);
	gtk_container_add(GTK_CONTAINER(toolitem[3]), reference_label);
	cpu_reference = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(cpu_reference), 8);
	gtk_container_add(GTK_CONTAINER(toolitem[4]), cpu_reference);
	gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

	/* Debugger list */
	store = gtk_list_store_new(N_COLUMNS, 
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_LONG);
	cpu_debugger = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(cpu_debugger), TRUE);
	for(i=0; i<3; i++)
	{
		column = gtk_tree_view_column_new();
		switch(i)
		{
			case ADDRESS:
				gtk_tree_view_column_set_title(column, "Address");
				break;
			case BYTES:
				gtk_tree_view_column_set_title(column, "Bytes");
				break;
			case INSTRUCTION:
				gtk_tree_view_column_set_title(column, "Instruction");
				break;
		}
		gtk_tree_view_append_column(GTK_TREE_VIEW(cpu_debugger), column);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", i);
		gtk_tree_view_column_add_attribute(column, renderer, "cell-background", BG_COLOR);
	}

	/* Debugger view */
	hbox1 = gtk_hbox_new(FALSE, 0);
	scroll_debugger = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroll_debugger), cpu_debugger);
	gtk_box_pack_start(GTK_BOX(hbox1), scroll_debugger, TRUE, TRUE, 0);

	/* Registers/flags */
	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_set_spacing(GTK_BOX(vbox2), 6);
	frame_reg = gtk_frame_new("Registers");
	gtk_box_pack_start(GTK_BOX(vbox2), frame_reg, FALSE, FALSE, 0);
	frame_flag = gtk_frame_new("Flags");
	gtk_box_pack_start(GTK_BOX(vbox2), frame_flag, FALSE, FALSE, 0);
	cpu_registers = gtk_table_new(1, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(frame_reg), cpu_registers);
	cpu_flags = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame_flag), cpu_flags);
	gtk_box_pack_start(GTK_BOX(hbox1), vbox2, FALSE, FALSE, 12);
	
	/* Statusbar */
	cpu_statusbar = gtk_statusbar_new();

	gtk_box_pack_start(GTK_BOX(vbox1), handlebox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 6);
	gtk_box_pack_end(GTK_BOX(vbox1), cpu_statusbar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(cpu_window), vbox1);

	/* Popup menu */
	popup_menu = gtk_menu_new();
	bp_item = gtk_menu_item_new_with_label("Set breakpoint");
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), bp_item);
	bp_once_item = gtk_menu_item_new_with_label("Set one-time-only breakpoint");
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), bp_once_item);
	g_signal_connect_swapped(cpu_debugger, "button_press_event",
			G_CALLBACK(cpu_debugger_clicked), popup_menu);
	g_signal_connect_swapped(bp_item, "button_press_event",
			G_CALLBACK(bp_item_clicked), 1);
	g_signal_connect_swapped(bp_once_item, "button_press_event",
			G_CALLBACK(bp_item_clicked), 0);
	gtk_widget_show_all(popup_menu);

	/* Add registers and flags */
	i=0;
	while(emu_cpu_register_name(i))
	{
		gtk_table_resize(GTK_TABLE(cpu_registers), i+1, 2);
		gtk_table_attach(GTK_TABLE(cpu_registers), 
				gtk_label_new(emu_cpu_register_name(i)),
				0, 1, i, i+1,
				GTK_SHRINK, GTK_SHRINK, 6, 6);
		registers[i] = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(registers[i]), 4);
		gtk_table_attach(GTK_TABLE(cpu_registers), 
				registers[i], 1, 2, i, i+1,
				GTK_SHRINK, GTK_SHRINK, 0, 0);
		i++;
	}
	num_registers = i;

	i=0;
	while(emu_cpu_flag_name(i))
	{
		flags[i] = gtk_check_button_new_with_label(emu_cpu_flag_name(i));
		gtk_box_pack_start(GTK_BOX(cpu_flags), flags[i], FALSE, FALSE, 0);
		i++;
	}
	num_flags = i;

	gtk_widget_show_all(vbox1);

	update_flags_and_registers();
	set_debugger_reference(0);
	ip = previous_ip = 0;
	set_debugger_ip();

	gtk_window_present(GTK_WINDOW(cpu_window));
}
