/* sim_dfa.c */
/* The main file for simulating. This implements the pipeline itself, and
 * generally the very highest level of machine abstraction. */
/* This version of the file is the "DFA", or Data Flow Analysis, simulator.
 *
 * It works like this: execute the instructions one at a time, not actually
 * simulating the pipeline. After each instruction, we look back at the
 * instructions before it that might have affected it and calculate the cycle
 * timings for each pipeline stage. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

#define MAXIMIZE(c, i) do { if ((i) > (c)) (c) = (i); } while (FALSE)

void run(STATUS *s, PIPELINE *pl)
{
	int x;

	for (x = 0; ; x++) {
		DEBUG_PRINT("inst %i-----------\n", x);

		/* Allocate a structure for this instruction, and also a list of
		 * previous instructions. */
#define INST_LOOKBACK (2)
		INST *i, *last[INST_LOOKBACK];
		int y;

		/* Fill in the last instruction pointers. */
		for (y = 0; y < INST_LOOKBACK; y++)
			last[y] = NULL;
		for (y = 0; y < INST_LOOKBACK; y++) {
			if (x - y <= 0) /* Make sure we don't go before time T=0 */
				continue;

			int last_ptr = (pl->pl_ptr - y + PL_SIZE) % PL_SIZE;
			last[y] = &pl->pipeline[last_ptr];
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
			MAXIMIZE(fetch_cycle, last[0]->stage_cycle[PL_READ]);

			if (fetch_depends_on(i, last[0]))
				MAXIMIZE(fetch_cycle, last[0]->stage_cycle[PL_WRITE]);
		}
		/* No last instruction --> first instruction */
		else
			fetch_cycle = 0;

		/* Calculate the earliest cycle we can read the instruction. */
		int read_cycle = 0;

		MAXIMIZE(read_cycle, fetch_cycle + 1);

		if (last[0] != NULL) {
			MAXIMIZE(read_cycle, last[0]->stage_cycle[PL_EXEC]);

			if (read_depends_on(i, last[0]))
				MAXIMIZE(read_cycle, last[0]->stage_cycle[PL_WRITE]);
			if (last[1] != NULL)
				if (read_depends_on(i, last[1]))
					MAXIMIZE(read_cycle, last[1]->stage_cycle[PL_WRITE]);
		}

		/* Calculate the earliest cycle we can exec the instruction. */
		int exec_cycle = 0;

		MAXIMIZE(exec_cycle, read_cycle + 1);

		if (last[0] != NULL)
			MAXIMIZE(exec_cycle, last[0]->stage_cycle[PL_WRITE]);

		/* Calculate the earliest cycle we can write the instruction. */
		int write_cycle = 0;

		MAXIMIZE(write_cycle, exec_cycle + 1);

		if (last[0] != NULL)
			MAXIMIZE(write_cycle, last[0]->stage_cycle[PL_WRITE] + 1);

		/* Store the cycle timings into the instruction so instructions that
		 * come after can be timed accurately. */
		i->stage_cycle[PL_FETCH] = fetch_cycle;
		i->stage_cycle[PL_READ] = read_cycle;
		i->stage_cycle[PL_EXEC] = exec_cycle;
		i->stage_cycle[PL_WRITE] = write_cycle;

		printf("%s f=%i r=%i e=%i w=%i\n", opcode_str[i->opcode],
				fetch_cycle, read_cycle, exec_cycle, write_cycle);

		if (s->halt)
			break;
	}
	printf("machine halted after %i cycles.\n",
		pl->pipeline[pl->pl_ptr].stage_cycle[PL_WRITE]); /* HACK */
	for (x = 0; x < 8; x++)
		printf("R%i=0x%04X=%i\n", x, REGISTER(x) & 0xFFFF, REGISTER(x));
}
