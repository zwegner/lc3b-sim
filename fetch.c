/* fetch.c */
/* Functions for fetching and decoding instructions. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

#define FETCH_FN(name) void fetch_##name(INST *i, int inst, int PC)

FETCH_FN(add_and_xor)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->src_reg_1 = EXT_BITS(inst, 8, 6);

	i->has_imm = EXT_BIT(inst, 5);

	if (i->has_imm)
		i->imm = SEXT(inst, 5);
	else
		i->src_reg_2 = EXT_BITS(inst, 2, 0);

	i->dep_mask = DM_DR | DM_SR1 | DM_WCC | (i->has_imm ? 0 : DM_SR2);
}

FETCH_FN(br)
{
	i->imm = EXT_BITS(inst, 11, 9);
	i->mem_ptr = PC + SEXT(inst, 9);

	i->dep_mask = DM_PC | DM_RCC;
}

FETCH_FN(jmp)
{
	i->src_reg_1 = EXT_BITS(inst, 8, 6);

	i->dep_mask = DM_SR1 | DM_PC;
}

FETCH_FN(jsr)
{
	i->has_imm = EXT_BIT(inst, 11);

	if (i->has_imm)
		i->imm = PC + SEXT(inst, 11);
	else
		i->src_reg_1 = EXT_BITS(inst, 8, 6);

	i->dep_mask = DM_PC | (i->has_imm ? 0 : DM_SR1);
}

FETCH_FN(ldb_ldw)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->src_reg_1 = EXT_BITS(inst, 8, 6);
	i->imm = SEXT(inst, 6);

	i->dep_mask = DM_DR | DM_SR1 | DM_RM | DM_WCC;
}

FETCH_FN(lea)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->mem_ptr = PC + (SEXT(inst, 9) << 1);

	i->dep_mask = DM_DR | DM_WCC;
}

/* TODO: change dependencies/sources to have mem/R6 */
FETCH_FN(rti)
{
	i->dep_mask = DM_PC;
}

FETCH_FN(shf)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->src_reg_1 = EXT_BITS(inst, 8, 6);
	i->imm = ZEXT(inst, 6);

	i->dep_mask = DM_DR | DM_SR1 | DM_WCC;
}

FETCH_FN(stb_stw)
{
	i->src_reg_1 = EXT_BITS(inst, 11, 9);
	i->src_reg_2 = EXT_BITS(inst, 8, 6);
	i->imm = SEXT(inst, 6);

	i->dep_mask = DM_SR1 | DM_SR2 | DM_WM;
}

FETCH_FN(trap)
{
	i->imm = ZEXT(inst, 8);

	i->dep_mask = DM_PC;

	switch (i->imm) {
		case 0x20:
			i->dst_reg = 0;
			i->dep_mask |= DM_DR;
			break;
		case 0x21:
		case 0x22:
			i->src_reg_1 = 0;
			i->dep_mask |= DM_SR1;
			break;
	}
}

void run_fetch(STATUS *s, INST *i)
{
	UWORD IR;

	/* Fetch the instruction by just reading the memory location. */
	IR = MEMORY(s->PC) + (MEMORY(s->PC + 1) << 8);

	/* Increment PC. */
	s->PC += 2;

	/* Set up INST struct. */
	i->inst = IR;
	i->opcode = EXT_BITS(IR, 15, 12);

	/* Instruction-specific processing. */
	switch (i->opcode) {
		case OP_ADD:
		case OP_AND:
		case OP_XOR:
			fetch_add_and_xor(i, IR, s->PC);
			break;
		case OP_BR:
			fetch_br(i, IR, s->PC);
			break;
		case OP_JMP:
			fetch_jmp(i, IR, s->PC);
			break;
		case OP_JSR:
			fetch_jsr(i, IR, s->PC);
			break;
		case OP_LDB:
		case OP_LDW:
			fetch_ldb_ldw(i, IR, s->PC);
			break;
		case OP_LEA:
			fetch_lea(i, IR, s->PC);
			break;
		case OP_RTI:
			fetch_rti(i, IR, s->PC);
			break;
		case OP_SHF:
			fetch_shf(i, IR, s->PC);
			break;
		case OP_STB:
		case OP_STW:
			fetch_stb_stw(i, IR, s->PC);
			break;
		case OP_TRAP:
			fetch_trap(i, IR, s->PC);
			break;
		default:
			printf("fetch bad opcode = %i\n", i->opcode);
			exit(EXIT_FAILURE);
	}
}
