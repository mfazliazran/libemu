#include <glib.h>
#include "libemu.h"

void emu_rom_load(char* filename, long pos)
{
	GError *err;
	gchar *contents;
	gsize length;
	int i;

	g_file_get_contents(filename, &contents, &length, &err);
	for(i=0; i<length; i++)
		emu_mem_set(pos+i, contents[i]);
}
