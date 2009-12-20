/* write.c */
/* Functions for writing back any results of each instruction. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

#define WRITE_FN(name) void write_##name(STATUS *s, INST *i, int inst, int PC)

WRITE_FN(add_and_xor_shf)
{
	REGISTER(i->dst_reg) = i->result;
}

WRITE_FN(br_jmp)
{
	s->PC = i->mem_ptr;
}

WRITE_FN(ldb_ldw_lea)
{
	REGISTER(i->dst_reg) = i->result;
}

/* TODO: add some stuff here */
WRITE_FN(jsr)
{
	s->PC = i->mem_ptr;
}

/* TODO: something */
WRITE_FN(rti)
{
}

WRITE_FN(stb)
{
	MEMORY(i->mem_ptr) = i->op_1;
}

WRITE_FN(stw)
{
	MEMORY(i->mem_ptr) = i->op_1;
	MEMORY(i->mem_ptr+1) = i->op_1 >> 8;
}

/* TODO: something */
WRITE_FN(trap)
{
	if (i->op_1 == 0x25)
		s->halt = TRUE;
	else if (i->op_1 == 0x20)
		REGISTER(i->dst_reg) = i->result;
}

void run_write(STATUS *s, INST *i)
{
	switch (i->opcode) {
		case OP_ADD:
		case OP_AND:
		case OP_XOR:
		case OP_SHF:
			write_add_and_xor_shf(s, i, i->inst, s->PC);
			break;
		case OP_BR:
		case OP_JMP:
			write_br_jmp(s, i, i->inst, s->PC);
			break;
		case OP_LDB:
		case OP_LDW:
		case OP_LEA:
			write_ldb_ldw_lea(s, i, i->inst, s->PC);
			break;
		case OP_JSR:
			write_jsr(s, i, i->inst, s->PC);
			break;
		case OP_RTI:
			write_rti(s, i, i->inst, s->PC);
			break;
		case OP_STB:
			write_stb(s, i, i->inst, s->PC);
			break;
		case OP_STW:
			write_stw(s, i, i->inst, s->PC);
			break;
		case OP_TRAP:
			write_trap(s, i, i->inst, s->PC);
			break;
		default:
			printf("write bad opcode = %i\n", i->opcode);
			exit(EXIT_FAILURE);
	}
}
