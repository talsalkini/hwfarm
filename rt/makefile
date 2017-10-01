MPICC   = mpicc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGS = -c
CONSTANTS = -D MOBILITY_TIMING=1

all: rt

rt: rt.c hwfarm.o
	$(MPICC) $(FLAGS) -o rt rt.c hwfarm.o

hwfarm.o: ../hwfarm/hwfarm.c
	$(MPICC) $(FLAGS) $(CFLAGS) $(CONSTANTS) ../hwfarm/hwfarm.c -o hwfarm.o
	
clean:
	$(RM) *.o

