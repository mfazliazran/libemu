#include <string.h>
#include <gdk/gdk.h>
#include "libemu.h"
#include "other.h"

#define MAX_REGISTERS 255
#define MAX_VERTICAL 12

static GtkWidget* video_window;
static GtkWidget* video_register[MAX_REGISTERS];
static gint num_registers;
static gboolean video_loaded = FALSE;
static GdkColor *color, *current_color;
static gint number_of_colors;

/*
 * EVENT HANDLERS
 */

/* When the CPU menu item is clicked on the main window */
static void video_show_hide(GtkCheckMenuItem *item, gpointer data)
{
	GtkWindow* window = GTK_WINDOW(data);
	if(item->active)
	{
		gtk_window_present(window);
		video_update();
	}
	else
		gtk_widget_hide(GTK_WIDGET(data));
}

/* When the close button is clicked on the debugger */
static gboolean video_hide(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(data), FALSE);
	gtk_widget_hide(widget);
	return TRUE;
}

/* Updates the sceen when the application in exposed */
static void screen_expose(GtkWidget *widget, GdkEventExpose *event, gpointer userdata)
{
	gdk_draw_drawable(screen->window, 
		screen->style->fg_gc[GTK_WIDGET_STATE(screen)],
		buffer, 
		event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);
}

/*
 * PUBLIC FUNCTIONS
 */

/* Update the debugger on the screen */
void video_update()
{
	int i;
	if(GTK_WIDGET_VISIBLE(video_window))
		for(i=0; i<num_registers; i++)
			gtk_entry_set_text(GTK_ENTRY(video_register[i]), emu_video_debug(i));
}

/*
 * API
 */

/* Create a new color palette */
void emu_video_create_palette(int n_colors)
{
	color = g_malloc(sizeof(GdkColor) * n_colors);
	number_of_colors = n_colors;
}

/* Set a color on the color palette */
void emu_video_palette_set_color(int n_color, int r, int g, int b)
{
	if(n_color > number_of_colors)
		emu_error("Color number higher than number of colors in the palette!");
	color[n_color].red   = r * 256;
	color[n_color].green = g * 256;
	color[n_color].blue  = b * 256;
}

/* Draws one pixel in the screen */
void emu_video_draw_pixel(int x, int y, long palette_color)
{
	if(&color[palette_color] != current_color)
	{
		gdk_gc_set_rgb_fg_color(gc, &color[palette_color]);
		current_color = &color[palette_color];
	}
	gdk_draw_point(GDK_DRAWABLE(buffer), gc, x, y);
}

/* Draw a horizontal line on the screen */
void emu_video_draw_hline(int x1, int x2, int y, long palette_color)
{
	if(&color[palette_color] != current_color)
	{
		gdk_gc_set_rgb_fg_color(gc, &color[palette_color]);
		current_color = &color[palette_color];
	}
	gdk_draw_line(GDK_DRAWABLE(buffer), gc, x1, y, x2, y);
}

/* Updates the TV screen */
void emu_video_update_screen()
{
	GdkRectangle rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = *emu_video_pixels_x;
	rect.height = *emu_video_pixels_y;
	gdk_draw_drawable(screen->window, 
			screen->style->fg_gc[GTK_WIDGET_STATE(screen)],
			buffer, 0, 0, 0, 0, -1, -1);
	gdk_window_invalidate_rect(screen->window, &rect, FALSE);
	gdk_window_process_updates(screen->window, FALSE);
}

