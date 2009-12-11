/* lc3.h */
/* Architecture for assembling/simulating an LC3 computer. */

#define REG_COUNT (8)

typedef unsigned char REG; /* 0-7 */

typedef enum { FALSE = 0, TRUE = 1 } BOOL;

typedef enum {
	OP_ADD,
	OP_ADD_IMM,
	OP_AND,
	OP_AND_IMM,
	OP_BR,
	OP_JMP,
	OP_JSR,
	OP_JSRR,
	OP_LD,
	OP_LDI,
	OP_LDR,
	OP_LEA,
	OP_XOR,
	OP_XOR_IMM,
	OP_RTI,
	OP_ST,
	OP_STI,
	OP_STR,
	OP_TRAP
} INST_ID;

/* Bit stuff for decoding */
#define EXT_BITS(i,h,l)	((i)>>(l) & (1 << (h)-(l)+1) - 1)
#define EXT_BIT(i,l)	EXT_BITS((i),(l),(l))
#define SEXT(w, i)		(EXT_BIT((w),(i)-1) * (-1<<(i))+(w))
#define ZEXT(w, i)		EXT_BITS((w),(i)-1,0)

/* Dependency masks */
#define DM_SR1	(1 << 0) /* Reads SR1 */
#define DM_SR2	(1 << 1) /* Reads SR2 */
#define DM_DR	(1 << 2) /* Writes DR */
#define DM_WCC	(1 << 3) /* Writes condition codes */
#define DM_RCC	(1 << 4) /* Reads condition codes */
#define DM_PC	(1 << 5) /* Writes PC */
#define DM_RM	(1 << 6) /* Reads from mem */
#define DM_WM	(1 << 7) /* Writes to mem */

typedef struct INST {
	unsigned int inst;
	INST_ID opcode;
	REG src_reg_1;
	REG src_reg_2;
	REG dst_reg;
	unsigned int mem_ptr; /* pc+off or baser+off */
	signed int imm; /* sign extended */
	signed short result;

	/* Dependency stuff. */
	unsigned int dep_mask;
	BOOL mem_ptr_known;
	BOOL done;
} INST;

/* Pipeline. */
#define PL_SIZE		(4) /* Only 4 stages of LC3 instruction */

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

typedef struct {
	unsigned int PC;
} STATUS;
