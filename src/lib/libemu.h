#ifndef _LIBEMU_H_
#define _LIBEMU_H_

#include "libdefs.h"

#define MAX_GENERIC  255
#define MAX_JOYSTICK 2

#define CPU   -1
#define VIDEO -2

/* libemu API */

/* General functions */
void emu_init(const char* name, int argc, char** argv);
void emu_main();
void emu_message(char* message);
void emu_error(char* message);

/* CPU functions */
int emu_cpu_init(char* filename);
void emu_cpu_run();
void emu_cpu_pause();
void emu_cpu_set_breakpoint(unsigned long int pos, int one_time_only);
void emu_cpu_unset_breakpoint(unsigned long int pos);
unsigned long emu_cpu_get_debugger_reference();
void emu_cpu_set_debugger_reference(unsigned long initial_pos);
char* emu_cpu_name;
char* (*emu_cpu_register_name)(int n);
unsigned int (*emu_cpu_register_value)(int n);
char* (*emu_cpu_flag_name)(int n);
unsigned char (*emu_cpu_flag_value)(int n);
char* (*emu_cpu_debug)(unsigned long addr, int* num_cycles, int* bytes);
void (*emu_cpu_reset)();
unsigned long int (*emu_cpu_ip)();
int (*emu_cpu_step)(int* cycles);

/* Video functions */
int emu_video_init(char* filename, double video_cycles_per_cpu_cycle, int frames_per_second);
char* emu_video_name;
int *emu_video_pixels_x;
int *emu_video_pixels_y;
int *emu_video_scanline_cycles;
int *emu_video_scanlines_vblank;
int *emu_video_scanlines_overscan;
int *emu_video_pos_x;
int *emu_video_pos_y;
int *emu_video_wait_vsync;
int *emu_video_wait_hsync;
void emu_video_update_screen();
void emu_video_create_palette(int n_colors);
void emu_video_palette_set_color(int n_color, int r, int g, int b);
void emu_video_draw_pixel(int x, int y, long palette_color);
void emu_video_draw_hline(int x1, int x2, int y, long palette_color);
void (*emu_video_reset)();
void (*emu_video_step)(int cycles);
int (*emu_video_memory_set)(unsigned long int pos, unsigned char data);
char* (*emu_video_debug_name)(int n);
char* (*emu_video_debug)(int n);

/* Generic device functions */
int emu_generic_init(char* filename, double device_cycles_per_cpu_cycle);
char* emu_generic_name[MAX_GENERIC];
void (*emu_generic_reset[MAX_GENERIC])();
void (*emu_generic_step[MAX_GENERIC])(int cycles);
int (*emu_generic_memory_set[MAX_GENERIC])(unsigned long int pos, unsigned char data);
char* (*emu_generic_debug_name[MAX_GENERIC])(int n);
char* (*emu_generic_debug[MAX_GENERIC])(int n);

/* libemu RAM API */
unsigned long int emu_mem_size();
void emu_mem_init(unsigned long sz);
void emu_mem_init_k(unsigned int sz);
void emu_mem_set_direct(unsigned long int pos, unsigned char data);
void emu_mem_set(unsigned long int pos, unsigned char data);
unsigned char emu_mem_get(unsigned long int pos);
unsigned long emu_mem_get_reference();
void emu_mem_set_reference(unsigned long initial_pos);
int emu_mem_map_add(int device, unsigned long int initial, unsigned long int final);

/* libemu ROM API */
int emu_rom_load(char* filename, long pos);
long emu_rom_size(char* filename);
long emu_rom_size_k(char* filename);
void emu_rom_set_load_callback(char* title, char* filter, void (*callback)(char* filename));

/* libemu joystick API */
int emu_joystick_init(void (*callback)(KEYEVENT_TYPE event_type, JOYBUTTON joybutton));

#endif
