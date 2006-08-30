#include <string.h>
#include "libemu.h"
#include "other.h"

typedef struct
{
	unsigned long int pos;
	gboolean one_time_only;
} BKP;

/* Variables */
static gboolean cpu_loaded = FALSE;
static GtkWidget* cpu_window;
static GtkWidget *cpu_debugger, *cpu_reference, *cpu_step, *cpu_vblank;
static GtkWidget *run_image, *run_label;
GtkWidget *popup_menu, *popup_menu_unset;
static int num_registers, num_flags;
static GtkWidget *registers[255], *flags[255];
static GtkListStore *store;
static long int ip, previous_ip;
static GSList* breakpoints;
static gchar* previous_reference;
glong h_cycles = 0, v_cycles = 0, total_cycles = 0;

/* Prototypes */
static void cpu_run_pause_clicked(GtkButton* cpu_run_pause, gpointer data);

/* Debugger columns */
enum
{
	ADDRESS = 0,  /* The hexa address (F432)                       */
	BYTES,        /* The bytes that for the instruction (F4 B2 02) */
	INSTRUCTION,  /* The instruction (MOV A 42)                    */
	CYCLES,       /* Number of cycles of the instruction           */
	BG_COLOR,     /* The color of the cell background              */
	LONG_ADDRESS, /* The numeric address, for consulting           */
	N_COLUMNS     /* The number of columns                         */
};

/*
 * PRIVATE FUNCTIONS
 */

/* Returns if a given position has a breakpoint */
static inline gboolean is_breakpoint(unsigned long int pos, gboolean delete_weak)
{
	GSList *list;

	list = breakpoints;

	while(list)
	{
		if(((BKP*)(list->data))->pos == pos)
		{
			/* check if it's a weak breakpoint */
			if(delete_weak)
				if(((BKP*)(list->data))->one_time_only)
					breakpoints = g_slist_remove(breakpoints, list->data);
			return TRUE;
		}
		list = list->next;
	}
	return FALSE;
}

/* Set the correct values on the flags and the registers on the debugger */
static void update_flags_and_registers()
{
	int i;

	if(!cpu_loaded) return;

	for(i=0; i<num_registers; i++)
		gtk_entry_set_text(GTK_ENTRY(registers[i]),
			g_strdup_printf("%02X", emu_cpu_register_value(i)));
	for(i=0; i<num_flags; i++)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(flags[i]),
				(emu_cpu_flag_value(i) > 0));
}

/* Sets the IP on the correct line on the debugger (yellow line) */
static void update_debugger(gboolean find_ip)
{
	GtkTreeIter iter;
	gboolean found = FALSE;

	if(!cpu_loaded) return;

	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		emu_cpu_set_debugger_reference(ip);
		return;
	}
	do
	{
		long pos;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, LONG_ADDRESS, &pos, -1);
		if(pos == previous_ip)
			if(is_breakpoint(previous_ip, FALSE))
				gtk_list_store_set(store, &iter, BG_COLOR, "Red", -1);
			else
				gtk_list_store_set(store, &iter, BG_COLOR, NULL, -1);
		if(pos == ip)
		{
			GtkTreePath *path;
			gtk_list_store_set(store, &iter, BG_COLOR, "Yellow", -1);
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(cpu_debugger),
					path, NULL, FALSE, 0.5, 0);
			gtk_tree_path_free(path);
			found = TRUE;
		}
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));

	update_flags_and_registers();

	if(!found && find_ip)
		emu_cpu_set_debugger_reference(ip);
}

static inline void execute_devices_step_exact(int num_cycles)
{
	int i;
	if(emu_video_sync == EXACT_SYNC)
		emu_video_step(emu_video_cycles * num_cycles);
	for(i=1; i<generic_count; i++)
		if(emu_generic_sync[i] == EXACT_SYNC)
			emu_generic_step[i](num_cycles * emu_generic_cycles[i]);
}

