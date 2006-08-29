#include <string.h>
#include "libemu.h"
#include "other.h"

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

GtkWidget* button_with_stock_image(gchar* mnemonic, gchar* stock)
{
	GtkWidget *button, *alignment, *hbox, *label, *image;

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
