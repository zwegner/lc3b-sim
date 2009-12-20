ARCH_FILES=fetch read exec write main

LATCH_FILES=${ARCH_FILES} sim_latch dep_latch
DFA_FILES=${ARCH_FILES} sim_dfa dep_dfa

LATCH_CFILES=${addsuffix .c, ${LATCH_FILES}}
DFA_CFILES=${addsuffix .c, ${DFA_FILES}}

dfa: ${DFA_CFILES} lc3.h
	cc -g ${DFA_CFILES}

latch: ${LATCH_CFILES} lc3.h
	cc -g ${LATCH_CFILES}
