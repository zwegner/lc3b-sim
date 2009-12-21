FILES=fetch read exec write main dep

CFILES=${addsuffix .c, ${FILES}}

event: ${CFILES} sim_event.c lc3.h
	cc -g ${CFILES} sim_event.c

dfa: ${CFILES} sim_dfa.c lc3.h
	cc -g ${CFILES} sim_dfa.c

latch: ${CFILES} sim_latch.c lc3.h
	cc -g ${CFILES} sim_latch.c