static inline void execute_devices_step_horizontal(int num_cycles)
{
	int i;
	if(emu_video_sync == HORIZONTAL_SYNC)
		emu_video_step(emu_video_cycles * num_cycles);
	for(i=1; i<generic_count; i++)
		if(emu_generic_sync[i] == HORIZONTAL_SYNC)
			emu_generic_step[i](num_cycles * emu_generic_cycles[i]);
}

/* Execute one step */
static inline gboolean execute_one_step()
{
	int num_cycles, i, video_cycles, pre_y;

	if(!emu_cpu_step(&num_cycles))
	{
		emu_error(g_strdup_printf("Instruction invalid in address 0x%04X!", emu_cpu_ip()));
		if(running)
			cpu_run_pause_clicked(NULL, NULL);
		update_debugger(TRUE);
		return FALSE;
	}

	execute_devices_step_exact(num_cycles);
	pre_y = *emu_video_pos_y;
	h_cycles += num_cycles;

	if(*emu_video_wait_vsync != 0)
	{
		if(*emu_video_pos_y > *emu_video_scanlines_vblank
		&& *emu_video_pos_y <= *emu_video_scanlines_vblank + *emu_video_pixels_y)
			emu_video_update_screen();
		*emu_video_pos_x = 0;
		*emu_video_pos_y = 0;
		*emu_video_wait_vsync = 0;		
	}
	else
	{
		if (*emu_video_wait_hsync != 0)
		{
			execute_devices_step_horizontal(h_cycles);
			h_cycles = 0;
			*emu_video_pos_x = 0;
			*emu_video_pos_y = *emu_video_pos_y + 1;
			*emu_video_wait_hsync = 0;
		}
		else
		{
			*emu_video_pos_x = *emu_video_pos_x + (num_cycles * emu_video_cycles);
			/* check for hsync */
			while(*emu_video_pos_x > *emu_video_scanline_cycles)
			{
				execute_devices_step_horizontal(h_cycles);
				h_cycles = 0;
				*emu_video_pos_y = *emu_video_pos_y + 1;
				*emu_video_pos_x = *emu_video_pos_x - *emu_video_scanline_cycles;
			}
		}
	}

	/* adjust Y position */
	if(pre_y < (*emu_video_scanlines_vblank + *emu_video_pixels_y)
	&& *emu_video_pos_y > (*emu_video_scanlines_vblank + *emu_video_pixels_y))
		emu_video_update_screen();
	if(*emu_video_pos_y > (*emu_video_scanlines_vblank + *emu_video_pixels_y + *emu_video_scanlines_overscan))
		*emu_video_pos_y = 0;

	/*
	video_cycles = emu_video_cycles * num_cycles;
	execute_devices_step_exact(num_cycles);

	if(*emu_video_wait_hsync != 0)
	{
		*emu_video_pos_x = 0;
		*emu_video_pos_y = *emu_video_pos_y + 1;
		*emu_video_wait_hsync = 0;
		if(emu_video_sync == HORIZONTAL_SYNC)
			emu_video_step(h_cycles);
		execute_generic_step_horizontal(total_cycles);
		total_cycles = 0;
		h_cycles = 0;
	}
	else if(*emu_video_wait_vsync != 0)
	{
		if(*emu_video_pos_y > *emu_video_scanlines_vblank
		&& *emu_video_pos_y <= *emu_video_scanlines_vblank + *emu_video_pixels_y)
			emu_video_update_screen();
		*emu_video_pos_x = 0;
		*emu_video_pos_y = 0;
		*emu_video_wait_vsync = 0;
	}
	
	h_cycles += video_cycles;
	total_cycles += num_cycles;
	if(h_cycles > (*emu_video_scanline_cycles))
	{
		execute_devices_step_horizontal(total_cycles);
		total_cycles = 0;
		h_cycles -= (*emu_video_scanline_cycles);
	}

	if(((*emu_video_pos_x) + video_cycles) > (*emu_video_scanline_cycles))
	{
		int pre = *emu_video_pos_y;
		*emu_video_pos_y += (int)(((*emu_video_pos_x) + video_cycles) / (*emu_video_scanline_cycles));
		*emu_video_pos_x = ((*emu_video_pos_x) + video_cycles) - (*emu_video_scanline_cycles);
		if (pre < (*emu_video_scanlines_vblank + *emu_video_pixels_y)
		&& *emu_video_pos_y > (*emu_video_scanlines_vblank + *emu_video_pixels_y))
			emu_video_update_screen();
		if(*emu_video_pos_y > (*emu_video_scanlines_vblank + *emu_video_pixels_y + *emu_video_scanlines_overscan))
			*emu_video_pos_y = *emu_video_pos_y - (*emu_video_scanlines_vblank + *emu_video_pixels_y + *emu_video_scanlines_overscan) - 1;
	}
	else
		*emu_video_pos_x = (*emu_video_pos_x) + video_cycles;
	*/

	ip = emu_cpu_ip();
	return TRUE;
}

