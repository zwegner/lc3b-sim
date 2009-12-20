/* exec.c */
/* Functions for executing each instruction. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

#define EXEC_FN(name) void exec_##name(STATUS *s, INST *i, int inst, int PC)

EXEC_FN(add)
{
	i->result = i->op_1 + i->op_2;
}

EXEC_FN(and)
{
	i->result = i->op_1 & i->op_2;
}

EXEC_FN(xor)
{
	i->result = i->op_1 ^ i->op_2;
}

EXEC_FN(br)
{
	i->result = i->op_1 & i->imm;

	/* i->mem_ptr already contains target of branch, so only change it if the
	 * branch is not taken. */
	if (!i->result)
		i->mem_ptr = s->PC; /* HACK: relies on no other instructions being
							   executed between fetch of BR and now. Which is
							   the case, but maybe not later with branch pred. */
}

EXEC_FN(jmp_ldb_ldw_lea)
{
	/* Nothing */
	i->result = i->op_1;
}

/* TODO: add some stuff here */
EXEC_FN(jsr)
{
}

/* TODO: something */
EXEC_FN(rti)
{
}

EXEC_FN(shf)
{
	/* The most complicated function in here. Deals with two steering bits. */
	if (EXT_BIT(i->inst, 4)) {
		if (EXT_BIT(i->inst, 5))
			i->result = i->op_1 >> i->op_2;
		else
			i->result = (unsigned)i->op_1 >> i->op_2;
	} else
		i->result = i->op_1 << i->op_2;
}

EXEC_FN(stb_stw)
{
	/* Nothing */
}

/* TODO: something */
EXEC_FN(trap)
{
	switch (i->op_1) {
		/* GETC */
		case 0x20:
			i->result = getchar();
			break;
		/* OUT */
		case 0x21:
			putchar(i->op_2 & 0x7F);
			break;
		/* PUTS */
		case 0x22:
			{
				UWORD ptr;
				/* HACK: we don't notice a memory dependence on the memory we
				 * are writing, but by the time we execute all writes are
				 * done anyways. */
				for (ptr = i->op_2; MEMORY(ptr) != 0; ptr++)
					putchar(MEMORY(ptr));
			}
			break;
	}
}

void run_exec(STATUS *s, INST *i)
{
	switch (i->opcode) {
		case OP_ADD:
			exec_add(s, i, i->inst, s->PC);
			break;
		case OP_AND:
			exec_and(s, i, i->inst, s->PC);
			break;
		case OP_XOR:
			exec_xor(s, i, i->inst, s->PC);
			break;
		case OP_BR:
			exec_br(s, i, i->inst, s->PC);
			break;
		case OP_JMP:
		case OP_LDB:
		case OP_LDW:
		case OP_LEA:
			exec_jmp_ldb_ldw_lea(s, i, i->inst, s->PC);
			break;
		case OP_JSR:
			exec_jsr(s, i, i->inst, s->PC);
			break;
		case OP_RTI:
			exec_rti(s, i, i->inst, s->PC);
			break;
		case OP_SHF:
			exec_shf(s, i, i->inst, s->PC);
			break;
		case OP_STB:
		case OP_STW:
			exec_stb_stw(s, i, i->inst, s->PC);
			break;
		case OP_TRAP:
			exec_trap(s, i, i->inst, s->PC);
			break;
		default:
			printf("exec bad opcode = %i\n", i->opcode);
			exit(EXIT_FAILURE);
	}
}
