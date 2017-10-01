MPICC   = mpicc
GCC     = gcc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGS = -c
GCCFLAGS = -lm
CONSTANTS = -D MOBILITY_TIMING=0

all: blast

blast: blast.c hwfarm.o
	$(MPICC) $(FLAGS) -o blast blast.c hwfarm.o

hwfarm.o: ../hwfarm/hwfarm.c
	$(MPICC) $(FLAGS) $(CFLAGS) $(CONSTANTS) ../hwfarm/hwfarm.c -o hwfarm.o

serial: blast_s

blast_s: blast_s.c
	$(GCC) $(GCCFLAGS) blast_s.c -o blast_s
	
clean:
	$(RM) *.o 

