/* fetch.c */
/* Functions for reading the operands of instructions. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

#define READ_FN(name) void read_##name(STATUS *s, INST *i, int inst, int PC)

READ_FN(add_and_xor)
{
	i->op_1 = REGISTER(i->src_reg_1);

	if (i->has_imm)
		i->op_2 = i->imm;
	else
		i->op_2 = REGISTER(i->src_reg_2);
}

READ_FN(br)
{
	i->op_1 = EXT_BITS(s->PSR, 2, 0);
}

READ_FN(jmp)
{
}

READ_FN(jsr)
{
	if (i->has_imm)
		i->mem_ptr = i->imm;
	else
		i->mem_ptr = REGISTER(i->src_reg_1);
}

READ_FN(ldb)
{
	int addr;

	addr = REGISTER(i->src_reg_1) + i->imm;

	i->mem_ptr = addr;
	i->op_1 = SEXT(MEMORY(addr), 8);
}

READ_FN(ldw)
{
	int addr;

	addr = REGISTER(i->src_reg_1) + i->imm * 2;

	i->mem_ptr = addr;
	i->op_1 = MEMORY(addr) | MEMORY(addr+1) << 8;
}

READ_FN(lea)
{
	/* Probably could've done this in fetch_lea(), but this is a bit
	 * more consistent. */
	i->op_1 = i->mem_ptr;
}

/* TODO: Probably need to add some R6 stuff here... */
READ_FN(rti)
{
}

READ_FN(shf)
{
	i->op_1 = REGISTER(i->src_reg_1);
	i->op_2 = EXT_BITS(i->imm, 3, 0);
}

READ_FN(stb)
{
	i->op_1 = REGISTER(i->src_reg_1);

	i->mem_ptr = REGISTER(i->src_reg_2) + i->imm;
}

READ_FN(stw)
{
	i->op_1 = REGISTER(i->src_reg_1);

	i->mem_ptr = REGISTER(i->src_reg_2) + i->imm * 2;
}

/* TODO: Do something with R7? */
READ_FN(trap)
{
	i->op_1 = i->imm;

	if (i->op_1 == 0x21 || i->op_1 == 0x22)
		i->op_2 = REGISTER(i->src_reg_1); /* Always R0... */
}

void run_read(STATUS *s, INST *i)
{
	switch (i->opcode) {
		case OP_ADD:
		case OP_AND:
		case OP_XOR:
			read_add_and_xor(s, i, i->inst, s->PC);
			break;
		case OP_BR:
			read_br(s, i, i->inst, s->PC);
			break;
		case OP_JMP:
			read_jmp(s, i, i->inst, s->PC);
			break;
		case OP_JSR:
			read_jsr(s, i, i->inst, s->PC);
			break;
		case OP_LDB:
			read_ldb(s, i, i->inst, s->PC);
			break;
		case OP_LDW:
			read_ldw(s, i, i->inst, s->PC);
			break;
		case OP_LEA:
			read_lea(s, i, i->inst, s->PC);
			break;
		case OP_RTI:
			read_rti(s, i, i->inst, s->PC);
			break;
		case OP_SHF:
			read_shf(s, i, i->inst, s->PC);
			break;
		case OP_STB:
			read_stb(s, i, i->inst, s->PC);
			break;
		case OP_STW:
			read_stw(s, i, i->inst, s->PC);
			break;
		case OP_TRAP:
			read_trap(s, i, i->inst, s->PC);
			break;
		default:
			printf("read bad opcode = %i\n", i->opcode);
			exit(EXIT_FAILURE);
	}
}
