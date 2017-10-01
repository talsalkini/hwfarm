MPICC   = mpicc
GCC     = gcc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGS = -c
GCCFLAGS = -lm
CONSTANTS = -D MOBILITY_TIMING=0

all: nbody

nbody: nbody.c hwfarm.o
	$(MPICC) $(FLAGS) -o nbody nbody.c hwfarm.o

hwfarm.o: ../hwfarm/hwfarm.c
	$(MPICC) $(FLAGS) $(CFLAGS) $(CONSTANTS) ../hwfarm/hwfarm.c -o hwfarm.o

serial: nbody_s

nbody_s: nbody_s.c
	$(GCC) $(GCCFLAGS) nbody_s.c -o nbody_s
	
clean:
	$(RM) *.o 

