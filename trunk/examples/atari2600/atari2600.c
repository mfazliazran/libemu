#include <libemu.h>
#include <stdlib.h>

#define SWCHA  0x280
#define INPT4  0xc

void joystick_event(KEYEVENT_TYPE evt_type, int joynumber, JOYBUTTON button)
{
	if(joynumber != 0)
		return;

	if(evt_type == PRESSED)
	{
		switch(button)
		{
			case UP:    emu_mem_set(SWCHA, emu_mem_get(SWCHA) ^ 0x10); break;
			case DOWN:  emu_mem_set(SWCHA, emu_mem_get(SWCHA) ^ 0x20); break;
			case LEFT:  emu_mem_set(SWCHA, emu_mem_get(SWCHA) ^ 0x40); break;
			case RIGHT: emu_mem_set(SWCHA, emu_mem_get(SWCHA) ^ 0x80); break;
			case B0:    emu_mem_set(INPT4, emu_mem_get(INPT4) ^ 0x80); break;
		}
	}
	else
	{
		switch(button)
		{
			case UP:    emu_mem_set(SWCHA, emu_mem_get(SWCHA) | 0x10); break;
			case DOWN:  emu_mem_set(SWCHA, emu_mem_get(SWCHA) | 0x20); break;
			case LEFT:  emu_mem_set(SWCHA, emu_mem_get(SWCHA) | 0x40); break;
			case RIGHT: emu_mem_set(SWCHA, emu_mem_get(SWCHA) | 0x80); break;
			case B0:    emu_mem_set(INPT4, emu_mem_get(INPT4) | 0x80); break;
		}
	}
}

void load_rom(char* filename)
{
	switch(emu_rom_size_k(filename))
	{
		case 2:
			emu_rom_load(filename, 0xf000);
			emu_rom_load(filename, 0xf800);
			break;
		case 4:
			emu_rom_load(filename, 0xf000);
			break;
		default:
			emu_error("Invalid ROM size");
	}
}

int main(int argc, char** argv)
{
	int tia, pia;

	/* Initialize libemu */
	emu_init("Atari 2600", argc, argv);
	
	/* Initialize components */
	emu_mem_init_k(64);
	emu_cpu_init("6502");
	tia = emu_video_init("tia", 3, 60);
	pia = emu_generic_init("pia", 1);
	emu_mem_map_add(tia, 0x0, 0x7f);
	emu_mem_map_add(pia, 0x280, 0x297);

	/* Load ROM */
	// emu_rom_load("rom/simple.bin", 0xF000);
	emu_rom_set_load_callback("Load ROM", "*.bin", load_rom);

	/* Initialize joystick */
	emu_joystick_init(joystick_event);

	/* Start emulation */
	emu_cpu_reset();
	emu_main();

	return 0;
}
