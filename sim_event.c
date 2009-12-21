/* sim_event.c */
/* The main file for simulating. This implements the pipeline itself, and
 * generally the very highest level of machine abstraction. */
/* This version of the file is the event simulator.
 *
 * It works like this: take the instructions one at a time and execute them.
 * As they are being executed, we create a timeline of when each stage of
 * each instruction is executed. It is similar to the DFA simulator, but can
 * handle the cases where instructions can affect instructions that came before
 * it. The extra cost is memory, so we only create a timeline for a limited
 * period before we "flush" it. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

#define EVQ_SIZE (32)

#define MAXIMIZE(c, i) do { if ((i) > (c)) (c) = (i); } while (FALSE)

INST *event_queue[EVQ_SIZE][PL_SIZE];
int cycles;

void shift_queue(INST *i)
{
	int x, y, z;

	/* Clear this instruction pointer from the event queue. If it's in
	 * there, it's from an old instruction that won't matter anymore. */
	for (x = 0; x < EVQ_SIZE; x++)
		for (y = 0; y < PL_SIZE; y++)
			if (event_queue[x][y] == i)
				event_queue[x][y] = NULL;

	/* Find the first stage in the event queue with active instructions. */
	int first = 0;
	for (x = 0; x < EVQ_SIZE; x++) {
		for (y = 0; y < PL_SIZE; y++) {
			if (event_queue[x][y] != NULL) {
				first = x;
				goto found_first;
			}
		}
	}
found_first:

	/* We shift the event queue down to make room for more instructions, and
	 * to keep the queue logic simple. */
	for (x = 0, y = first; y < EVQ_SIZE; x++, y++)
		for (z = 0; z < PL_SIZE; z++)
			event_queue[x][z] = event_queue[y][z];

	/* Increment the cycle count. */
	cycles += first;

	printf("Adding %i cycles from %s.\n", first, opcode_str[i->opcode]);
}

void run(STATUS *s, PIPELINE *pl)
{
	INST *i;
	int x, y;

	cycles = 0;

	for (x = 0; x < EVQ_SIZE; x++)
		for (y = 0; y < PL_SIZE; y++)
			event_queue[x][y] = NULL;

	for (x = 0; ; x++) {
		DEBUG_PRINT("inst %i-----------\n", x);

		int last_pl_ptr = pl->pl_ptr;

		pl->pl_ptr = (pl->pl_ptr + 1) % PL_SIZE;
		i = &pl->pipeline[pl->pl_ptr];

		/* Run the entire instruction now. */
		run_fetch(s, i);
		run_read(s, i);
		run_exec(s, i);
		run_write(s, i);

		shift_queue(i);

		/* Insert the fetch stage into the event queue. */
		int fetch = 0;
		for (y = 0; y < EVQ_SIZE; y++) {
			/* If there was a write to the PC, we have to fetch after it. */
			if (event_queue[y][PL_WRITE] != NULL &&
					fetch_depends_on(i, event_queue[y][PL_WRITE]))
				MAXIMIZE(fetch, y);

			/* Wait until the fetch unit is free. */
			if (event_queue[y][PL_FETCH] != NULL)
				MAXIMIZE(fetch, y + 1);

			/* Make sure a read isn't stalling in the read unit. */
			if (event_queue[y][PL_READ] != NULL)
				MAXIMIZE(fetch, y);
		}

		event_queue[fetch][PL_FETCH] = i;

		/* Insert the read stage into the event queue. */
		int read = fetch + 1;
		for (y = 0; y < EVQ_SIZE; y++) {
			/* Wait until our dependencies have been written. */
			if (event_queue[y][PL_WRITE] != NULL &&
					read_depends_on(i, event_queue[y][PL_WRITE]))
				MAXIMIZE(read, y);

			/* Wait until the read unit is free. */
			if (event_queue[y][PL_READ] != NULL)
				MAXIMIZE(read, y + 1);
		}

		event_queue[read][PL_READ] = i;

		/* Insert the exec stage into the event queue. */
		int exec = read + 1;
		for (y = 0; y < EVQ_SIZE; y++) {
			/* Wait until the exec unit is free. */
			if (event_queue[y][PL_EXEC] != NULL)
				MAXIMIZE(exec, y + 1);
		}

		event_queue[exec][PL_EXEC] = i;

		/* Insert the write stage into the event queue. */
		int write = exec + 1;
		for (y = 0; y < EVQ_SIZE; y++) {
			/* Wait until the write unit is free. */
			if (event_queue[y][PL_WRITE] != NULL)
				MAXIMIZE(write, y + 1);
		}

		event_queue[write][PL_WRITE] = i;

#define PRINT_NICE(s) \
		if (event_queue[y][s] == NULL) printf("|----| "); \
		else printf("|%4s| ", opcode_str[event_queue[y][s]->opcode]);

		for (y = 0; y <= write; y++) {
			PRINT_NICE(PL_FETCH);
			PRINT_NICE(PL_READ);
			PRINT_NICE(PL_EXEC);
			PRINT_NICE(PL_WRITE);
			printf("\n");
		}

		if (s->halt)
			break;
	}

	/* "Flush out" the event queue to accurately count the cycles. */
	for (x = EVQ_SIZE - 1; x >= 0; x--) {
		if (event_queue[x][PL_FETCH] != NULL ||
				event_queue[x][PL_READ] != NULL ||
				event_queue[x][PL_EXEC] != NULL ||
				event_queue[x][PL_WRITE] != NULL) {
			cycles += x;
			break;
		}
	}

	printf("machine halted after %i cycles.\n", cycles);
	for (x = 0; x < 8; x++)
		printf("R%i=0x%04X=%i\n", x, REGISTER(x) & 0xFFFF, REGISTER(x));
}
