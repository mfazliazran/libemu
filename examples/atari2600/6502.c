/*
 * Describe your processor here.
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libdev.h"

/***************************
 * a few helping functions *
 ***************************/
typedef unsigned short WORD;
typedef unsigned char BYTE;

/***************************
 * a few helping functions *
 ***************************/

/* bit manipulation */
#define UPPER(ad)		(((ad)>>8)&0xff)
#define LOWER(ad)		((ad)&0xff)
#define FULL(lo,hi)		((lo)|((hi)<<8))

/* flag manipulation */
#define SET_SIGN(a)             (S=(a)&0x80)
#define SET_ZERO(a)             (Z=!(a))
#define SET_CARRY(a)            (C=(a))
#define SET_INTERRUPT(a)        (I=(a))
#define SET_DECIMAL(a)          (D=(a))
#define SET_OVERFLOW(a)         (O=(a))
#define SET_BREAK(a)            (B=(a))

/* get & set flags */
#define SET_FLAGS(a)            (S=(a) & 0x80, Z=(a) & 0x02, C=(a) & 0x01, I=(a) & 0x04, D=(a) & 0x08, O=(a) & 0x40, B=(a) & 0x10)
#define GET_FLAGS()             ((S ? 0x80 : 0) | (Z ? 0x02 : 0) | (C ? 0x01 : 0) | (I ? 0x04 : 0) | (D ? 0x08 : 0) | (O ? 0x40 : 0) | (B ? 0x10 : 0) | 0x20)

/* gets the relative address */
#define REL_ADDR(pc,src) 	(pc+((signed char)src))

/* push & pull from stack */
#define PUSH(b) 		dev_mem_set(SP+0x100,(b));SP--
#define PULL()			dev_mem_get((++SP)+0x100)

/****************************
 * Variables that'll remain *
 * between cycles.          *
 ****************************/
BYTE opcode;		/* opcode */
WORD src, address;	/* source (parameter) */
BYTE lsrc, hsrc;	/* high and low byte of the source */
BYTE bytes_inst;	/* bytes used by this operation */
BYTE bytes_left;	/* bytes to end to read instruction */

WORD original_pc;  	/* PC of the last opcode */
int cycles;


EXPORT char dev_type[] = "cpu";

/* Fill in the name of the CPU */
EXPORT char dev_cpu_name[] = "MOS 6502";

/* Program Counter (also known as Instruction Pointer):
 * the memory address that contains the opcode currently being executed. */
WORD PC; 		/* program counter */

/* 
 * Registers & Flags:
 *
 * Put your registers and flags here. For example, if your processor has
 * two registers, one with 16 bits and the other with 8 bits, the 
 * appropriate code would be:
 *
 * unsigned char X;
 * unsigned short int Y; 
 *
 * If your processor has a flag Z and another N, then the appropriate code 
 * would be:
 * 
 * unsgined char N, Z;
 * 
 */
BYTE A;
BYTE X, Y;
BYTE S,O,B,D,I,Z,C;
BYTE SP;		/* stack pointer */

/* The following functions (inside the DEBUG directive) are used only by the
 * debugger, and will be stripped off when the final version is created. */
#ifdef DEBUG

/* This variable will be used to return the debugger data in a persistent 
 * way. */
char buffer[100];

/* You must implement this function.
 *
 * This function will return the register names. For example, if your processor
 * has two registers, X and Y, when n == 0, the function would return "X", and
 * when n == 1, it would return "Y". The funcion must return NULL for every 
 * other value of n. */
EXPORT char* dev_cpu_register_name(int n)
{
	switch(n)
	{
		case 0: return "A";
		case 1: return "X";
		case 2: return "Y";
		default: return NULL;
	}
}

/* You must implement this function.
 *
 * This function will return the register values. For example, if your processor
 * has two registers, X and Y, when n == 0, the function would return the value 
 * in X, and when n == 1, it would return the value in Y. The value of the 
 * register n must match the register n passed on the function register_name. */
EXPORT unsigned int dev_cpu_register_value(int n)
{
	switch(n)
	{
		case 0: return A;
		case 1: return X;
		case 2: return Y;
		default: return 0;
	}
}

/* You must implement this function.
 * 
 * This function will return the flag names. For example, if your processor
 * has two flags, N and Z, when n == 0, the function would return "N", and
 * when n == 1, it would return "Z". The funcion must return NULL for every other
 * value of n. */
EXPORT char* dev_cpu_flag_name(int n)
{
	switch(n)
	{
		case 0: return "S";
		case 1: return "O";
		case 2: return "B";
		case 3: return "D";
		case 4: return "I";
		case 5: return "Z";
		case 6: return "C";
		default: return NULL;
	}
}

/* You must implement this function.
 *
 * This function will return the flag values. For example, if your processor
 * has two flags, N and Z, when n == 0, the function would return the value 
 * in N, and when n == 1, it would return the value in Z. The value of the flag
 * n must match the flag n passed on the function flag_name. 
 *
 * The flag is boolean, so the return value should be 0 for FALSE, or any
 * other value for TRUE. */
EXPORT unsigned char dev_cpu_flag_value(int n)
{
	switch(n)
	{
		case 0: return S;
		case 1: return O;
		case 2: return B;
		case 3: return D;
		case 4: return I;
		case 5: return Z;
		case 6: return C;
		default: return 0;
	}
}

/* This code will return the human-readable opcode from a given position in
 * memory. It'll be used bu the debugger to display the data. The data must be
 * returned in a persistent way, using the global variable buffer, or a segfault
 * might happen. The parameters are:
 *
 * - addr -> the address to be debugger
 * - num_cycles -> return the number of cycles that would be spent
 * - bytes -> the number of bytes used by the instruction. It'll be more than
 *            one if the instruction recieves a parameter. 
 *
 * The function MUST return NULL in case of invalid opcode. */
