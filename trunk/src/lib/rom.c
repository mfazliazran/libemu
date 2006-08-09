#include <glib.h>
#include "libemu.h"

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

	/* TODO - check length */

	for(i=0; i<length; i++)
		emu_mem_set_direct(pos+i, contents[i]);

	emu_message(g_strdup_printf("ROM %s loaded in 0x%X.", filename, pos));
	
	if(emu_cpu_get_debugger_reference() == -1)
		emu_cpu_set_debugger_reference(pos);
	else
		emu_cpu_set_debugger_reference(emu_cpu_get_debugger_reference());
	
	return -1;
}
