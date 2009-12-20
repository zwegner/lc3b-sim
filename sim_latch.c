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

void run(STATUS *s, PIPELINE *pl)
{
	int x;

	for (x = 0; ; x++) {
		DEBUG_PRINT("----------------------cycle %03i----------------------\n",
			x);

		/* Push new instruction if pipeline can take it */
		if (pl->fetch == NULL) {
			INST *i;
			pl->pl_ptr = (pl->pl_ptr + 1) % PL_SIZE;
			i = &pl->pipeline[pl->pl_ptr];

			/* Should always work! */
			PL_PUSH(i, pl->fetch);
		}

		/* WRITE */
		if (pl->write != NULL) {
			/* Write back anything we need to. No need to wait. */
			run_write(s, pl->write);
			DEBUG_PRINT_INST(write);

			/* Check if the machine has halted. */
			if (s->halt)
				break;

			/* Clear dependencies */
			clear_write_deps(pl, pl->write);

			/* Clear write stage for next inst. No need to push. */
			pl->write = NULL;
		}

		/* EXEC */
		if (pl->exec != NULL) {
			/* Execute. We don't have to check dependencies, and since the
			 * write stage doesn't have to either, we don't have to wait. */
			run_exec(s, pl->exec);
			DEBUG_PRINT_INST(exec);

			/* Try to push exec -> write */
			PL_PUSH(pl->exec, pl->write);
		}

		/* READ */
		if (pl->read != NULL) {
			BOOL wait = FALSE;

			/* Resolve dependencies */
			if (check_read_deps(s, pl, pl->read))
				wait = TRUE;

			if (!wait) {
				if (!pl->read->done) {
					run_read(s, pl->read);
					DEBUG_PRINT_INST(read);
				}

				/* If we're modifying something, set up dependency flags don't
				 * try and read stale data. */
				set_read_deps(pl, pl->read);

				/* Try to push read -> exec */
				PL_PUSH(pl->read, pl->exec);
			}
		}

		/* FETCH */
		if (pl->fetch != NULL) {
			BOOL wait = FALSE;

			/* Check dependencies on fetch. Just BR/JMP now, since we don't know
			 * the new PC until the last inst. is done. */
			if (check_fetch_deps(pl, pl->fetch))
				wait = TRUE;

			/* If we're not waiting on any dependencies, keep running */
			if (!wait) {
				if (!pl->fetch->done) {
					run_fetch(s, pl->fetch);
					DEBUG_PRINT_INST(fetch);
				}

				/* Try to push fetch -> read */
				PL_PUSH(pl->fetch, pl->read);

				/* HACK: set the mem_ptr_known flag on read, so we can check
				 * for dependencies cleanly. */
				pl->read->mem_ptr_known = FALSE;
			}
		}

		/* Print debug info. */
#define PRINT_NICE(s) \
		if (pl->s == NULL) printf("|----| "); \
		else printf("|%4s| ", opcode_str[pl->s->opcode]);

		PRINT_NICE(fetch);
		PRINT_NICE(read);
		PRINT_NICE(exec);
		PRINT_NICE(write);
		printf("\n");
	}
	printf("machine halted after %i cycles.\n", x);
	for (x = 0; x < 8; x++)
		printf("R%i=0x%04X=%i\n", x, REGISTER(x) & 0xFFFF, REGISTER(x));
}