EXPORT char* dev_cpu_debug(unsigned long addr, int *num_cycles, int *bytes)
{
	int dcycles = 0;
	char *debug_opcode;
	char *debug_par;
	char *dbg;
	int pc = addr;
	debug_opcode = malloc(10);
	debug_par = malloc(20);

again:
	/* load next byte from memory */
	switch(bytes_left)
	{
		case 0: 
			opcode = dev_mem_get(addr);
			/* find out how many bytes this opcode uses */
			switch(opcode)
			{
				/* 1 byte */
				case 0x0A: /* ASL */
				case 0x00: /* BRK */
				case 0x18: /* CLC */
				case 0xD8: /* CLD */
				case 0x58: /* CLI */
				case 0xB8: /* CLV */
				case 0xCA: /* DEX */
				case 0x88: /* DEY */
				case 0xE8: /* INX */
				case 0xC8: /* INY */
				case 0x4A: /* LSR */
				case 0xEA: /* NOP */
				case 0x48: /* PHA */
				case 0x08: /* PHP */
				case 0x68: /* PLA */
				case 0x28: /* PLP */
				case 0x2A: /* ROL */
				case 0x6A: /* ROR */
				case 0x4D: /* RTI */
				case 0x60: /* RTS */
				case 0x38: /* SEC */
				case 0xF8: /* SED */
				case 0x78: /* SEI */
				case 0xAA: /* TAX */
				case 0xA8: /* TAY */
				case 0xBA: /* TSX */
				case 0x8A: /* TXA */
				case 0x9A: /* TXS */
				case 0x98: /* TYA */
					bytes_inst = 1;
					bytes_left = 1;
					break;
					
				/* 2 bytes */
				case 0x61: /* ADC */
				case 0x69:
				case 0x65:
				case 0x71:
				case 0x75:
				case 0x21: /* AND */
				case 0x25:
				case 0x29:
				case 0x31:
				case 0x35:
				case 0x06: /* ASL */
				case 0x16:
				case 0x90: /* BCC */
				case 0xB0: /* BCS */
				case 0xF0: /* BEQ */
				case 0x24: /* BIT */
				case 0x30: /* BMI */
				case 0xD0: /* BNE */
				case 0x10: /* BPL */
				case 0x50: /* BVC */
				case 0x70: /* BVS */
				case 0xC9: /* CMP */
				case 0xC5:
				case 0xD5:
				case 0xC1:
				case 0xD1:
				case 0xE0: /* CPX */
				case 0xE4:
				case 0xC0: /* CPY */
				case 0xC4:
				case 0xC6: /* DEC */
				case 0xD6:
				case 0x49: /* EOR */
				case 0x45:
				case 0x55:
				case 0x41:
				case 0x51:
				case 0xE6: /* INC */
				case 0xF6:
				case 0xA9: /* LDA */
				case 0xA5:
				case 0xB5:
				case 0xA1:
				case 0xB1:
				case 0xA2: /* LDX */
				case 0xA6:
				case 0xB6:
				case 0xA0: /* LDY */
				case 0xA4:
				case 0xB4:
				case 0x46: /* LSR */
				case 0x56:
				case 0x04: /* NOP - undocumented */
				case 0x09: /* ORA */
				case 0x05:
				case 0x15:
				case 0x01:
				case 0x11:
				case 0x26: /* ROL */
				case 0x36:
				case 0x66: /* ROR */
				case 0x76:
				case 0xE9: /* SBC */
				case 0xE5:
				case 0xF5:
				case 0xE1:
				case 0xF1:
				case 0x85: /* STA */
				case 0x95:
				case 0x81:
				case 0x91:
				case 0x86: /* STX */
				case 0x96:
				case 0x84: /* STY */
				case 0x94:
					bytes_inst = 2;
					bytes_left = 2;
					break;

				/* 3 bytes */
				case 0x6D: /* ADC */
				case 0x79:
				case 0x7D:
				case 0x2D: /* AND */
				case 0x39:
				case 0x3D:
				case 0x0E: /* ASL */
				case 0x1E:
				case 0x2C: /* BIT */
				case 0xCD: /* CMP */
				case 0xDD:
				case 0xD9:
				case 0xEC: /* CPX */
				case 0xCC: /* CPY */
				case 0xCE: /* DEC */
				case 0xDE:
				case 0x40: /* EOR */
				case 0x5D:
				case 0x59:
				case 0xEE: /* INC */
				case 0xFE:
				case 0x4C: /* JMP */
				case 0x6C:
				case 0x20: /* JSR */
				case 0xAD: /* LDA */
				case 0xBD:
				case 0xB9:
				case 0xAE: /* LDX */
				case 0xBE:
				case 0xAC: /* LDY */
				case 0xBC:
				case 0x4E: /* LSR */
				case 0x5E:
				case 0x1D: /* ORA */
				case 0x19:
				case 0x2E: /* ROL */
				case 0x3E:
				case 0x6E: /* ROR */
				case 0x7E:
				case 0xED: /* SBC */
				case 0xFD:
				case 0x8D: /* STA */
				case 0x9D:
				case 0x99:
				case 0x8E: /* STX */
				case 0x8C: /* STY */
					bytes_inst = 3;
					bytes_left = 3;
					break;
				default:
					*num_cycles = 0;
					*bytes = 0;
					return NULL;
			}
			lsrc = hsrc = src = 0; /* clear src variables */
			break;
		case 1:
			if(bytes_inst == 2)
				lsrc = dev_mem_get(addr);
			else /* if == 3 */
				hsrc = dev_mem_get(addr);
			break;
		case 2:
			lsrc = dev_mem_get(addr);
			break;
	}

	/* advance position in the memory */
	addr++;

	bytes_left--;

	if(bytes_left != 0) 
		goto again;

	/* if there's no byte left to mount the instruction,
	 * execute it */
	if (bytes_left == 0)
	{
		debug_opcode = " ";
		
		switch (opcode)
		{
			/* Implied: the opcode uses no parameter */
			case 0x00: /* BRK */
				dcycles = 1; /* 7 cycles */
			case 0x4D: /* RTI */
			case 0x60: /* RTS */
				dcycles += 2; /* 6 cycles */
			case 0x68: /* PLA */
			case 0x28: /* PLP */
				dcycles += 1; /* 4 cycles */
			case 0x48: /* PHA */
			case 0x08: /* PHP */
				dcycles += 1; /* 3 cycles */
			case 0x18: /* CLC */
			case 0xD8: /* CLD */
			case 0x58: /* CLI */
			case 0xB8: /* CLV */
			case 0xCA: /* DEX */
			case 0x88: /* DEY */
			case 0xE8: /* INX */
			case 0xC8: /* INY */
			case 0xEA: /* NOP */
			case 0x38: /* SEC */
			case 0xF8: /* SED */
			case 0x78: /* SEI */
			case 0xAA: /* TAX */
			case 0xA8: /* TAY */
			case 0xBA: /* TSX */
			case 0x8A: /* TXA */
			case 0x9A: /* TXS */
			case 0x98: /* TYA */
				dcycles += 2; /* 2 cycles */
				sprintf(debug_par," ");
				break;

			/* Accumulator: it'll use the accumulator (A)
			 * as parameter. */
			case 0x0A: /* ASL */
			case 0x4A: /* LSR */
			case 0x2A: /* ROL */
			case 0x6A: /* ROR */
				dcycles = 2;
				sprintf(debug_par,"A");
				break;

			/* Immediate: it'll pass an absoulte number
			 * as a parameter. */
			case 0x04: /* NOP - undocumented! */
				dcycles = 1;
			case 0x69: /* ADC */
			case 0x29: /* AND */
			case 0xC9: /* CMP */
			case 0xE0: /* CPX */
			case 0xC0: /* CPY */
			case 0x49: /* EOR */
			case 0xA9: /* LDA */
			case 0xA2: /* LDX */
			case 0xA0: /* LDY */
			case 0x09: /* ORA */
			case 0xE9: /* SBC */
				dcycles += 2;
				sprintf(debug_par, "#$%02x", lsrc);
				break;

			/* Zero page: it'll use only one byte to access the
			 * addresses $0000-00FF from the memory. */
			case 0x06: /* ASL */
			case 0xC6: /* DEC */
			case 0xE6: /* INC */
			case 0x46: /* LSR */
			case 0x26: /* ROL */
			case 0x66: /* ROR */
				dcycles = 2; /* 5 cycles */
			case 0x65: /* ADC */
			case 0x25: /* AND */
			case 0x24: /* BIT */
			case 0xC5: /* CMP */
			case 0xE4: /* CPX */
			case 0xC4: /* CPY */
			case 0x45: /* EOR */
			case 0xA5: /* LDA */
			case 0xA6: /* LDX */
			case 0xA4: /* LDY */
			case 0x05: /* ORA */
			case 0xE5: /* SBC */
			case 0x85: /* STA */
			case 0x86: /* STX */
			case 0x84: /* STY */
				dcycles += 3; /* 3 cycles */
				sprintf(debug_par, "$%02x", lsrc);
				break;

			/* Zero page, X: loads from the (zero page + X) */
			case 0x16: /* ASL */
			case 0xD6: /* DEC */
			case 0xF6: /* INC */
			case 0x56: /* LSR */
			case 0x36: /* ROL */
			case 0x76: /* ROR */
				dcycles = 2; /* 6 cycles */
			case 0x75: /* ADC */
			case 0x35: /* AND */
			case 0xD5: /* CMP */
			case 0x55: /* EOR */
			case 0xB5: /* LDA */
			case 0xB4: /* LDY */
			case 0x15: /* ORA */
			case 0xF5: /* SBC */
			case 0x95: /* STA */
			case 0x96: /* STX */
			case 0x94: /* STY */
				dcycles += 4; /* 4 cycles */
				sprintf(debug_par, "$%02x,X", lsrc);
				break;

			/* Zero page, Y: loads from the (zero page + Y) */
			case 0xB6: /* LDX */
				dcycles = 4;
				sprintf(debug_par, "$%02x,Y", lsrc);
				break;

			/* Absolute: it'll pass an absolute 16-bit memory
			 * address. */
			case 0x0E: /* ASL */
			case 0xCE: /* DEC */
			case 0xEE: /* INC */
			case 0x20: /* JSR */
			case 0x4E: /* LSR */
			case 0x2E: /* ROL */
			case 0x6E: /* ROR */
				dcycles += 2; /* 6 cycles */
			case 0x6D: /* ADC */
			case 0x2D: /* AND */
			case 0x2C: /* BIT */
			case 0xCD: /* CMP */
			case 0xEC: /* CPX */
			case 0xCC: /* CPY */
			case 0x40: /* EOR */
			case 0xAD: /* LDA */
			case 0xAE: /* LDX */
			case 0xAC: /* LDY */
			case 0x0D: /* ORA */
			case 0xED: /* SBC */
			case 0x8D: /* STA */
			case 0x8E: /* STX */
			case 0x8C: /* STY */
				dcycles += 1; /* 4 cycles */
			case 0x4C: /* JMP */
				dcycles += 3; /* 3 cycles */
				sprintf(debug_par, "$%04x", FULL(lsrc,hsrc));
				break;

			/* Absoulte, X: get the byte in the (absolute + X)
			 * position. */
			case 0x1E: /* ASL */
			case 0xDE: /* DEC */
			case 0xFE: /* INC */
			case 0x5E: /* LSR */
			case 0x3E: /* ROL */
			case 0x7E: /* ROR */
				dcycles = 2; /* 7 cycles */
			case 0x9D: /* STA */
				dcycles++; /* 5 cycles */
			case 0x7D: /* ADC */
			case 0x3D: /* AND */
			case 0xDD: /* CMP */
			case 0x5D: /* EOR */
			case 0xBD: /* LDA */
			case 0xBC: /* LDY */
			case 0x1D: /* ORA */
			case 0xFD: /* SBC */
				dcycles += 4; /* 4 cycles */
				sprintf(debug_par, "$%04x,X",FULL(lsrc,hsrc));
				break;
				
			/* Absoulte, Y: get the byte in the (absolute + Y)
			 * position. */
			case 0x99: /* STA */
				dcycles = 1; /* 5 cycles */
			case 0x79: /* ADC */
			case 0x39: /* AND */
			case 0xD9: /* CMP */
			case 0x59: /* EOR */
			case 0xB9: /* LDA */
			case 0xBE: /* LDY */
			case 0x19: /* ORA */
			case 0xF9: /* SBC */
				dcycles += 4; /* 4 cycles */
				sprintf(debug_par, "$%04x,Y", FULL(lsrc,hsrc));
				break;

			/* (Indirect, X): get the byte into the (zero page + X)
			 * and the following one and makes an adress Z. Takes 
			 * the byte from Z. */
			case 0x61: /* ADC */
			case 0x21: /* AND */
			case 0xC1: /* CMP */
			case 0x41: /* EOR */
			case 0xA1: /* LDA */
			case 0x01: /* ORA */
			case 0xE1: /* SBC */
			case 0x81: /* STA */
				dcycles = 6;
				sprintf(debug_par, "($%02x,X)", lsrc);
				break;

			/* (Indirect), Y: get the byte into the zero page and
			 * the following one and makes an adress Z. Takes the
			 * byte from (Z + Y). */
			case 0x91: /* STA */
				dcycles = 1; /* 6 cycles */
			case 0x71: /* ADC */
			case 0x31: /* AND */
			case 0xD1: /* CMP */
			case 0x51: /* EOR */
			case 0xB1: /* LDA */
			case 0x11: /* ORA */
			case 0xF1: /* SBC */
				dcycles += 5; /* 5 cycles */
				sprintf(debug_par, "($%02x),Y", lsrc);
				break;

			/* Relative: used in branches. Adds to PC if the
			 * highest bit of the parameter is 0, and substracts
			 * from PC if it's 1. */
			case 0x90: /* BCC */
			case 0xB0: /* BCS */
			case 0xF0: /* BEQ */
			case 0x30: /* BMI */
			case 0xD0: /* BNE */
			case 0x10: /* BPL */
			case 0x50: /* BVC */
			case 0x70: /* BVS */
				dcycles = 2;
				sprintf(debug_par, "$%04x",
							REL_ADDR(pc+bytes_inst, lsrc));
				break;

			/* Indirect: it takes the byte that's into the given
			 * position of the memory and the next byte. */
			case 0x6C: /* JMP */
				dcycles = 5;
				sprintf(debug_par, "($%04x)", src);
				break;

		}

		/* Instruction */
		switch (opcode)
		{

			case 0x6D:
			case 0x61:
			case 0x65:
			case 0x69:
			case 0x71:
			case 0x75:
			case 0x79:
			case 0x7D:
				debug_opcode = "ADC";
				break;
			case 0x21:
			case 0x25:
			case 0x29:
			case 0x2D:
			case 0x31:
			case 0x35:
			case 0x39:
			case 0x3D:
				debug_opcode = "AND";
				break;
			case 0x0A:
			case 0x06:
			case 0x16:
			case 0x0E:
			case 0x1E:
				debug_opcode = "ASL";
				break;
			case 0x90:
				debug_opcode = "BCC";
				break;
			case 0xB0:
				debug_opcode = "BCS";
				break;
			case 0xF0:
				debug_opcode = "BEQ";
				break;
			case 0x24:
			case 0x2C:
				debug_opcode = "BIT";
				break;
			case 0x30:
				debug_opcode = "BMI";
				break;
			case 0xD0:
				debug_opcode = "BNE";
				break;
			case 0x10:
				debug_opcode = "BPL";
				break;
			case 0x00:
				debug_opcode = "BRK";
				break;
			case 0x50:
				debug_opcode = "BVC";
				break;
			case 0x70:
				debug_opcode = "BVS";
				break;
			case 0x18:
				debug_opcode = "CLC";
				break;
			case 0xD8:
				debug_opcode = "CLD";
				break;
			case 0x58:
				debug_opcode = "CLI";
				break;
			case 0xB8:
				debug_opcode = "CLV";
				break;
			case 0xC9:
			case 0xC5:
			case 0xD5:
			case 0xCD:
			case 0xDD:
			case 0xD9:
			case 0xC1:
			case 0xD1:
				debug_opcode = "CMP";
				break;
			case 0xE0:
			case 0xE4:
			case 0xEC:
				debug_opcode = "CPX";
				break;
			case 0xC0:
			case 0xC4:
			case 0xCC:
				debug_opcode = "CPY";
				break;
			case 0xC6:
			case 0xD6:
			case 0xCE:
			case 0xDE:
				debug_opcode = "DEC";
				break;
			case 0xCA:
				debug_opcode = "DEX";
				break;
			case 0x88:
				debug_opcode = "DEY";
				break;
			case 0x49:
			case 0x45:
			case 0x55:
			case 0x40:
			case 0x5D:
			case 0x59:
			case 0x41:
			case 0x51:
				debug_opcode = "EOR";
				break;
			case 0xE6:
			case 0xF6:
			case 0xEE:
			case 0xFE:
				debug_opcode = "INC";
				break;
			case 0xE8:
				debug_opcode = "INX";
				break;
			case 0xC8:
				debug_opcode = "INY";
				break;
			case 0x4C:
			case 0x6C:
				debug_opcode = "JMP";
				break;
			case 0x20:
				debug_opcode = "JSR";
				break;
			case 0xA9:
			case 0xA5:
			case 0xB5:
			case 0xAD:
			case 0xBD:
			case 0xB9:
			case 0xA1:
			case 0xB1:
				debug_opcode = "LDA";
				break;
			case 0xA2:
			case 0xA6:
			case 0xB6:
			case 0xAE:
			case 0xBE:
				debug_opcode = "LDX";
				break;
			case 0xA0:
			case 0xA4:
			case 0xB4:
			case 0xAC:
			case 0xBC:
				debug_opcode = "LDY";
				break;
			case 0x4A:
			case 0x46:
			case 0x56:
			case 0x4E:
			case 0x5E:
				debug_opcode = "LSR";
				break;
			case 0x04:
			case 0xEA:
				debug_opcode = "NOP";
				break;
			case 0x09:
			case 0x05:
			case 0x15:
			case 0x0D:
			case 0x1D:
			case 0x19:
			case 0x01:
			case 0x11:
				debug_opcode = "ORA";
				break;
			case 0x48:
				debug_opcode = "PHA";
				break;
			case 0x08:
				debug_opcode = "PHP";
				break;
			case 0x68:
				debug_opcode = "PLA";
				break;
			case 0x28:
				debug_opcode = "PLP";
				break;
			case 0x2A:
			case 0x26:
			case 0x36:
			case 0x2E:
			case 0x3E:
				debug_opcode = "ROL";
				break;
			case 0x6A:
			case 0x66:
			case 0x76:
			case 0x6E:
			case 0x7E:
				debug_opcode = "ROR";
				break;
			case 0x4D:
				debug_opcode = "RTI";
				break;
			case 0x60:
				debug_opcode = "RTS";
				break;
			case 0xE9:
			case 0xE5:
			case 0xF5:
			case 0xED:
			case 0xFD:
			case 0xF9:
			case 0xE1:
			case 0xF1:
				debug_opcode = "SBC";
				break;
			case 0x38:
				debug_opcode = "SEC";
				break;
			case 0xF8:
				debug_opcode = "SED";
				break;
			case 0x78:
				debug_opcode = "SEI";
				break;
			case 0x85:
			case 0x95:
			case 0x8D:
			case 0x9D:
			case 0x99:
			case 0x81:
			case 0x91:
				debug_opcode = "STA";
				break;
			case 0x86:
			case 0x96:
			case 0x8E:
				debug_opcode = "STX";
				break;
			case 0x84:
			case 0x94:
			case 0x8C:
				debug_opcode = "STY";
				break;
			case 0xAA:
				debug_opcode = "TAX";
				break;
			case 0xA8:
				debug_opcode = "TAY";
				break;
			case 0xBA:
				debug_opcode = "TSX";
				break;
			case 0x8A:
				debug_opcode = "TXA";
				break;
			case 0x9A:
				debug_opcode = "TXS";
				break;
			case 0x98:
				debug_opcode = "TYA";
				break;
		}
	}
	*num_cycles = dcycles;
	*bytes = bytes_inst;
	dbg = malloc(40);
	sprintf(dbg, "%s %s", debug_opcode, debug_par);
	return dbg;
}

