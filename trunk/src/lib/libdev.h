#ifndef _LIBEMU_DEV_H_
#define _LIBEMU_DEV_H_

#ifdef _WIN32
#	define EXPORT __declspec(dllexport)
#else
#	define EXPORT
#endif

typedef enum {
	EXACT_SYNC = 0,
	HORIZONTAL_SYNC,
	VERTICAL_SYNC
} SYNC_TYPE;

/* General */
void (*dev_message)(char* message);

/* Memory */
unsigned long int (*dev_mem_size)();
void (*dev_mem_set_direct)(unsigned long int pos, unsigned char data);
void (*dev_mem_set)(unsigned long int pos, unsigned char data);
unsigned char (*dev_mem_get)(unsigned long int pos);

/* Video */
void (*dev_video_update_screen)();
void (*dev_video_create_palette)(int n_colors);
void (*dev_video_palette_set_color)(int n_color, int r, int g, int b);
void (*dev_video_draw_pixel)(int x, int y, long palette_color);
void (*dev_video_draw_hline)(int x1, int x2, int y, long palette_color);

/* Callbacks */
EXPORT void set_callbacks(
	unsigned long int (*dev_mem_size_ptr)(),
	void (*dev_message_ptr)(char*),
	void (*dev_mem_set_direct_ptr)(unsigned long int, unsigned char),
	void (*dev_mem_set_ptr)(unsigned long int, unsigned char),
	unsigned char (*dev_mem_get_ptr)(unsigned long int)
) 
{
	dev_mem_size       = dev_mem_size_ptr;
	dev_message        = dev_message_ptr;
	dev_mem_set_direct = dev_mem_set_direct_ptr;
	dev_mem_set        = dev_mem_set_ptr;
	dev_mem_get        = dev_mem_get_ptr;
}

EXPORT void set_video_callbacks(
	void (*dev_video_update_screen_ptr)(),
	void (*dev_video_create_palette_ptr)(int),
	void (*dev_video_palette_set_color_ptr)(int, int, int, int),
	void (*dev_video_draw_pixel_ptr)(int, int, long),
	void (*dev_video_draw_hline_ptr)(int, int, int, long)
)
{
	dev_video_update_screen = dev_video_update_screen_ptr;
	dev_video_create_palette = dev_video_create_palette_ptr;
	dev_video_palette_set_color = dev_video_palette_set_color_ptr;
	dev_video_draw_pixel = dev_video_draw_pixel_ptr;
	dev_video_draw_hline = dev_video_draw_hline_ptr;
}

#endif
