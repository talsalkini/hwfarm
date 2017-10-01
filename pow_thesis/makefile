MPICC   = mpicc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGS = -c
CONSTANTS = -D MOBILITY_TIMING=0

all: pow

pow: pow.c hwfarm_thesis.o
	$(MPICC) $(FLAGS) -o pow pow.c hwfarm_thesis.o

hwfarm_thesis.o: ../hwfarm/hwfarm_thesis.c
	$(MPICC) $(FLAGS) $(CFLAGS) $(CONSTANTS) ../hwfarm/hwfarm_thesis.c -o hwfarm_thesis.o
	
clean:
	$(RM) *.o 

