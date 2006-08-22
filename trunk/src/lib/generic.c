#include <string.h>
#include "libemu.h"
#include "other.h"

/* Create a new generic device, and return its number */
int emu_generic_init(char* filename)
{
	GModule *generic_mod;
	gchar *path, *type;
	GtkWidget *debug_item;

	if(generic_count >= MAX_GENERIC)
	{
		g_error("There is a limit of %d devices.", generic_count);
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
	generic_mod = g_module_open(path, G_MODULE_BIND_LAZY);
	if(!generic_mod)
	{
		g_error("%s: invalid Generic Device file (%s)", path, g_module_error());
		return 0;
	}

	/* check if it's really a CPU */
	if(!g_module_symbol(generic_mod, "dev_type", (gpointer*)&type))
		g_error("variable dev_type not defined in %s", path);
	if(strcmp(type, "generic"))
		g_error("%s not a Generic Device file.", path);

	/* Connect functions */
	if(!g_module_symbol(generic_mod, "dev_generic_name", (gpointer*)&emu_generic_name[generic_count]))
		g_error("variable dev_generic_name not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_reset", (void*)&emu_generic_reset[generic_count]))
		g_error("variable dev_generic_reset not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_memory_set", (void*)&emu_generic_memory_set[generic_count]))
		g_error("variable dev_generic_memory_set not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_debug_name", (void*)&emu_generic_debug_name[generic_count]))
		g_error("variable dev_generic_debug_name not defined in %s", path);
	if(!g_module_symbol(generic_mod, "dev_generic_debug", (void*)&emu_generic_debug[generic_count]))
		g_error("variable dev_generic_debug not defined in %s", path);

	/* Connect callbacks */
	if(!connect_callbacks(generic_mod))
		g_error("Generic Device callbacks couldn't be connected in %s", path);

	g_message("Generic Device %s loaded from %s", emu_generic_name, path);
	
	/* Add a new menu option */
	debug_item = gtk_check_menu_item_new_with_label(g_strdup_printf("%s (generic)", emu_generic_name));
	gtk_menu_shell_append(GTK_MENU_SHELL(debug_menu), debug_item);
	// g_signal_connect(debug_item, "toggled", G_CALLBACK(generic_show_hide), NULL);



	generic_count++;
	return generic_count - 1;
}