/* Thread that runs the emulator */
gboolean run()
{
	BKP *bkp;
	GtkTreeIter iter;

	/* clear IP from debugger */
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	do
	{
		long pos;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, LONG_ADDRESS, &pos, -1);
		if(!is_breakpoint(pos, FALSE))
			gtk_list_store_set(store, &iter, BG_COLOR, NULL, -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));

	/* run! */
	previous_ip = ip;
	while(running)
	{
		if(!execute_one_step())
			return FALSE;

		/* check if it's a breakpoint */
		if(is_breakpoint(ip, TRUE))
		{
			cpu_run_pause_clicked(NULL, NULL);
			update_debugger(TRUE);
			video_update();
			generic_update();
			return FALSE;
		}

		gtk_main_iteration_do(FALSE);
	}
	update_debugger(TRUE);
	emu_mem_set_reference(emu_mem_get_reference());
	video_update();
	generic_update();
	return FALSE;
}

/*
 * EVENT HANDLERS
 */

/* When the CPU menu item is clicked on the main window */
static void cpu_show_hide(GtkCheckMenuItem *item, gpointer data)
{
	if(item->active)
	{
		gtk_window_present(GTK_WINDOW(cpu_window));
		emu_cpu_set_debugger_reference(emu_cpu_get_debugger_reference());
	}
	else
		gtk_widget_hide(cpu_window);
}

/* When the close button is clicked on the debugger */
static gboolean cpu_hide(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data), FALSE);
	gtk_widget_hide(cpu_window);
	return TRUE;
}

/* Handle when the debugger is right clicked */
static gboolean cpu_debugger_clicked(GtkWidget *widget, GdkEvent *event)
{
	GdkEventButton *event_button;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	gulong pos;

	event_button = (GdkEventButton*)event;
	if(event_button->button != 3)
		return FALSE;
	
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(cpu_debugger),
				(gint)event_button->x, (gint)event_button->y,
				&path, NULL, NULL, NULL))
	{
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(cpu_debugger));
		if(gtk_tree_model_get_iter(model, &iter, path))
		{
			gtk_tree_model_get(model, &iter, LONG_ADDRESS, &pos, -1);
			if(is_breakpoint(pos, FALSE))
				gtk_menu_popup(GTK_MENU(popup_menu_unset), NULL, NULL, NULL, NULL, event_button->button, event_button->time);
			else
				gtk_menu_popup(GTK_MENU(popup_menu), NULL, NULL, NULL, NULL, event_button->button, event_button->time);
		}
	}
	
	return FALSE;
}

/* Handle when "set breakpoint" is clicked */
static gboolean bp_item_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gulong pos;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(cpu_debugger));
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LONG_ADDRESS, &pos, -1);
		emu_cpu_set_breakpoint(pos, GPOINTER_TO_INT(data));
	}
	gtk_tree_selection_unselect_all(selection);
	return FALSE;
}

