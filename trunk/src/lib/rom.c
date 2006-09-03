#include <glib.h>
#include "libemu.h"
#include "other.h"

void emu_rom_set_load_callback(char* title, char* filter, int (*callback)(char* filename))
{
	GtkWidget *button;
	button = button_with_stock_image(title, "gtk-open", FALSE);
	gtk_box_pack_start_defaults(GTK_BOX(controls), button);
	gtk_widget_show(button);
}

int emu_rom_load(char* filename, long pos)
{
	GError *err;
	gchar *contents;
	gsize length;
	int i;

	if(!g_file_get_contents(filename, &contents, &length, &err))
	{
		emu_error(g_strdup_printf("ROM %s could not be loaded", filename));
		return 0;
	}

	// TODO - check length

	for(i=0; i<length; i++)
		emu_mem_set_direct(pos+i, contents[i]);

	emu_message(g_strdup_printf("ROM %s loaded in 0x%X, with %d bytes.", filename, pos, length));

	if(emu_cpu_get_debugger_reference() == -1)
		emu_cpu_set_debugger_reference(pos);
	else
		emu_cpu_set_debugger_reference(emu_cpu_get_debugger_reference());
	emu_mem_set_reference(0);
	
	return -1;
}
