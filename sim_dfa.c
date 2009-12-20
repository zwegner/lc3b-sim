/* sim_latch.c */
/* The main file for simulating. This implements the pipeline itself, and
 * generally the very highest level of machine abstraction. */
/* This version of the file is the "latch" simulator.
 *
 * It works in a very simple way: take each stage of the pipeline in the
 * reverse order (try to retire an instruction first). Once a pipeline stage
 * is cleared, the next stage can try to run and push it's instruction along. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

/* Push an instruction into the next stage, if that unit is free */
#define PL_PUSH(from, to) \
	do { \
		if (to == NULL) { /* Try to push into next stage */ \
			to = from; \
			to->done = FALSE; \
			from = NULL; /* Clear stage for next inst. */ \
		} else \
			from->done = TRUE; /* Make sure we don't rerun this stage */ \
	} while (FALSE)

#define DEBUG_PRINT_INST(n) DEBUG_PRINT("****** " #n " %s at %p ******\n", \
					opcode_str[pl->n->opcode], pl->n)

#define MAXIMIZE(c, i) do { if ((i) > (c)) (c) = (i); } while (FALSE)

void run(STATUS *s, PIPELINE *pl)
{
	int x;

	for (x = 0; ; x++) {
		printf("inst %i-----------\n", x);

		/* Allocate a structure for this instruction, and also a list of
		 * previous instructions. */
		INST *i, *last[4];
		int y;

		/* Fill in the last instruction pointers. */
		for (y = 0; y < 4; y++)
			last[y] = NULL;
		for (y = 0; y < 4; y++) {
			if (x - y <= 0) /* Make sure we don't go before time T=0 */
				continue;

			int last_ptr = (pl->pl_ptr - y + PL_SIZE) % PL_SIZE;
			last[y] = &pl->pipeline[last_ptr];
			printf("last[%i]=%p\n", y, last[y]);
		}

		pl->pl_ptr = (pl->pl_ptr + 1) % PL_SIZE;
		i = &pl->pipeline[pl->pl_ptr];

		/* Run the entire instruction now. */
		run_fetch(s, i);
		run_read(s, i);
		run_exec(s, i);
		run_write(s, i);

		/* Calculate the earliest cycle we can fetch this instruction. This is
		 * either the cycle after the last instruction has moved to the read
		 * stage, or, if it was a branch, after it finished the write stage. */
		int fetch_cycle = 0;

		/* Baseline of last instruction */
		if (last[0] != NULL) {
			/* Fetching can only be done when the last read is done, so we can
			 * push the fetch through. */
			MAXIMIZE(fetch_cycle, last[0]->read_cycle);

			if (fetch_depends_on(i, last[0]))
				MAXIMIZE(fetch_cycle, last[0]->write_cycle);
		}
		/* No last instruction --> first instruction */
		else
			fetch_cycle = 0;

		/* Calculate the earliest cycle we can read the instruction. */
		int read_cycle = 0;

		MAXIMIZE(read_cycle, fetch_cycle + 1);

		if (last[0] != NULL) {
			MAXIMIZE(read_cycle, last[0]->exec_cycle);

			if (read_depends_on(i, last[0]))
				MAXIMIZE(read_cycle, last[0]->write_cycle);
			if (last[1] != NULL)
				if (read_depends_on(i, last[1]))
					MAXIMIZE(read_cycle, last[1]->write_cycle);
		}

		/* Calculate the earliest cycle we can exec the instruction. */
		int exec_cycle = 0;

		MAXIMIZE(exec_cycle, read_cycle + 1);

		if (last[0] != NULL)
			MAXIMIZE(exec_cycle, last[0]->write_cycle);

		/* Calculate the earliest cycle we can write the instruction. */
		int write_cycle = 0;

		MAXIMIZE(write_cycle, exec_cycle + 1);

		if (last[0] != NULL)
			MAXIMIZE(write_cycle, last[0]->write_cycle + 1);

		/* Store the cycle timings into the instruction so instructions that
		 * come after can be timed accurately. */
		i->fetch_cycle = fetch_cycle;
		i->read_cycle = read_cycle;
		i->exec_cycle = exec_cycle;
		i->write_cycle = write_cycle;

		printf("%s f=%i r=%i e=%i w=%i\n", opcode_str[i->opcode],
				fetch_cycle, read_cycle, exec_cycle, write_cycle);

		if (s->halt)
			break;
	}
	printf("machine halted after %i cycles.\n",
		pl->pipeline[pl->pl_ptr].write_cycle); /* HACK */
	for (x = 0; x < 8; x++)
		printf("R%i=0x%04X=%i\n", x, REGISTER(x) & 0xFFFF, REGISTER(x));
}
