MPICC   = mpicc
GCC     = gcc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGS = -c
GCCFLAGS = -lm
CONSTANTS = -D MOBILITY_TIMING=0

all: locality

locality: locality.c hwfarm.o
	$(MPICC) $(FLAGS) -o locality locality.c hwfarm.o

hwfarm.o: ../hwfarm/hwfarm.c
	$(MPICC) $(FLAGS) $(CFLAGS) $(CONSTANTS) ../hwfarm/hwfarm.c -o hwfarm.o

	
clean:
	$(RM) *.o 

