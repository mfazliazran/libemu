#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include "libemu.h"
#include "other.h"
#include "../../pixmaps/pixmaps.c"

int connect_callbacks(GModule* mod)
{
	void (*set_callbacks)(
		unsigned long (*dev_mem_size_ptr)(),
		void (*dev_message_ptr)(char*),
		void (*dev_mem_set_direct_ptr)(unsigned long int, unsigned char),
		void (*dev_mem_set_ptr)(unsigned long int, unsigned char),
		unsigned char (*dev_mem_get_ptr)(unsigned long int)
	);

	if(!g_module_symbol(mod, "set_callbacks", (void*)&set_callbacks))
		return 0;

	set_callbacks(
			(void*)&emu_mem_size,
			(void*)&emu_message,
			(void*)&emu_mem_set_direct,
			(void*)&emu_mem_set,
			(void*)&emu_mem_get
		     );

	return 1;
}

int connect_video_callbacks(GModule* mod)
{
	void (*set_video_callbacks)(
		void (*dev_video_update_screen_ptr)(),
		void (*dev_video_create_palette_ptr)(int),
		void (*dev_video_palette_set_color_ptr)(int, int, int, int),
		void (*dev_video_draw_pixel_ptr)(int, int, long),
		void (*dev_video_draw_hline_ptr)(int, int, int, long)
	);

	if(!g_module_symbol(mod, "set_video_callbacks", (void*)&set_video_callbacks))
		return 0;

	set_video_callbacks(
				(void*)&emu_video_update_screen,
				(void*)&emu_video_create_palette,
				(void*)&emu_video_palette_set_color,
				(void*)&emu_video_draw_pixel,
				(void*)&emu_video_draw_hline
			   );

	return 1;
}

GtkWidget* button_with_stock_image(gchar* mnemonic, gchar* stock, gboolean toggle)
{
	GtkWidget *button, *alignment, *hbox, *label, *image;

	if(toggle)
		button = gtk_toggle_button_new();
	else
		button = gtk_button_new();
	
	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(button), alignment);

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(alignment), hbox);

	image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic(mnemonic);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all(alignment);

	return button;
}

GtkWidget* button_with_pixmap_image(gchar* mnemonic, gint image_number, gboolean toggle)
{
	GtkWidget *button, *alignment, *hbox, *label, *image;
	GdkPixbuf *pixbuf;

	if(toggle)
		button = gtk_toggle_button_new();
	else
		button = gtk_button_new();
	
	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_container_add(GTK_CONTAINER(button), alignment);

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_container_add(GTK_CONTAINER(alignment), hbox);

	// image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_SMALL_TOOLBAR);
	switch(image_number)
	{
		case P_CPU:
			pixbuf = gdk_pixbuf_new_from_inline(-1, cpu_pixmap, FALSE, NULL);
			break;
		case P_VIDEO:
			pixbuf = gdk_pixbuf_new_from_inline(-1, video_pixmap, FALSE, NULL);
			break;
		case P_DEVICE:
			pixbuf = gdk_pixbuf_new_from_inline(-1, device_pixmap, FALSE, NULL);
			break;
		case P_MEMORY:
			pixbuf = gdk_pixbuf_new_from_inline(-1, memory_pixmap, FALSE, NULL);
			break;
		case P_MONITOR:
			pixbuf = gdk_pixbuf_new_from_inline(-1, monitor_pixmap, FALSE, NULL);
			break;
		case P_JOYSTICK:
			pixbuf = gdk_pixbuf_new_from_inline(-1, joystick_pixmap, FALSE, NULL);
			break;
		default:
			g_error("Invalid pixmap");
	}
	image = gtk_image_new_from_pixbuf(pixbuf);

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic(mnemonic);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all(alignment);

	return button;
}

unsigned long hex2long(char* hex)
{
	int size = strlen(hex);
	int i, factor = 1;
	long val = 0;
	if(size == 0)
		return -1;
	for(i=(size-1); i>=0; i--)
	{
		switch(hex[i])
		{
			case '0': break;
			case '1': val += 1 * factor; break;
			case '2': val += 2 * factor; break;
			case '3': val += 3 * factor; break;
			case '4': val += 4 * factor; break;
			case '5': val += 5 * factor; break;
			case '6': val += 6 * factor; break;
			case '7': val += 7 * factor; break;
			case '8': val += 8 * factor; break;
			case '9': val += 9 * factor; break;
			case 'A': case 'a': val += 10 * factor; break;
			case 'B': case 'b': val += 11 * factor; break;
			case 'C': case 'c': val += 12 * factor; break;
			case 'D': case 'd': val += 13 * factor; break;
			case 'E': case 'e': val += 14 * factor; break;
			case 'F': case 'f': val += 15 * factor; break;
			default: return -1;
		}
		factor *= 16;
	}
	return val;
}