/* Create a new video device, and return its number */
int emu_video_init(char* filename, double video_cycles_per_cpu_cycle)
{
	GModule *video_mod;
	gchar *path, *type;
	GtkWidget *debug_item;
	GtkWidget *table, *label;
	gint row, col, i;
	SYNC_TYPE* sync;

	if(video_loaded)
	{
		g_error("A video card was already loaded.");
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
	video_mod = g_module_open(path, G_MODULE_BIND_LAZY);
	if(!video_mod)
	{
		g_error("%s: invalid Video file (%s)", path, g_module_error());
		return 0;
	}

	/* check if it's really a video */
	if(!g_module_symbol(video_mod, "dev_type", (gpointer*)&type))
		g_error("variable dev_type not defined in %s", path);
	if(strcmp(type, "video"))
		g_error("%s not a Video file.", path);

	/* Connect functions */
	if(!g_module_symbol(video_mod, "dev_video_name", (gpointer*)&emu_video_name))
		g_error("variable dev_video_name not defined in %s", path);
	if(!g_module_symbol(video_mod, "dev_video_sync_type", (gpointer*)&sync))
		g_error("variable dev_video_sync_type is not defined");
	emu_video_sync = *sync;
	emu_video_cycles = video_cycles_per_cpu_cycle;

	if(!g_module_symbol(video_mod, "dev_video_scanline_cycles", (gpointer*)&emu_video_scanline_cycles))
		g_error("variable dev_video_scanline_cycles is not defined");
	if(!g_module_symbol(video_mod, "dev_video_scanlines_vblank", (gpointer*)&emu_video_scanlines_vblank))
		g_error("variable dev_video_scanlines_vblank is not defined");
	if(!g_module_symbol(video_mod, "dev_video_scanlines_overscan", (gpointer*)&emu_video_scanlines_overscan))
		g_error("variable dev_video_scanlines_overscan is not defined");
	if(!g_module_symbol(video_mod, "dev_video_pos_x", (gpointer*)&emu_video_pos_x))
		g_error("variable dev_video_pos_x is not defined");
	if(!g_module_symbol(video_mod, "dev_video_pos_y", (gpointer*)&emu_video_pos_y))
		g_error("variable dev_video_pos_y is not defined");
	*emu_video_pos_x = *emu_video_pos_y = 0;
	if(!g_module_symbol(video_mod, "dev_video_wait_vsync", (gpointer*)&emu_video_wait_vsync))
		g_error("variable dev_video_wait_vsync is not defined");
	if(!g_module_symbol(video_mod, "dev_video_wait_hsync", (gpointer*)&emu_video_wait_hsync))
		g_error("variable dev_video_wait_hsync is not defined");
	*emu_video_wait_vsync = *emu_video_wait_hsync = 0;

	if(!g_module_symbol(video_mod, "dev_video_pixels_x", (gpointer*)&emu_video_pixels_x))
		g_error("variable dev_video_pixels_x is not defined");
	if(!g_module_symbol(video_mod, "dev_video_pixels_y", (gpointer*)&emu_video_pixels_y))
		g_error("variable dev_video_pixels_y is not defined");
	if(!g_module_symbol(video_mod, "dev_video_reset", (void*)&emu_video_reset))
		g_error("variable dev_video_reset not defined in %s", path);
	if(!g_module_symbol(video_mod, "dev_video_step", (void*)&emu_video_step))
		g_error("variable dev_video_step not defined in %s", path);
	if(!g_module_symbol(video_mod, "dev_video_memory_set", (void*)&emu_video_memory_set))
		g_error("variable dev_video_memory_set not defined in %s", path);
	if(!g_module_symbol(video_mod, "dev_video_debug_name", (void*)&emu_video_debug_name))
		g_error("variable dev_video_debug_name not defined in %s", path);
	if(!g_module_symbol(video_mod, "dev_video_debug", (void*)&emu_video_debug))
		g_error("variable dev_video_debug not defined in %s", path);

	/* Connect callbacks */
	if(!connect_callbacks(video_mod))
		g_error("Video callbacks couldn't be connected in %s", path);
	if(!connect_video_callbacks(video_mod))
		g_error("Video specific callbacks couldn't be connected in %s", path);


	g_message("Video %s loaded from %s", emu_video_name, path);
	
	/* Add a new menu option */
	debug_item = gtk_check_menu_item_new_with_label(g_strdup_printf("%s (video)", emu_video_name));
	gtk_menu_shell_append(GTK_MENU_SHELL(debug_menu), debug_item);

	/* Create window */
	video_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(video_window), emu_video_name);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(video_window), TRUE);
	g_signal_connect(debug_item, "toggled", G_CALLBACK(video_show_hide), video_window);
	g_signal_connect(video_window, "delete_event", G_CALLBACK(video_hide), debug_item);

	/* Create table */
	table = gtk_table_new(1, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 12);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	/* Add registers to table */
	row = 0; col = 0; i = 0 ;
	while(emu_video_debug_name(i))
	{
		gtk_table_attach(GTK_TABLE(table),
				gtk_label_new(emu_video_debug_name(i)),
				col, col+1, row, row+1,
				GTK_SHRINK, GTK_SHRINK, 0, 0);
		video_register[i] = gtk_entry_new();
		gtk_entry_set_width_chars(GTK_ENTRY(video_register[i]), 4);
		gtk_table_attach_defaults(GTK_TABLE(table),
				video_register[i],
				col+1, col+2, row, row+1);

		row++;
		if(row >= MAX_VERTICAL)
		{
			row = 0;
			col += 2;
		}
		i++;
	}
	num_registers = i;

	g_signal_connect(screen, "expose_event", G_CALLBACK(screen_expose), NULL);

	emu_video_reset();

	gtk_container_add(GTK_CONTAINER(video_window), table);
	gtk_widget_show_all(table);

	return VIDEO;
}
