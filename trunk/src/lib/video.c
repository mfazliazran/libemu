#include "SDL.h"
#include <string.h>
#include "libemu.h"
#include "other.h"

#define MAX_REGISTERS 255
#define MAX_VERTICAL 12

static GtkWidget* video_window;
static GtkWidget* video_register[MAX_REGISTERS];
static gint num_registers;
static gboolean video_loaded = FALSE;
static SDL_Color *color;
static SDL_Surface *screen, *buffer;
static gint number_of_colors;
static gdouble fps;
static GTimer *timer;
static gint frame_count = 0;
static double time_busy = 0.0f;

/* TODO - check when SDL screen is asked to close */

/*
 * EVENT HANDLERS
 */

/* When the monitor item is clicked on the main window */
static void monitor_show_hide(GtkToggleButton *item, gpointer data)
{
	// GtkWindow* window = GTK_WINDOW(data);
	if(gtk_toggle_button_get_active(item))
	{
		SDL_InitSubSystem(SDL_INIT_VIDEO);
		screen = SDL_SetVideoMode(*emu_video_pixels_x, *emu_video_pixels_y, 8,
				SDL_HWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF);
		if(!screen)
			g_error("A SDL_Screen could not be created (%s)", SDL_GetError());
		SDL_WM_SetCaption("Monitor", "Monitor");
		emu_video_update_screen();
	}
	else
	{
		SDL_FreeSurface(screen);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
}

/* When the video menu item is clicked on the main window */
static void video_show_hide(GtkToggleButton *item, gpointer data)
{
	GtkWindow* window = GTK_WINDOW(data);
	if(gtk_toggle_button_get_active(item))
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
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), FALSE);
	gtk_widget_hide(widget);
	return TRUE;
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
	color = g_malloc(sizeof(SDL_Color) * n_colors);
	number_of_colors = n_colors;
}

/* Set a color on the color palette */
void emu_video_palette_set_color(int n_color, int r, int g, int b)
{
	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return;
	SDL_Color *c = g_malloc(sizeof(SDL_Color));
	if(n_color > number_of_colors)
		emu_error("Color number higher than number of colors in the palette!");
	c->r = r;
	c->g = g;
	c->b = b;
	SDL_SetColors(buffer, c, n_color, 1);
}

/* Draws one pixel in the screen */
void emu_video_draw_pixel(int x, int y, long palette_color)
{
	Uint8 *p;

	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return;
	if(y < 0 || y >= *emu_video_pixels_y)
		return;

	SDL_LockSurface(buffer);
	p = (Uint8*)buffer->pixels + y * buffer->pitch;
	p[x] = palette_color;
	SDL_UnlockSurface(buffer);
}

/* Draw a horizontal line on the screen */
void emu_video_draw_hline(int x1, int x2, int y, long palette_color)
{
	Uint8 *p;
	int x;

	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return;
	if(y < 0 || y >= *emu_video_pixels_y)
		return;

	SDL_LockSurface(buffer);
	p = (Uint8*)buffer->pixels + y * buffer->pitch;
	for(x=x1; x<x2; x++)
		p[x] = palette_color;
	SDL_UnlockSurface(buffer);
}

/* Updates the TV screen */
void emu_video_update_screen()
{
	if(!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	/* halt until the time for this frame has passed */
	SDL_BlitSurface(buffer, NULL, screen, NULL);
	time_busy += g_timer_elapsed(timer, NULL);
	while(g_timer_elapsed(timer, NULL) < (gdouble)(1/fps));
	g_timer_start(timer);
	SDL_Flip(screen);

	frame_count++;
	if(frame_count >= fps)
	{
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(usage_bar),
				time_busy);
		frame_count = 0;
		time_busy = 0.0f;
	}

	/* draw the frame on the screen */
}

/* Create a new video device, and return its number */
int emu_video_init(char* filename, double video_cycles_per_cpu_cycle, int frames_per_second)
{
	GModule *video_mod;
	gchar *path, *type;
	GtkWidget *debug_item, *monitor;
	GtkWidget *table, *label;
	gint row, col, i;
	SYNC_TYPE* sync;
	SDL_VideoInfo *info;

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
	// *emu_video_pos_x = *emu_video_pos_y = 0;
	if(!g_module_symbol(video_mod, "dev_video_wait_vsync", (gpointer*)&emu_video_wait_vsync))
		g_error("variable dev_video_wait_vsync is not defined");
	if(!g_module_symbol(video_mod, "dev_video_wait_hsync", (gpointer*)&emu_video_wait_hsync))
		g_error("variable dev_video_wait_hsync is not defined");
	// *emu_video_wait_vsync = *emu_video_wait_hsync = 0;

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
	debug_item = button_with_pixmap_image(emu_video_name, P_VIDEO, TRUE);
	gtk_box_pack_start_defaults(GTK_BOX(internal_hbox), debug_item);
	monitor = button_with_pixmap_image("Monitor", P_MONITOR, TRUE);
	gtk_box_pack_start_defaults(GTK_BOX(external_hbox), monitor);

	/* Create window */
	video_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(video_window), emu_video_name);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(video_window), TRUE);
	g_signal_connect(debug_item, "toggled", G_CALLBACK(video_show_hide), video_window);
	g_signal_connect(monitor, "toggled", G_CALLBACK(monitor_show_hide), NULL);
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

	/* Initialize video (SDL) */
	if(SDL_Init(SDL_INIT_VIDEO) == -1)
		g_error("SDL could not be initialized (%s)", SDL_GetError());

	/* Tests video quality */
	info = SDL_GetVideoInfo();
	if(!info->hw_available)
		g_warning("It's not possible to create hardware surfaces in this video card.");
	if(!info->blit_hw)
		g_warning("Hardware to hardware blits are not accelerated in this video card.");

	buffer = SDL_CreateRGBSurface(SDL_HWSURFACE, *emu_video_pixels_x,
			*emu_video_pixels_y, 8, 0, 0, 0, 0);

	fps = frames_per_second;
	timer = g_timer_new();

	emu_video_reset();

	gtk_container_add(GTK_CONTAINER(video_window), table);
	gtk_widget_show_all(table);

	return VIDEO;
}