#endif

/* The next functions are going to be part of the final emulator. */

/* You must implement this function.
 *
 * Resets the emulator. Everything must be set as how the CPU would look like
 * when turned on. */
EXPORT void dev_cpu_reset()
{
	bytes_left = 0;
	A = X = Y = S = O = B = D = I = Z = C = 0;
	PC = (dev_mem_get(0xfffd)*0x100) + dev_mem_get(0xfffc);
}

/* This function returns the Instruction Pointer. */
EXPORT unsigned long int dev_cpu_ip()
{
	return PC; /* original_pc */
}

/* You must implement this function.
 *
 * This function performs a CPU step. Here is where the magic happens :-)
 * The function must return 0 in case of success, or -1 in case of faluire
 * (a bad opcode, for example). The num_cycles parameter is used to return
 * the number of cycles spent on the step. You must advance the Instruction
 * Pointer manually.
 *
 * The skeleton below is from a very simple CPU, and more complex CPUs might
 * have to modify the whole function. 
 *
 * This code will run hundreds of times per second, so make it run as fast as
 * possible, but remember to mantain it C ANSI and portable. */
EXPORT int dev_cpu_step(int *num_cycles)
{
	unsigned int temp; /* helping variable */
	original_pc = PC;
	cycles = 0;

again:
	/******************************
	 * load next byte from memory *
	 ******************************/
	switch(bytes_left)
	{
		case 0: 
			opcode = dev_mem_get(PC);
			/* find out how many bytes this opcode uses */
			switch(opcode)
			{
				/* 1 byte */
				case 0x0A: /* ASL */
				case 0x00: /* BRK */
				case 0x18: /* CLC */
				case 0xD8: /* CLD */
				case 0x58: /* CLI */
				case 0xB8: /* CLV */
				case 0xCA: /* DEX */
				case 0x88: /* DEY */
				case 0xE8: /* INX */
				case 0xC8: /* INY */
				case 0x4A: /* LSR */
				case 0xEA: /* NOP */
				case 0x48: /* PHA */
				case 0x08: /* PHP */
				case 0x68: /* PLA */
				case 0x28: /* PLP */
				case 0x2A: /* ROL */
				case 0x6A: /* ROR */
				case 0x4D: /* RTI */
				case 0x60: /* RTS */
				case 0x38: /* SEC */
				case 0xF8: /* SED */
				case 0x78: /* SEI */
				case 0xAA: /* TAX */
				case 0xA8: /* TAY */
				case 0xBA: /* TSX */
				case 0x8A: /* TXA */
				case 0x9A: /* TXS */
				case 0x98: /* TYA */
					bytes_inst = 1;
					bytes_left = 1;
					break;
					
				/* 2 bytes */
				case 0x61: /* ADC */
				case 0x69:
				case 0x65:
				case 0x71:
				case 0x75:
				case 0x21: /* AND */
				case 0x25:
				case 0x29:
				case 0x31:
				case 0x35:
				case 0x06: /* ASL */
				case 0x16:
				case 0x90: /* BCC */
				case 0xB0: /* BCS */
				case 0xF0: /* BEQ */
				case 0x24: /* BIT */
				case 0x30: /* BMI */
				case 0xD0: /* BNE */
				case 0x10: /* BPL */
				case 0x50: /* BVC */
				case 0x70: /* BVS */
				case 0xC9: /* CMP */
				case 0xC5:
				case 0xD5:
				case 0xC1:
				case 0xD1:
				case 0xE0: /* CPX */
				case 0xE4:
				case 0xC0: /* CPY */
				case 0xC4:
				case 0xC6: /* DEC */
				case 0xD6:
				case 0x49: /* EOR */
				case 0x45:
				case 0x55:
				case 0x41:
				case 0x51:
				case 0xE6: /* INC */
				case 0xF6:
				case 0xA9: /* LDA */
				case 0xA5:
				case 0xB5:
				case 0xA1:
				case 0xB1:
				case 0xA2: /* LDX */
				case 0xA6:
				case 0xB6:
				case 0xA0: /* LDY */
				case 0xA4:
				case 0xB4:
				case 0x46: /* LSR */
				case 0x56:
				case 0x04: /* NOP */
				case 0x09: /* ORA */
				case 0x05:
				case 0x15:
				case 0x01:
				case 0x11:
				case 0x26: /* ROL */
				case 0x36:
				case 0x66: /* ROR */
				case 0x76:
				case 0xE9: /* SBC */
				case 0xE5:
				case 0xF5:
				case 0xE1:
				case 0xF1:
				case 0x85: /* STA */
				case 0x95:
				case 0x81:
				case 0x91:
				case 0x86: /* STX */
				case 0x96:
				case 0x84: /* STY */
				case 0x94:
					bytes_inst = 2;
					bytes_left = 2;
					break;

				/* 3 bytes */
				case 0x6D: /* ADC */
				case 0x79:
				case 0x7D:
				case 0x2D: /* AND */
				case 0x39:
				case 0x3D:
				case 0x0E: /* ASL */
				case 0x1E:
				case 0x2C: /* BIT */
				case 0xCD: /* CMP */
				case 0xDD:
				case 0xD9:
				case 0xEC: /* CPX */
				case 0xCC: /* CPY */
				case 0xCE: /* DEC */
				case 0xDE:
				case 0x40: /* EOR */
				case 0x5D:
				case 0x59:
				case 0xEE: /* INC */
				case 0xFE:
				case 0x4C: /* JMP */
				case 0x6C:
				case 0x20: /* JSR */
				case 0xAD: /* LDA */
				case 0xBD:
				case 0xB9:
				case 0xAE: /* LDX */
				case 0xBE:
				case 0xAC: /* LDY */
				case 0xBC:
				case 0x4E: /* LSR */
				case 0x5E:
				case 0x1D: /* ORA */
				case 0x19:
				case 0x2E: /* ROL */
				case 0x3E:
				case 0x6E: /* ROR */
				case 0x7E:
				case 0xED: /* SBC */
				case 0xFD:
				case 0x8D: /* STA */
				case 0x9D:
				case 0x99:
				case 0x8E: /* STX */
				case 0x8C: /* STY */
					bytes_inst = 3;
					bytes_left = 3;
					break;
	
				/* Invalid operation */
				default:
					return 0;
					break;

			}
			lsrc = hsrc = src = 0; /* clear src variables */
			break;
		case 1:
			if(bytes_inst == 2)
				lsrc = dev_mem_get(PC);
			else /* if == 3 */
				hsrc = dev_mem_get(PC);
			break;
		case 2:
			lsrc = dev_mem_get(PC);
			break;
	}

	/* advance position in the memory */
	PC++;

	bytes_left--;

	if(bytes_left != 0) 
		goto again;

	/* if there's no byte left to mount the instruction,
	 * execute it */
	if (bytes_left == 0)
	{
		/*******************
		 * Adressing modes *
		 *******************
		 * This will get the source to be used as a parameter to
		 * the instruction. The 6502 has quite a few adressing modes,
		 * so this switch will (hopefully) cover all of them */
		switch (opcode)
		{
			/* Implied: the opcode uses no parameter */
			case 0x00: /* BRK */
				cycles = 1; /* 7 cycles */
			case 0x4D: /* RTI */
			case 0x60: /* RTS */
				cycles += 2; /* 6 cycles */
			case 0x68: /* PLA */
			case 0x28: /* PLP */
				cycles += 1; /* 4 cycles */
			case 0x48: /* PHA */
			case 0x08: /* PHP */
				cycles += 1; /* 3 cycles */
			case 0x18: /* CLC */
			case 0xD8: /* CLD */
			case 0x58: /* CLI */
			case 0xB8: /* CLV */
			case 0xCA: /* DEX */
			case 0x88: /* DEY */
			case 0xE8: /* INX */
			case 0xC8: /* INY */
			case 0xEA: /* NOP */
			case 0x38: /* SEC */
			case 0xF8: /* SED */
			case 0x78: /* SEI */
			case 0xAA: /* TAX */
			case 0xA8: /* TAY */
			case 0xBA: /* TSX */
			case 0x8A: /* TXA */
			case 0x9A: /* TXS */
			case 0x98: /* TYA */
				cycles += 2; /* 2 cycles */
				break;

			/* Accumulator: it'll use the accumulator (A)
			 * as parameter. */
			case 0x0A: /* ASL */
			case 0x4A: /* LSR */
			case 0x2A: /* ROL */
			case 0x6A: /* ROR */
				cycles = 2;
				src = A;
				break;

			/* Immediate: it'll pass an absoulte number
			 * as a parameter. */
			case 0x04: /* NOP */
				cycles = 1;
			case 0x69: /* ADC */
			case 0x29: /* AND */
			case 0xC9: /* CMP */
			case 0xE0: /* CPX */
			case 0xC0: /* CPY */
			case 0x49: /* EOR */
			case 0xA9: /* LDA */
			case 0xA2: /* LDX */
			case 0xA0: /* LDY */
			case 0x09: /* ORA */
			case 0xE9: /* SBC */
				cycles += 2;
				src = FULL(lsrc,0);
				break;

			/* Zero page: it'll use only one byte to access the
			 * addresses $0000-00FF from the memory. */
			case 0x06: /* ASL */
			case 0xC6: /* DEC */
			case 0xE6: /* INC */
			case 0x46: /* LSR */
			case 0x26: /* ROL */
			case 0x66: /* ROR */
				cycles = 2; /* 5 cycles */
			case 0x65: /* ADC */
			case 0x25: /* AND */
			case 0x24: /* BIT */
			case 0xC5: /* CMP */
			case 0xE4: /* CPX */
			case 0xC4: /* CPY */
			case 0x45: /* EOR */
			case 0xA5: /* LDA */
			case 0xA6: /* LDX */
			case 0xA4: /* LDY */
			case 0x05: /* ORA */
			case 0xE5: /* SBC */
			case 0x85: /* STA */
			case 0x86: /* STX */
			case 0x84: /* STY */
				cycles += 3; /* 3 cycles */
				address = lsrc;
				src = dev_mem_get(address);
				break;

			/* Zero page, X: loads from the (zero page + X) */
			case 0x16: /* ASL */
			case 0xD6: /* DEC */
			case 0xF6: /* INC */
			case 0x56: /* LSR */
			case 0x36: /* ROL */
			case 0x76: /* ROR */
				cycles = 2; /* 6 cycles */
			case 0x75: /* ADC */
			case 0x35: /* AND */
			case 0xD5: /* CMP */
			case 0x55: /* EOR */
			case 0xB5: /* LDA */
			case 0xB4: /* LDY */
			case 0x15: /* ORA */
			case 0xF5: /* SBC */
			case 0x95: /* STA */
			case 0x96: /* STX */
			case 0x94: /* STY */
				cycles += 4; /* 4 cycles */
				address = lsrc + X;
				src = dev_mem_get(address);
				break;

			/* Zero page, Y: loads from the (zero page + Y) */
			case 0xB6: /* LDX */
				cycles = 4;
				address = lsrc + Y;
				src = dev_mem_get(address);
				break;

			/* Absolute: it'll pass an absolute 16-bit memory
			 * address. */
			case 0x0E: /* ASL */
			case 0xCE: /* DEC */
			case 0xEE: /* INC */
			case 0x20: /* JSR */
			case 0x4E: /* LSR */
			case 0x2E: /* ROL */
			case 0x6E: /* ROR */
				cycles = 2; /* 6 cycles */
			case 0x6D: /* ADC */
			case 0x2D: /* AND */
			case 0x2C: /* BIT */
			case 0xCD: /* CMP */
			case 0xEC: /* CPX */
			case 0xCC: /* CPY */
			case 0x40: /* EOR */
			case 0xAD: /* LDA */
			case 0xAE: /* LDX */
			case 0xAC: /* LDY */
			case 0x0D: /* ORA */
			case 0xED: /* SBC */
			case 0x8D: /* STA */
			case 0x8E: /* STX */
			case 0x8C: /* STY */
				cycles += 1; /* 4 cycles */
			case 0x4C: /* JMP */
				cycles += 3; /* 3 cycles */
				address = FULL(lsrc,hsrc);
				src = dev_mem_get(address);
				break;

			/* Absoulte, X: get the byte in the (absolute + X)
			 * position. */
			case 0x1E: /* ASL */
			case 0xDE: /* DEC */
			case 0xFE: /* INC */
			case 0x5E: /* LSR */
			case 0x3E: /* ROL */
			case 0x7E: /* ROR */
				cycles = 2; /* 7 cycles */
			case 0x9D: /* STA */
				cycles++; /* 5 cycles */
			case 0x7D: /* ADC */
			case 0x3D: /* AND */
			case 0xDD: /* CMP */
			case 0x5D: /* EOR */
			case 0xBD: /* LDA */
			case 0xBC: /* LDY */
			case 0x1D: /* ORA */
			case 0xFD: /* SBC */
				cycles += 4; /* 4 cycles */
				address = FULL(lsrc,hsrc) + X;
				src = dev_mem_get(address);
				break;
				
			/* Absoulte, Y: get the byte in the (absolute + Y)
			 * position. */
			case 0x99: /* STA */
				cycles = 1; /* 5 cycles */
			case 0x79: /* ADC */
			case 0x39: /* AND */
			case 0xD9: /* CMP */
			case 0x59: /* EOR */
			case 0xB9: /* LDA */
			case 0xBE: /* LDY */
			case 0x19: /* ORA */
			case 0xF9: /* SBC */
				cycles += 4; /* 4 cycles */
				address = FULL(lsrc,hsrc) + Y;
				src = dev_mem_get(address);
				break;

			/* (Indirect, X): get the byte into the (zero page + X)
			 * and the following one and makes an adress Z. Takes 
			 * the byte from Z. */
			case 0x61: /* ADC */
			case 0x21: /* AND */
			case 0xC1: /* CMP */
			case 0x41: /* EOR */
			case 0xA1: /* LDA */
			case 0x01: /* ORA */
			case 0xE1: /* SBC */
			case 0x81: /* STA */
				cycles = 6;
				address = FULL(dev_mem_get(lsrc+X), dev_mem_get(lsrc+X+1));
				src = dev_mem_get(address);
				break;

			/* (Indirect), Y: get the byte into the zero page and
			 * the following one and makes an adress Z. Takes the
			 * byte from (Z + Y). */
			case 0x91: /* STA */
				cycles = 1; /* 6 cycles */
			case 0x71: /* ADC */
			case 0x31: /* AND */
			case 0xD1: /* CMP */
			case 0x51: /* EOR */
			case 0xB1: /* LDA */
			case 0x11: /* ORA */
			case 0xF1: /* SBC */
				cycles += 5; /* 5 cycles */
				address = FULL(dev_mem_get(lsrc),
							dev_mem_get(lsrc+1))+Y;
				src = dev_mem_get(address);
				break;

			/* Relative: used in branches. Adds to PC if the
			 * highest bit of the parameter is 0, and substracts
			 * from PC if it's 1. */
			case 0x90: /* BCC */
			case 0xB0: /* BCS */
			case 0xF0: /* BEQ */
			case 0x30: /* BMI */
			case 0xD0: /* BNE */
			case 0x10: /* BPL */
			case 0x50: /* BVC */
			case 0x70: /* BVS */
				cycles = 2;
				src = REL_ADDR(PC, lsrc);
				break;

			/* Indirect: it takes the byte that's into the given
			 * position of the memory and the next byte. */
			case 0x6C: /* JMP */
				cycles = 5;
				src = FULL(lsrc,hsrc);
				address = FULL(
						dev_mem_get(address),
						dev_mem_get(address+1)
						);
				break;

		}

		/***************************
		 * Execute the instruction *
		 ***************************/
		switch (opcode)
		{

			/*******
			 * ADC *  ->  add memory to accumulator with carry
			 *******/
			case 0x6D:
			case 0x61:
			case 0x65:
			case 0x69:
			case 0x71:
			case 0x75:
			case 0x79:
			case 0x7D:
				temp = src + A + ((C!=0) ? 1 : 0);
				/* This is not valid in decimal mode */
				SET_ZERO(temp & 0xff);
				if (D) 
				{
					if (((A&0xf)+(src&0xf)+((C!=0) ? 1 : 0))>9)
						temp += 6;
					SET_SIGN(temp);
					SET_OVERFLOW(!((A^src)&0x80)
							&&((A^temp)&0x80));
					if (temp > 0x99) 
						temp += 96;
					SET_CARRY(temp > 0x99);
				} 
				else 
				{
					SET_SIGN(temp);
					SET_OVERFLOW(!((A^src)&0x80)
							&&((A^temp)&0x80));
					SET_CARRY(temp > 0xff);
				}
				A = ((BYTE) temp);
				break;

			/*******
			 * AND *  -> logical AND between A and memory
			 *******/
			case 0x21:
			case 0x25:
			case 0x29:
			case 0x2D:
			case 0x31:
			case 0x35:
			case 0x39:
			case 0x3D:
				A &= src;
				SET_SIGN(A);
				SET_ZERO(A);
				break;

			/*******
			 * ASL *  -> Shift left one bit (memory or A)
			 *******/
			case 0x0A:
			case 0x06:
			case 0x16:
			case 0x0E:
			case 0x1E:
				SET_CARRY(src & 0x80);
				src <<= 1;
				src &= 0xFF;
				SET_SIGN(src);
				SET_ZERO(src);
				if (opcode == 0x0A)
					A = src;
				else
					dev_mem_set(address,src);
				break;

			/*******
			 * BCC *  -> Branch on carry clear (i.e., C == 0)
			 *******/
			case 0x90:
				if (C == 0)
					PC = src;
				break;

			/*******
			 * BCS *  -> Branch on carry set (i.e., C == 1)
			 *******/
			case 0xB0:
				if (C != 0)
					PC = src;
				break;

			/*******
			 * BEQ *  -> Branch if equal (i.e., Z == 1)
			 *******/
			case 0xF0:
				if (Z != 0)
					PC = src;
				break;

			/*******
			 * BIT *  -> Test bits in memory with A
			 *******/
			case 0x24:
			case 0x2C:
				SET_SIGN(src);
				/* Copy bit 6 to OVERFLOW flag. */
				SET_OVERFLOW(0x40 & src);
				SET_ZERO(src & A);
				break;

			/*******
			 * BMI *  -> Branch if minus (i.e., S == 1)
			 *******/
			case 0x30:
				if (S != 0)
					PC = src;
				break;

			/*******
			 * BNE *  -> Branch if not equal (i.e., Z == 0)
			 *******/
			case 0xD0:
				if (Z == 0)
					PC = src;
				break;
				
			/*******
			 * BPL *  -> Branch if plus (i.e., S == 0)
			 *******/
			case 0x10:
				if (S == 0)
					PC = src;
				break;

			/*******
			 * BRK *  -> Force break
			 *******/
			case 0x00:
				/* push return address onto the stack */
				PUSH((PC >> 8) & 0xFF);
				PUSH(PC & 0xFF);
				SET_BREAK((1));
				PUSH(SP);
				SET_INTERRUPT((1));
				PC = (dev_mem_get(0xFFFE)|(dev_mem_get(0xFFFF)<<8));
				break;

			/*******
			 * BVC *  -> Branch on overflow clear
			 *******/
			case 0x50:
				if (O == 0)
					PC = src;
				break;

			/*******
			 * BVS *  -> Branch on overflow set
			 *******/
			case 0x70:
				if (O != 0)
					PC = src;
				break;

			/*******
			 * CLC *  -> Clear carry flag
			 *******/
			case 0x18:
				SET_CARRY(0);
				break;

			/*******
			 * CLD *  -> Clear decimal flag
			 *******/
			case 0xD8:
				SET_DECIMAL(0);
				break;

			/*******
			 * CLI *  -> Clear interrupt flag
			 *******/
			case 0x58:
				SET_INTERRUPT(0);
				break;

			/*******
			 * CLV *  -> Clear overflow flag
			 *******/
			case 0xB8:
				SET_OVERFLOW(0);
				break;

			/*******
			 * CMP *  -> Compare memory with A
			 *******/
			case 0xC9:
			case 0xC5:
			case 0xD5:
			case 0xCD:
			case 0xDD:
			case 0xD9:
			case 0xC1:
			case 0xD1:
				src = A - src;
				SET_CARRY(src < 0x100);
				SET_SIGN(src);
				SET_ZERO(src &= 0xff);
				break;

			/*******
			 * CPX *  -> Compare memory with X
			 *******/
			case 0xE0:
			case 0xE4:
			case 0xEC:
				src = X - src;
				SET_CARRY(src < 0x100);
				SET_SIGN(src);
				SET_ZERO(src &= 0xff);
				break;
				
			/*******
			 * CPY *  -> Compare memory with Y
			 *******/
			case 0xC0:
			case 0xC4:
			case 0xCC:
				src = Y - src;
				SET_CARRY(src < 0x100);
				SET_SIGN(src);
				SET_ZERO(src &= 0xff);
				break;

			/*******
			 * DEC *  -> Decrement memory by one
			 *******/
			case 0xC6:
			case 0xD6:
			case 0xCE:
			case 0xDE:
				src = (src - 1) & 0xff;
				SET_SIGN(src);
				SET_ZERO(src);
				dev_mem_set(address,src);
				break;

			/*******
			 * DEX *  -> Decrement X by one
			 *******/
			case 0xCA:
				X = (X - 1) & 0xff;
				SET_SIGN(X);
				SET_ZERO(X);
				break;

			/*******
			 * DEY *  -> Decrement Y by one
			 *******/
			case 0x88:
				Y = (Y - 1) & 0xff;
				SET_SIGN(Y);
				SET_ZERO(Y);
				break;

			/*******
			 * EOR *  -> Exclusive OR memory with accumulator
			 *******/
			case 0x49:
			case 0x45:
			case 0x55:
			case 0x40:
			case 0x5D:
			case 0x59:
			case 0x41:
			case 0x51:
				src ^= A;
				SET_SIGN(src);
				SET_ZERO(src);
				A = src;
				break;

			/*******
			 * INC *  -> Increment memory by one
			 *******/
			case 0xE6:
			case 0xF6:
			case 0xEE:
			case 0xFE:
				src = (src + 1) & 0xff;
				SET_SIGN(src);
				SET_ZERO(src);
				dev_mem_set(address, src);
				break;

			/*******
			 * INX *  -> Increment X by one
			 *******/
			case 0xE8:
				X = (X+1) & 0xFF;
				SET_SIGN(X);
				SET_ZERO(X);
				break;

			/*******
			 * INY *  -> Increment Y by one
			 *******/
			case 0xC8:
				Y = (Y+1) & 0xFF;
				SET_SIGN(Y);
				SET_ZERO(Y);
				break;

			/*******
			 * JMP *  -> Jump to a new location
			 *******/
			case 0x4C:
			case 0x6C:
				PC = address;
				break;

			/*******
			 * JSR *  -> Jump and save return address
			 *******/
			case 0x20:
				PC--;
				/* push return address onto the stack */
				PUSH((PC >> 8) & 0xFF);
				PUSH(PC & 0xFF);
				PC = address;
				break;

			/*******
			 * LDA *  -> Load A with memory
			 *******/
			case 0xA9:
			case 0xA5:
			case 0xB5:
			case 0xAD:
			case 0xBD:
			case 0xB9:
			case 0xA1:
			case 0xB1:
				SET_SIGN(src);
				SET_ZERO(src);
				A = src;
				break;

			/*******
			 * LDX *  -> Load X with memory
			 *******/
			case 0xA2:
			case 0xA6:
			case 0xB6:
			case 0xAE:
			case 0xBE:
				SET_SIGN(src);
				SET_ZERO(src);
				X = src;
				break;

			/*******
			 * LDY *  -> Load Y with memory
			 *******/
			case 0xA0:
			case 0xA4:
			case 0xB4:
			case 0xAC:
			case 0xBC:
				SET_SIGN(src);
				SET_ZERO(src);
				Y = src;
				break;

			/*******
			 * LSR *  -> Shift right one bit
			 *******/
			case 0x4A:
			case 0x46:
			case 0x56:
			case 0x4E:
			case 0x5E:
				SET_CARRY(src & 0x01);
				src >>= 1;
				SET_SIGN(src);
				SET_ZERO(src);
				if(opcode == 0x4A)
					A = src;
				else
					dev_mem_set(address, src);
				break;

			/*******
			 * NOP *  -> No operation
			 *******/
			case 0xEA:
			case 0x04:
				/* do nothing */
				break;

			/*******
			 * ORA *  -> Memory Or A
			 *******/
			case 0x09:
			case 0x05:
			case 0x15:
			case 0x0D:
			case 0x1D:
			case 0x19:
			case 0x01:
			case 0x11:
				src |= A;
				SET_SIGN(src);
				SET_ZERO(src);
				A = src;
				break;

			/*******
			 * PHA *  -> Push A onto stack
			 *******/
			case 0x48:
				src = A;
				PUSH(src);
				break;

			/*******
			 * PHP *  -> Push SP onto stack
			 *******/
			case 0x08:
				src = SP;
				PUSH(src);
				break;

			/*******
			 * PLA *  -> Pull A from stack
			 *******/
			case 0x68:
				src = PULL();
				SET_SIGN(src);
				SET_ZERO(src);
				A = src;
				break;

			/*******
			 * PLP *  -> Pull SP from stack
			 *******/
			case 0x28:
				src = PULL();
				SP = src;
				break;

			/*******
			 * ROL *  -> Rotate one bit left
			 *******/
			case 0x2A:
			case 0x26:
			case 0x36:
			case 0x2E:
			case 0x3E:
				src <<= 1;
				if(C != 0)
					src |= 0x1;
				C = (src > 0xff);
				src &= 0xff;
				SET_SIGN(src);
				SET_ZERO(src);
				if(opcode == 0x2A)
					A = src;
				else
					dev_mem_set(address, src);
				break;

			/*******
			 * ROR *  -> Rotate one bit right
			 *******/
			case 0x6A:
			case 0x66:
			case 0x76:
			case 0x6E:
			case 0x7E:
				if(C != 0)
					src |= 0x100;
				SET_CARRY(src & 0x01);
				src >>= 1;
				SET_SIGN(src);
				SET_ZERO(src);
				if(opcode == 0x6A)
					A = src;
				else
					dev_mem_set(address, src);
				break;

			/*******
			 * RTI *  -> Return from interrupt
			 *******/
			case 0x4D:
				src = PULL();
				SP = src;
				src = PULL();
				src |= (PULL() << 8);
				PC = src;
				break;

			/*******
			 * RTS *  -> Return from subroutine
			 *******/
			case 0x60:
				src = PULL();
				src += ((PULL()) << 8) + 1;
				PC = src;
				break;
				
			/*******
			 * SBC *  -> Substract with borrow (carry)
			 *******/
			case 0xE9:
			case 0xE5:
			case 0xF5:
			case 0xED:
			case 0xFD:
			case 0xF9:
			case 0xE1:
			case 0xF1:
				temp = A - src - ((C!=0) ? 0 : 1);
				SET_SIGN(temp);
				SET_ZERO(temp & 0xFF);
				SET_OVERFLOW(((A^temp)&0x80)&&((A^src)&0x80));
				if(D) 
				{
					if(((A&0xf)-((C!=0)?0:1))<(src&0xf))
						temp -= 6;
					if(temp>0x99)
						temp -= 0x60;
				}
				SET_CARRY(temp < 0x100);
				A = (temp & 0xFF);
				break;

			/*******
			 * SEC *  -> Set carry flag
			 *******/
			case 0x38:
				SET_CARRY((1));
				break;

			/*******
			 * SED *  -> Set decimal mode
			 *******/
			case 0xF8:
				SET_DECIMAL((1));
				break;

			/*******
			 * SEI *  -> Set interrupt disable status
			 *******/
			case 0x78:
				SET_INTERRUPT((1));
				break;

			/*******
			 * STA *  -> Store A into memory
			 *******/
			case 0x85:
			case 0x95:
			case 0x8D:
			case 0x9D:
			case 0x99:
			case 0x81:
			case 0x91:
				dev_mem_set(address, A);
				break;

			/*******
			 * STX *  -> Store X into memory
			 *******/
			case 0x86:
			case 0x96:
			case 0x8E:
				dev_mem_set(address, X);
				break;

			/*******
			 * STY *  -> Store Y into memory
			 *******/
			case 0x84:
			case 0x94:
			case 0x8C:
				dev_mem_set(address, Y);
				break;

			/*******
			 * TAX *  -> Transfer A to X
			 *******/
			case 0xAA:
				src = A;
				SET_SIGN(src);
				SET_ZERO(src);
				X = src;
				break;

			/*******
			 * TAY *  -> Transfer A to Y
			 *******/
			case 0xA8:
				src = A;
				SET_SIGN(src);
				SET_ZERO(src);
				Y = src;
				break;

			/*******
			 * TSX *  -> Transfer SP to X
			 *******/
			case 0xBA:
				src = SP;
				SET_SIGN(src);
				SET_ZERO(src);
				X = src;
				break;

			/*******
			 * TXA *  -> Transfer X to A
			 *******/
			case 0x8A:
				src = X;
				SET_SIGN(src);
				SET_ZERO(src);
				A = src;
				break;

			/*******
			 * TXS *  -> Transfer X to SP
			 *******/
			case 0x9A:
				src = X;
				SP = src;
				break;

			/*******
			 * TYA *  -> Transfer Y to A
			 *******/
			case 0x98:
				src = Y;
				SET_SIGN(src);
				SET_ZERO(src);
				A = src;
				break;
		}
	}

	*num_cycles = cycles;
	return -1;
}
