/* sim.c */
/* The main file for simulating. This implements the pipeline itself, and
 * generally the very highest level of machine abstraction. */

#include <stdio.h>
#include <stdlib.h>
#include "lc3.h"

PIPELINE pl[1];
STATUS s[1];

char *opcode_str[16] = {
	"BR",
	"ADD",
	"LDB",
	"STB",
	"JSR",
	"AND",
	"LDW",
	"STW",
	"RTI",
	"XOR",
	"***UNUSED0***",
	"***UNUSED1***",
	"JMP",
	"SHF",
	"LEA",
	"TRAP"
};

void parse_file(char *fname)
{
	FILE *file = fopen(fname, "r");
	if (!file) {
		perror(fname);
		exit(EXIT_FAILURE);
	}

	int addr;

	fscanf(file, "%x", &addr);

	s->PC = addr;

	int x;
	while (fscanf(file, "%x", &x) == 1) {
		MEMORY(addr++) = (BYTE)x;
		MEMORY(addr++) = (BYTE)(x >> 8);
	}
}

void pl_init(void)
{
	int x;

	pl->pl_ptr = 0;
	pl->branching = FALSE;

	pl->fetch = NULL;
	pl->read = NULL;
	pl->exec = NULL;
	pl->write = NULL;

	for (x = 0; x < REG_COUNT; x++)
		pl->reg_dep[x] = 0;

	for (x = 0; x < PL_SIZE; x++)
		pl->mem_dep[x] = 0;
}

void st_init(void)
{
	int x;

	for (x = 0; x < REG_COUNT; x++)
		REGISTER(x) = 0;

	for (x = 0; x < MEM_SIZE; x++)
		MEMORY(x) = 0;

	s->PSR = 0;
	s->halt = FALSE;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s <file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	pl_init();
	st_init();
	parse_file(argv[1]);

	setbuf(stdout, NULL);
	run(s, pl);
}
