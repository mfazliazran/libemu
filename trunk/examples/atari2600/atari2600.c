#include <libemu.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int tia, pia;

	/* Initialize libemu */
	emu_init("Atari 2600", argc, argv);
	
	/* Initialize components */
	emu_mem_init_k(64);
	emu_cpu_init("6502");
	tia = emu_video_init("tia", 3);
	pia = emu_generic_init("pia", 1);
	emu_mem_map_add(tia, 0x0, 0x7f);
	emu_mem_map_add(pia, 0x280, 0x297);
	//temp = emu_generic_init("temp", 1);

	/* Load ROM */
	emu_rom_load("rom/simple.bin", 0xF000);

	/* Start emulation */
	emu_cpu_reset();
	emu_main();

	return 0;
}
