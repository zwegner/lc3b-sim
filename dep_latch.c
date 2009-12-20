/* dep.c */
/* Various routines for handling dependencies in the pipeline. */

#include "lc3.h"
#include <stdio.h>

//#define DEBUG_DEP

#ifdef DEBUG_DEP
#define DEBUG_DEP_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#else
#define DEBUG_DEP_PRINT(...)
#endif

/* Try and find a memory address in the pipeline. */
int *find_mem_dep(PIPELINE *pl, int addr)
{
	int x;

	/* Clear low bit to deal with byte/word accesses overlapping. */
	addr &= ~0x1;

	for (x = 0; x < PL_SIZE; x++) {
		if (pl->mem_dep[x] == addr)
			return &pl->mem_dep[x];
	}

	return NULL;
}

/* See if we can fetch the next instruction.  Just check if the last
 * instruction was a branch/jmp. */
BOOL check_fetch_deps(PIPELINE *pl, INST *fetch)
{
	if (pl->branching)
		return TRUE;
}

BOOL calc_mem_ptr(STATUS *s, PIPELINE *pl, INST *read)
{
	unsigned int addr;

	/* Make sure we can safely read the base register. */
	if (pl->reg_dep[read->src_reg_1] > 0)
		return FALSE;

	/* Get base address. */
	addr = s->reg[read->src_reg_1];

	/* Check whether the instruction is LDW or LDB. */
	if (EXT_BITS(read->inst, 15, 12) == 0x6)
		addr += read->imm << 1;
	else
		addr += read->imm;

	read->mem_ptr = addr;
	read->mem_ptr_known = TRUE;

	return TRUE;
}

/* Check if we can read the operands of an instruction. */
BOOL check_read_deps(STATUS *s, PIPELINE *pl, INST *read)
{
	DEBUG_DEP_PRINT("checking read deps, mask=%02X\n", read->dep_mask);

	/* Check both source registers. If we're reading them, make sure nobody
	 * is going to modify them. */
	if (read->dep_mask & DM_SR1) {
		DEBUG_DEP_PRINT("%s checking dep on R%i\n", opcode_str[read->opcode],
				read->src_reg_1);
		if (pl->reg_dep[read->src_reg_1] > 0)
			return TRUE;
	}

	if (read->dep_mask & DM_SR2) {
		DEBUG_DEP_PRINT("%s checking dep on R%i(S2)\n", opcode_str[read->opcode],
			read->src_reg_2);
		if (pl->reg_dep[read->src_reg_2] > 0)
			return TRUE;
	}

	/* Check memory dependency. We have to be careful, since we might not
	 * know the memory address yet. */
	if (read->dep_mask & DM_RM) {
		if (!read->mem_ptr_known) {
			if (!calc_mem_ptr(s, pl, read))
				return TRUE;
		}

		if (find_mem_dep(pl, read->mem_ptr) != NULL)
			return TRUE;
	}

	return FALSE;
}

/* After we've read an instruction, set up any dependencies on things
 * we will write, so they won't be read until we are done. */
void set_read_deps(PIPELINE *pl, INST *read)
{
	/* We're branching, so don't execute anything until we're done. */
	if (read->dep_mask & DM_PC)
		pl->branching = TRUE;

	if (read->dep_mask & DM_DR) {
		pl->reg_dep[read->dst_reg]++;
		DEBUG_DEP_PRINT("%s setting dep on R%i\n", opcode_str[read->opcode],
				read->dst_reg);
	}

	if (read->dep_mask & DM_WM)
		*find_mem_dep(pl, 0) = read->mem_ptr;
}

/* We finished a write, so clear any dependencies that we created. */
void clear_write_deps(PIPELINE *pl, INST *write)
{
	/* If this was a branch, we've now written to PC. */
	if (write->dep_mask & DM_PC)
		pl->branching = FALSE;

	/* If this was a write to mem, clear the dependency on the address. */
	if (write->dep_mask & DM_WM) {
		int *mem = find_mem_dep(pl, write->mem_ptr);

		if (mem == NULL)
			printf("FATAL ERROR: mem write addr not found in mem_dep[].\n");

		/* Clear out address. */
		*mem = 0;
	}

	/* If this was a write to a register, clear the dependencies on each
	 * register that has been written. */
	if (write->dep_mask & DM_DR) {
		int *reg = &pl->reg_dep[write->dst_reg];

		DEBUG_DEP_PRINT("%s clearing dep on R%i\n", opcode_str[write->opcode],
				write->dst_reg);

		/* Decrement reference count. */
		(*reg)--;
	}
}
