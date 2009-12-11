void parse_file(char *fname)
{
	FILE *file;
	while (!fgets()) {
	}
}

PIPELINE pl[1];

void pl_init(void)
{
	int x;

	pl->pl_ptr = 0;
	pl->branching = FALSE;

	pl->fetch = NULL;
	pl->decode = NULL;
	pl->exec = NULL;
	pl->write = NULL;

	for (x = 0; x < REG_COUNT; x++)
		pl->reg_dep[x] = 0;

	for (x = 0; x < PL_SIZE; x++)
		pl->mem_dep[x] = 0;
}

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

/* Try to advance the pipeline by one cycle in reverse order. */
void run_cycle(void)
{
	/* WRITE */
	if (pl->write != NULL) {
		/* Write back anything we need to. No need to wait. */
		run_write(pl->write);

		/* Clear dependencies */
		clear_write_deps(pl, pl->write);

		/* Clear write stage for next inst. No need to push. */
		pl->write = NULL;
	}

	/* EXEC */
	if (pl->exec != NULL) {
		/* Execute. We don't have to check dependencies, and since the
		 * write stage doesn't have to either, we don't have to wait. */
		run_exec(pl->exec);

		/* Try to push exec -> write */
		PL_PUSH(pl->exec, pl->write);
	}

	/* READ */
	if (pl->read != NULL) {
		BOOL wait = FALSE;

		/* Resolve dependencies */
		if (check_read_deps(pl, pl->read))
			wait = TRUE;

		if (!wait) {
			if (!pl->read->done)
				run_read(pl->read);

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
			if (!pl->fetch->done)
				run_fetch(pl->fetch);

			/* Try to push fetch -> read */
			PL_PUSH(pl->fetch, pl->read);

			/* HACK: set the mem_ptr_known flag on read, so we can check
			 * for dependencies cleanly. */
			pl->read->mem_ptr_known = FALSE;
		}
	}

	/* Push new instruction if pipeline can take it */
	if (pl->fetch == NULL) {
		INST *i;
		pl->pl_ptr = (pl->pl_ptr + 1) % PL_SIZE;
		i = &pl->pipeline[pl->pl_ptr];

		/* Should always work! */
		PL_PUSH(i, pl->fetch);
	}
}
