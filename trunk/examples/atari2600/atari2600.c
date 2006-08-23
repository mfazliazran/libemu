#include <libemu.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int pia, temp;

	/* Initialize libemu */
	emu_init(argc, argv);
	
	/* Initialize components */
	emu_mem_init_k(64);
	emu_cpu_init("6502");
	pia = emu_generic_init("pia");
	emu_mem_map_add(pia, 0x280, 0x297);
	temp = emu_generic_init("temp");

	/* Load ROM */
	emu_rom_load("rom/simple.bin", 0xF000);
	

	/* Start emulation */
	emu_cpu_reset();
	emu_main();

	return 0;
}