/* Handle when "unset breakpoint" is clicked */
static gboolean bp_item_unset_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gulong pos;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(cpu_debugger));
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LONG_ADDRESS, &pos, -1);
		emu_cpu_unset_breakpoint(pos);
	}
	gtk_tree_selection_unselect_all(selection);
	return FALSE;
}

/* When a new reference is set */
static void cpu_reference_changed(GtkEntry* entry, gpointer data)
{
	if(emu_cpu_get_debugger_reference() < 0)
	{
		gtk_entry_set_text(GTK_ENTRY(cpu_reference), previous_reference);
		return;
	}

	emu_cpu_set_debugger_reference(emu_cpu_get_debugger_reference());
}

/* When the step button is clicked */
static void cpu_step_clicked(GtkButton* cpu_step, gpointer data)
{
	previous_ip = ip;

	if(!execute_one_step())
		return;

	ip = emu_cpu_ip();
	update_debugger(TRUE);
	video_update();
	generic_update();
	emu_mem_set_reference(emu_mem_get_reference());
}

/* When the button Run/Pause is clicked */
static void cpu_run_pause_clicked(GtkButton* cpu_run_pause, gpointer data)
{
	if(!running)
	{
		gtk_label_set_text_with_mnemonic(GTK_LABEL(run_label), "_Pause");
		gtk_image_set_from_stock(GTK_IMAGE(run_image), GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_widget_set_sensitive(cpu_step, FALSE);
		gtk_widget_set_sensitive(cpu_vblank, FALSE);
		gtk_widget_set_sensitive(cpu_reference, FALSE);
		running = TRUE;
		g_idle_add_full(G_PRIORITY_HIGH_IDLE, run, NULL, NULL);
	}
	else
	{
		running = FALSE;
		gtk_label_set_text_with_mnemonic(GTK_LABEL(run_label), "_Run");
		gtk_image_set_from_stock(GTK_IMAGE(run_image), GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_widget_set_sensitive(cpu_step, TRUE);
		gtk_widget_set_sensitive(cpu_vblank, TRUE);
		gtk_widget_set_sensitive(cpu_reference, TRUE);
	}
}

/*
 * API CALLS
 */

/* Return the reference number */
unsigned long emu_cpu_get_debugger_reference()
{
	gchar* hex;
	if(!cpu_loaded) return -1;
	hex = gtk_entry_get_text(GTK_ENTRY(cpu_reference));
	return hex2long(hex);
}

/* Adjust the reference (first instruction to be decoded) on the debugger */
void emu_cpu_set_debugger_reference(unsigned long initial_pos)
{
	GtkTreeIter iter;
	int i=0, j, cycles, bytes;
	unsigned long pos = initial_pos;
	ip = emu_cpu_ip();

	if(!cpu_loaded) return;

	previous_reference = gtk_entry_get_text(GTK_ENTRY(cpu_reference));
	
	gtk_list_store_clear(store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(cpu_debugger), NULL);
	while(i<255 && pos < emu_mem_size())
	{
		gchar* inst;
		gchar* data;
		gchar* cyc;
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
			cyc = g_strdup_printf("0");
		}
		else
		{
			inst = g_strdup(buf);
			data = g_strdup_printf("%02X", emu_mem_get(pos));
			for(j=1; j<bytes; j++)
				data = g_strdup_printf("%s %02X", data, emu_mem_get(pos+j));
			cyc = g_strdup_printf("%d", cycles);
		}
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				ADDRESS, g_strdup_printf("%04X", pos),
				BYTES, data,
				INSTRUCTION, inst,
				BG_COLOR, NULL,
				LONG_ADDRESS, pos,
				CYCLES, cyc,
				-1);
		g_free(inst);
		g_free(data);
		pos += bytes;
		i++;
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(cpu_debugger), GTK_TREE_MODEL(store));

	gtk_entry_set_text(GTK_ENTRY(cpu_reference), g_strdup_printf("%04X", initial_pos));

	update_debugger(FALSE);
}

