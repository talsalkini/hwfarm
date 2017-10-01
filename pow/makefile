MPICC   = mpicc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGS = -c
CONSTANTS = -D MOBILITY_TIMING=0

all: pow

pow: pow.c hwfarm.o
	$(MPICC) $(FLAGS) -o pow pow.c hwfarm.o

hwfarm.o: ../hwfarm/hwfarm.c
	$(MPICC) $(FLAGS) $(CFLAGS) $(CONSTANTS) ../hwfarm/hwfarm.c -o hwfarm.o
	
clean:
	$(RM) *.o 

