#include "libemu.h"
#include "other.h"

int connect_callbacks(GModule* mod)
{
	void (*set_callbacks)(
		void (*dev_message_ptr)(char*),
		void (*dev_mem_set_direct_ptr)(unsigned long int, unsigned char),
		void (*dev_mem_set_ptr)(unsigned long int, unsigned char),
		unsigned char (*dev_mem_get_ptr)(unsigned long int)
	);

	if(!g_module_symbol(mod, "set_callbacks", (void*)&set_callbacks))
		return 0;

	set_callbacks(
			(void*)&emu_message,
			(void*)&emu_mem_set_direct,
			(void*)&emu_mem_set,
			(void*)&emu_mem_get
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

	image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic(mnemonic);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all(alignment);

	return button;
}
