MPICC   = mpicc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGS = -c
CONSTANTS = -D MOBILITY_TIMING=1

all: mm

mm: mm.c hwfarm.o
	$(MPICC) $(FLAGS) -o mm mm.c hwfarm.o

hwfarm.o: ../hwfarm/hwfarm.c
	$(MPICC) $(FLAGS) $(CFLAGS) $(CONSTANTS) ../hwfarm/hwfarm.c -o hwfarm.o
	
clean:
	$(RM) *.o

