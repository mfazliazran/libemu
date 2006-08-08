#include <libemu.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	/* Initialize libemu */
	emu_init(argc, argv);
	
	/* Initialize components */
	emu_mem_init_k(4);
	emu_mem_set(0, 'A');
	emu_mem_set(1, 'B');
	emu_mem_set(2, 'C');
	emu_cpu_init("x21");
	
	/* Load ROM */
	//emu_rom_load("x.rom", 0);

	/* Start emulation */
	emu_message("Will start emulation!");
	emu_cpu_reset();
	emu_main();

	return 0;
}
