/* dep_dfa.c */
/* Various routines for handling dependencies in the DFA simulator. */

#include "lc3.h"
#include <stdio.h>

//#define DEBUG_DEP

#ifdef DEBUG_DEP
#define DEBUG_DEP_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#else
#define DEBUG_DEP_PRINT(...)
#endif

/* Check whether i1 depends on i2 being written back before it can be fetched. */
BOOL fetch_depends_on(INST *i1, INST *i2)
{
	if (i2->dep_mask & DM_PC)
		return TRUE;

	return FALSE;
}

/* Check whether i1's read stage depends on i2's write stage. */
BOOL read_depends_on(INST *i1, INST *i2)
{
	/* See if the instruction wrote to a register. */
	if (i2->dep_mask & DM_DR) {
		int reg = i2->dst_reg;

		if (i1->dep_mask & DM_SR1) {
			if (i1->src_reg_1 == reg)
			return TRUE;
		}
		if (i1->dep_mask & DM_SR2) {
			if (i1->src_reg_2 == reg)
			return TRUE;
		}
	}

	/* See if the instruction wrote to memory. */
	if (i2->dep_mask & DM_WM && i1->dep_mask & DM_RM) {
		if (i1->mem_ptr == i2->mem_ptr)
			return TRUE;
	}

	return FALSE;
}
