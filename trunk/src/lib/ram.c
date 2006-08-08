#include "libemu.h"
#include "other.h"

unsigned char* ram;
unsigned long size;

void emu_mem_init(unsigned long sz)
{
	ram = g_malloc(sz);
	if(!ram)
		g_error("Not enough free memory when reseving memory for RAM.");
	else
		size = sz;
}

void emu_mem_init_k(unsigned int sz)
{
	emu_mem_init(sz * 1024);
}

void emu_mem_set_direct(unsigned long int pos, unsigned char data)
{
	if(pos > (size-1) || pos < 0)
		g_critical("Trying to write outside memory bounds, in position 0x%x!", pos);
	else
		ram[pos] = data;
}

void emu_mem_set(unsigned long int pos, unsigned char data)
{
	emu_mem_set_direct(pos, data);
}

unsigned char emu_mem_get(unsigned long int pos)
{
	if(pos > (size-1) || pos < 0)
		g_critical("Trying to read outside memory bounds, from position 0x%x!", pos);
	else
		return ram[pos];
}