/* Set a new breakpoint */
void emu_cpu_set_breakpoint(unsigned long int pos, int one_time_only)
{
	GtkTreeIter iter;
	GSList *list;
	BKP *bkp;

	if(!cpu_loaded) return;

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
	bkp->one_time_only = one_time_only;
	breakpoints = g_slist_append(breakpoints, bkp);

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

/* Set a new breakpoint */
void emu_cpu_unset_breakpoint(unsigned long int pos)
{
	GtkTreeIter iter;
	GSList *list;
	BKP *bkp;
	gboolean ok = FALSE;

	if(!cpu_loaded) return;

	/* delete the breakpoint */
	list = breakpoints;
	while(list)
	{
		if(((BKP*)(list->data))->pos == pos)
		{
			breakpoints = g_slist_remove(breakpoints, list->data);
			ok = TRUE;
			break;
		}
		list = list->next;
	}
	if(!ok) return;

	/* update the debugger */
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	do
	{
		long c;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, LONG_ADDRESS, &c, -1);
		if(c == pos)
			if(pos == ip)
				gtk_list_store_set(store, &iter, BG_COLOR, "Yellow", -1);
			else
				gtk_list_store_set(store, &iter, BG_COLOR, NULL, -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
}

/* Initialize a new CPU, loading the filename (dll or shared object) */
int emu_cpu_init(char* filename)
{
	GtkWidget *debug_item;
	GModule *cpu_mod;
	gchar *path, *type;

	int i;
	GtkWidget *toolitem[5];
	GtkWidget *vbox1, 
		    *handlebox,
		      *toolbar,
		          *cpu_run_pause,
		    *hbox1,
		      *vbox3,
		        *scroll_debugger,
			*hbox2,
			  *reference_label,
		      *vbox2,
		        *frame_reg,
			  *cpu_registers,
		        *frame_flag,
			  *cpu_flags,
		    *cpu_statusbar;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *bp_item, *bp_once_item, *bp_item_unset;
	GtkTooltips* tips;

	if(cpu_loaded)
	{
		emu_error("A CPU was already loaded.");
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
	cpu_mod = g_module_open(path, G_MODULE_BIND_LAZY);
	if(!cpu_mod)
	{
		g_error("%s: invalid CPU file (%s)", path, g_module_error());
		return 0;
	}

	/* check if it's really a CPU */
	if(!g_module_symbol(cpu_mod, "dev_type", (gpointer*)&type))
		g_error("variable dev_type not defined in %s", path);
	if(strcmp(type, "cpu"))
		g_error("%s not a CPU file.", path);

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
	debug_item = gtk_check_menu_item_new_with_label(g_strdup_printf("%s (cpu)", emu_cpu_name));
	gtk_menu_shell_append(GTK_MENU_SHELL(debug_menu), debug_item);
	g_signal_connect(debug_item, "toggled", G_CALLBACK(cpu_show_hide), NULL);

	/* Create a new window, hidden */
	cpu_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(cpu_window), emu_cpu_name);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(cpu_window), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(cpu_window), 310, 435);
	g_signal_connect(cpu_window, "delete_event", G_CALLBACK(cpu_hide), debug_item);

	tips = gtk_tooltips_new();
	
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
	cpu_run_pause = gtk_button_new();
	{
		GtkWidget *alignment, *hbox;

		alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
		gtk_container_add(GTK_CONTAINER(cpu_run_pause), alignment);

		hbox = gtk_hbox_new(FALSE, 2);
		gtk_container_add(GTK_CONTAINER(alignment), hbox);

		run_image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_box_pack_start(GTK_BOX(hbox), run_image, FALSE, FALSE, 0);

		run_label = gtk_label_new_with_mnemonic("_Run");
		gtk_box_pack_start(GTK_BOX(hbox), run_label, FALSE, FALSE, 0);
	}
	gtk_tooltips_set_tip(tips, cpu_run_pause, "Run the microprocessor until it reaches a breakpoint", "");

	g_signal_connect_swapped(cpu_run_pause, "clicked",
			G_CALLBACK(cpu_run_pause_clicked), NULL);
	gtk_container_add(GTK_CONTAINER(toolitem[0]), cpu_run_pause);
	cpu_step = button_with_stock_image("_Step", GTK_STOCK_MEDIA_NEXT);
	gtk_tooltips_set_tip(tips, cpu_step, "Execute one instruction", "");
	g_signal_connect_swapped(cpu_step, "clicked",
			G_CALLBACK(cpu_step_clicked), NULL);
	gtk_container_add(GTK_CONTAINER(toolitem[1]), cpu_step);
	cpu_vblank = button_with_stock_image("Go to Next _Frame", GTK_STOCK_JUSTIFY_FILL);
	gtk_tooltips_set_tip(tips, cpu_vblank, "Run the microprocessor until it reaches a VBLANK", "");
	gtk_container_add(GTK_CONTAINER(toolitem[2]), cpu_vblank);

	gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

	/* Debugger list */
	store = gtk_list_store_new(N_COLUMNS, 
			G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_LONG);
	cpu_debugger = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(cpu_debugger), TRUE);
	for(i=0; i<4; i++)
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
			case CYCLES:
				gtk_tree_view_column_set_title(column, "Cycles");
				break;
		}
		gtk_tree_view_append_column(GTK_TREE_VIEW(cpu_debugger), column);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", i);
		gtk_tree_view_column_add_attribute(column, renderer, "cell-background", BG_COLOR);
	}
	g_signal_connect_swapped(cpu_debugger, "button_press_event",
			G_CALLBACK(cpu_debugger_clicked), NULL);

	/* Debugger view */
	hbox1 = gtk_hbox_new(FALSE, 0);
	vbox3 = gtk_vbox_new(FALSE, 6);

	hbox2 = gtk_hbox_new(FALSE, 6);
	reference_label = gtk_label_new("Reference");
	//gtk_misc_set_padding(GTK_MISC(reference_label), 6, 6);
	gtk_box_pack_start(GTK_BOX(hbox2), reference_label, FALSE, FALSE, 6);
	cpu_reference = gtk_entry_new();
	gtk_tooltips_set_tip(tips, cpu_reference, "Update the debugging window, so that the first instruction is the one that is typed in this entry.", "");
	gtk_entry_set_width_chars(GTK_ENTRY(cpu_reference), 8);
	gtk_box_pack_start(GTK_BOX(hbox2), cpu_reference, FALSE, FALSE, 0);
	g_signal_connect_swapped(cpu_reference, "activate",
			G_CALLBACK(cpu_reference_changed), NULL);
	gtk_box_pack_start(GTK_BOX(vbox3), hbox2, FALSE, FALSE, 0);

	scroll_debugger = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroll_debugger), cpu_debugger);
	gtk_box_pack_start(GTK_BOX(vbox3), scroll_debugger, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hbox1), vbox3, TRUE, TRUE, 0);

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
	g_signal_connect(bp_item, "button_press_event",
			G_CALLBACK(bp_item_clicked), GINT_TO_POINTER(FALSE));
	g_signal_connect(bp_once_item, "button_press_event",
			G_CALLBACK(bp_item_clicked), GINT_TO_POINTER(TRUE));
	gtk_widget_show_all(popup_menu);

	popup_menu_unset = gtk_menu_new();
	bp_item_unset = gtk_menu_item_new_with_label("Unset breakpoint");
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu_unset), bp_item_unset);
	g_signal_connect(bp_item_unset, "button_press_event",
			G_CALLBACK(bp_item_unset_clicked), NULL);
	gtk_widget_show_all(popup_menu_unset);

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

	emu_cpu_set_debugger_reference(0);
	ip = previous_ip = 0;
	update_debugger(TRUE);

	// gtk_window_present(GTK_WINDOW(cpu_window));

	cpu_loaded = TRUE;

	return CPU;
}
