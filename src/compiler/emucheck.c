#include <glib.h>
#include <gmodule.h>
#include <stdio.h>
#include <string.h>

typedef enum { CPU, VIDEO, GENERIC } DEV_TYPE;

void error(const char* str)
{
	fprintf(stderr, "emucheck: error: %s\n", str);
	exit(-1);
}

void gm_error(const char* str)
{
	fprintf(stderr, "emucheck: error: %s (%s)\n", str, g_module_error());
	exit(-1);
}

void warning(const char* str)
{
	fprintf(stderr, "emuchek: warning: %s\n", str);
}

void gm_warning(const char* str)
{
	fprintf(stderr, "emuchek: warning: %s (%s)\n", str, g_module_error());
}

int main(int argc, char** argv)
{
	GModule* mod;
	gpointer sym;
	gchar* type;
	gchar* path;
	
	/* check for arguments */
	if(argc != 2)
	{
		fprintf(stderr, "Usage: emucheck FILE\n");
		return 1;
	}
	
	/* check for module support */
	if(!g_module_supported())
		error("no modular support on this operating system");

	/* create path (g_module_open requires a full path) */
	if(argv[1][0] == '.' || argv[1][0] == '/' || argv[1][0] == '~')
		path = g_strdup_printf("%s", argv[1]);
	else
		path = g_strdup_printf("./%s", argv[1]);

	/* opens the module */
	mod = g_module_open(path, G_MODULE_BIND_LAZY);
	if(mod == NULL)
		gm_error("invalid file");

	/* discovers the device type */
	if(!g_module_symbol(mod, "dev_type", (gpointer*)&type))
		gm_error("variable type is not defined");
	if(!strcmp(type, "cpu"))
	{
		/* check for CPU required functions */
		if(!g_module_symbol(mod, "dev_cpu_name", &sym))
			gm_error("variable dev_cpu_name is not defined");
		if(!g_module_symbol(mod, "dev_cpu_reset", &sym))
			gm_error("function dev_cpu_reset is not implemented");
		if(!g_module_symbol(mod, "dev_cpu_step", &sym))
			gm_error("function dev_cpu_step is not implemented");
		
		/* check for CPU recommended functions */
		if(!g_module_symbol(mod, "dev_cpu_register_name", &sym))
			gm_warning("function dev_cpu_register_name is not implemented");
		if(!g_module_symbol(mod, "dev_cpu_register_value", &sym))
			gm_warning("function dev_cpu_register_value is not implemented");
		if(!g_module_symbol(mod, "dev_cpu_flag_name", &sym))
			gm_warning("function dev_cpu_flag_name is not implemented");
		if(!g_module_symbol(mod, "dev_cpu_debug", &sym))
			gm_warning("function dev_cpu_debug is not implemented");
		if(!g_module_symbol(mod, "dev_cpu_ip", &sym))
			gm_warning("function dev_cpu_ip is not implemented");
	}
	else if(!strcmp(type, "generic"))
	{
		if(!g_module_symbol(mod, "dev_generic_reset", &sym))
			gm_error("variable dev_generic_reset is not defined");
		if(!g_module_symbol(mod, "dev_generic_memory_set", &sym))
			gm_error("variable dev_generic_memory_set is not defined");
		if(!g_module_symbol(mod, "dev_generic_debug_name", &sym))
			gm_error("variable dev_generic_debug_name is not defined");
		if(!g_module_symbol(mod, "dev_generic_debug", &sym))
			gm_error("variable dev_generic_debug is not defined");
	}
	else
		error("the device type (type) is not one of: cpu, video or generic");
}
