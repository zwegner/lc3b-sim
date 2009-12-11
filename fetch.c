/* fetch.c */
/* Functions for fetching and decoding instructions. */

#include "lc3.h"

#define FETCH_FN(name) void name(INST *i, int inst, int PC)

/* ADD/AND/XOR have the same bytecodes, basically. We can fetch them
 * the same from the point of view of operands and dependencies. */
FETCH_FN(add_and_xor)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->src_reg_1 = EXT_BITS(inst, 8, 6);

	BOOL immediate = EXT_BIT(inst, 5);

	if (immediate)
		i->imm = SEXT(inst, 5);
	else
		i->src_reg_2 = EXT_BITS(inst, 2, 0);

	i->dep_mask = DM_DR | DM_SR1 | DM_CC | (immediate ? 0 : DM_SR2);
}

FETCH_FN(br)
{
	i->imm = EXT_BITS(inst, 11, 9);
	i->ptr = PC+1 + SEXT(inst, 9);

	i->dep_mask = DM_BR;
}

FETCH_FN(jmp)
{
	i->src_reg_1 = EXT_BITS(inst, 8, 6);

	i->dep_mask = DM_SR1 | DM_BR;
}

FETCH_FN(jsr)
{
	BOOL reg = EXT_BIT(inst, 11);

	if (reg)
		i->imm = PC+1 + SEXT(inst, 11);
	else
		i->src_reg_1 = EXT_BITS(inst, 8, 6);

	i->dep_mask = DM_BR | (reg ? 0 : DM_SR1);
}

/* LDB/LDW the same from fetching/dependency POV */
FETCH_FN(ldb_ldw)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->src_reg_1 = EXT_BITS(inst, 8, 6);
	i->imm = SEXT(inst, 6);

	i->dep_mask = DM_DR | DM_SR1 | DM_RM | DM_CC;
}

FETCH_FN(lea)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->ptr = PC+1 + SEXT(inst, 9);

	i->dep_mask = DM_DR | DM_CC;
}

/* Easy shit */
FETCH_FN(rti)
{
	i->dep_mask = DM_BR;
}

FETCH_FN(shf)
{
	i->dst_reg = EXT_BITS(inst, 11, 9);
	i->src_reg_1 = EXT_BITS(inst, 8, 6);
	i->imm = ZEXT(inst, 6);

	i->dep_mask = DM_DR | DM_SR1 | DM_CC;
}

/* LDB/LDW the same from fetching/dependency POV */
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

	i->dep_mask = DM_BR;
}

void run_fetch(STATUS *s, INST *i)
{
	switch (EXT_BITS(i->inst, 15, 12)) {
		case 0x1:
		case 0x5:
		case 0x9:
			fetch_add_and_xor(inst, IR, PC);
			break;
		case 0x0:
			fetch_br(inst, IR, PC);
			break;
		case 0xC:
			fetch_jmp(inst, IR, PC);
			break;
		case 0x4:
			fetch_jsr(inst, IR, PC);
			break;
		case 0x2:
		case 0x6:
			fetch_ldb_ldw(inst, IR, PC);
			break;
		case 0xE:
			fetch_lea(inst, IR, PC);
			break;
		case 0x8:
			fetch_rti(inst, IR, PC);
			break;
		case 0xD:
			fetch_shf(inst, IR, PC);
			break;
		case 0x3:
		case 0x7:
			fetch_stb_stw(inst, IR, PC);
			break;
		case 0xF:
			fetch_trap(inst, IR, PC);
			break;
	}

	/* Increment PC. */
	s->PC++;
}
