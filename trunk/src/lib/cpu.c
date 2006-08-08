#include <string.h>
#include "libemu.h"
#include "other.h"

static GtkWidget *cpu_window;
static int num_registers, num_flags;
static GtkWidget *registers[255], *flags[255];
static GtkListStore *store;

enum
{
	ADDRESS = 0,
	INSTRUCTION,
	ICON,
	BREAKPOINT,
	IP_POINTER,
	N_COLUMNS
};

static void update_debugger(unsigned long initial_pos)
{
	GtkTreeIter iter;
	int i=0, cycles, bytes;
	unsigned long pos = initial_pos;

	while(i<255)
	{
		gchar* inst = g_strdup_printf(emu_cpu_debug(pos, &cycles, &bytes));
		if(!strcmp(inst,""))
			break;
		g_message(inst);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				ADDRESS, g_sprintf("%04x", pos),
				INSTRUCTION, inst,
				-1);
		pos += bytes;
		i++;
	}
}

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

int emu_cpu_init(char* filename)
{
	GtkWidget *debug_item;
	GModule* cpu_mod;
	gchar* path;

	int i;
	GtkWidget *vbox1, 
		    *handlebox,
		      *toolbar,
		        *toolitem[5],
		          *cpu_run_pause,
			  *cpu_step,
			  *reference_label,
			  *cpu_reference,
		    *hbox1,
		      *scroll_debugger,
		        *cpu_debugger,
		      *vbox2,
		        *frame_reg,
			  *cpu_registers,
		        *frame_flag,
			  *cpu_flags,
		    *cpu_statusbar;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

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
	}
	cpu_run_pause = button_with_stock_image("_Run", GTK_STOCK_MEDIA_PLAY);
	gtk_container_add(GTK_CONTAINER(toolitem[0]), cpu_run_pause);
	cpu_step = button_with_stock_image("_Step", GTK_STOCK_MEDIA_NEXT);
	gtk_container_add(GTK_CONTAINER(toolitem[1]), cpu_step);
	reference_label = gtk_label_new("Reference");
	gtk_misc_set_padding(GTK_MISC(reference_label), 6, 6);
	gtk_container_add(GTK_CONTAINER(toolitem[2]), reference_label);
	cpu_reference = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(cpu_reference), 8);
	gtk_container_add(GTK_CONTAINER(toolitem[3]), cpu_reference);
	gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

	/* Debugger list */
	store = gtk_list_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	cpu_debugger = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(
			"BP", renderer, NULL);
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "foreground", "red", NULL);
	column = gtk_tree_view_column_new_with_attributes(
			"Address", renderer, "text", ADDRESS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(cpu_debugger), column);
	column = gtk_tree_view_column_new_with_attributes(
			"Instruction", renderer, "text", INSTRUCTION, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(cpu_debugger), column);

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
	update_debugger(0);

	gtk_window_present(GTK_WINDOW(cpu_window));
}
