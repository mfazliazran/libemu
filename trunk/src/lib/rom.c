#include <glib.h>
#include <sys/stat.h>
#include "libemu.h"
#include "other.h"

typedef struct
{
	char* title;
	char* filter;
	void* callback;
} DIALOG;

/*
 * EVENT HANDLERS
 */
static void load_rom_clicked(GtkButton *button, gpointer data)
{
	GtkWidget *dialog;
	GtkFileFilter *filter1, *filter2;
	DIALOG *dl = data;

	void (*callback)(char* file);

	filter1 = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter1, dl->filter);
	gtk_file_filter_set_name(filter1, "ROM files");

	filter2 = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter2, "*.*");
	gtk_file_filter_set_name(filter2, "All files");

	dialog = gtk_file_chooser_dialog_new(dl->title, GTK_WINDOW(window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter1);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter2);
	
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		callback = dl->callback;
		callback(filename);
	}
	gtk_widget_destroy(dialog);
}

/*
 * API
 */

void emu_rom_set_load_callback(char* title, char* filter, void (*callback)(char* filename))
{
	GtkWidget *button;
	DIALOG *dl = malloc(sizeof(DIALOG));
	button = button_with_stock_image(title, "gtk-open", FALSE);
	gtk_box_pack_start_defaults(GTK_BOX(controls), button);
	dl->title = title;
	dl->filter = filter;
	dl->callback = (void*)callback;
	g_signal_connect(button, "clicked", G_CALLBACK(load_rom_clicked), (gpointer)dl);
	gtk_widget_show(button);
}

long emu_rom_size_k(char* filename)
{
	return emu_rom_size(filename) / 1024;
}

long emu_rom_size(char* filename)
{
	struct stat i;
	stat(filename, &i);
	return i.st_size;
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
