/* lc3.h */
/* Architecture for assembling/simulating an LC3 computer. */

#ifndef _LC3_H_
#define _LC3_H_

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#define REG_COUNT (8)
#define MEM_SIZE (0x10000)

typedef unsigned char REG; /* 0-7 */
typedef signed short WORD;
typedef unsigned short UWORD;
typedef unsigned char BYTE;

typedef enum { FALSE = 0, TRUE = 1 } BOOL;

/* Possible lc3b instructions. */
typedef enum {
	OP_BR,
	OP_ADD,
	OP_LDB,
	OP_STB,
	OP_JSR,
	OP_AND,
	OP_LDW,
	OP_STW,
	OP_RTI,
	OP_XOR,
	OP_UNUSED0,
	OP_UNUSED1,
	OP_JMP,
	OP_SHF,
	OP_LEA,
	OP_TRAP
} INST_ID;

extern char *opcode_str[16];

typedef enum { PL_FETCH, PL_READ, PL_EXEC, PL_WRITE, PL_SIZE } PL_STAGE;

/* Bit stuff for decoding */
#define EXT_BITS(i,h,l)	((i)>>(l) & (1 << (h)-(l)+1) - 1)
#define EXT_BIT(i,l)	EXT_BITS((i),(l),(l))
#define SEXT(w, i)		(EXT_BIT((w),(i)-1) * (-1 << (i)) | \
							EXT_BITS((w),(i-1),0))
#define ZEXT(w, i)		EXT_BITS((w),(i)-1,0)

/* Dependency masks */
#define DM_SR1	(0x01) /* Reads SR1 */
#define DM_SR2	(0x02) /* Reads SR2 */
#define DM_DR	(0x04) /* Writes DR */
#define DM_WCC	(0x08) /* Writes condition codes */
#define DM_RCC	(0x10) /* Reads condition codes */
#define DM_PC	(0x20) /* Writes PC */
#define DM_RM	(0x40) /* Reads from mem */
#define DM_WM	(0x80) /* Writes to mem */

typedef struct INST {
	unsigned int inst;
	INST_ID opcode;

	/* Decoded inst. info */
	REG src_reg_1;
	REG src_reg_2;
	REG dst_reg;
	UWORD mem_ptr; /* pc+off or baser+off */
	WORD imm; /* sign extended */
	BOOL has_imm;

	/* Read inst. info */
	WORD op_1;
	WORD op_2;

	/* Write inst. info */
	WORD result;

	/* Dependency stuff. */
	unsigned int dep_mask;
	BOOL mem_ptr_known;
	BOOL done;

	/* DFA bookkeeping: the cycle when each stage of the instruction was done. */
	int stage_cycle[PL_SIZE];
} INST;

typedef struct {
	INST pipeline[PL_SIZE];
	int pl_ptr; /* Pipeline implemented as a cir. queue, this is front. */

	/* Pointers to instructions in each stage of pipeline */
	INST *fetch;
	INST *read;
	INST *exec;
	INST *write;

	/* Dependency handling */
	BOOL branching; /* Whether we're waiting on a branch target to be
					   calculated so we can fetch */
	unsigned int reg_dep[REG_COUNT]; /* Counter for each reg, 0=no deps */
	unsigned int mem_dep[PL_SIZE]; /* An inst. is modifying this loc. */
} PIPELINE;

#define REGISTER(x)		(s->reg[x])
#define MEMORY(x)		(s->mem[x])

typedef struct {
	UWORD PC;
	WORD PSR;
	WORD reg[REG_COUNT];
	BYTE mem[MEM_SIZE];

	BOOL halt;
} STATUS;

/* Prototypes */
void run(STATUS *s, PIPELINE *pl);

#endif
