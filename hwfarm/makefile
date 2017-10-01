CC      = gcc
MPICC   = mpicc
RM      = rm
FLAGS   = -lpthread -lm
CFLAGES = -c

all: hwfarm

hwfarm: hwfarm.o
	$(MPICC) $(FLAGS) -o hwfarm hwfarm.c

hwfarm.o: hwfarm.c
	$(CC) $(CFLAGES) hwfarm.c -o hwfarm.o
	
clean:
	$(RM) *.o

