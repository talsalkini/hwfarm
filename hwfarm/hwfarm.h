#include <mpi.h>
#include <pthread.h>

int numprocs; 
int namelen;
char processor_name[MPI_MAX_PROCESSOR_NAME];
int rank; //process rank

//For all process
double startTime;
double startTaskTime;
double start_time;

typedef struct hwfarm_state{
	int counter;
	int max_counter;
	void* state_data;
	int state_len;
} hwfarm_state;

typedef struct hwfarm_task_data{
	int task_id;
	void* input_data;
	int input_len;
	void* shared_data;
	int shared_len;
	void* state_data;
	int state_len;
	void* output_data;
	int output_len;
	int* counter;
	int* counter_max;
} hwfarm_task_data;

typedef  void(chFM)();

typedef  void(fp)(hwfarm_task_data*, void(checkForMobility)());

void initHWFarm(int argc, char ** argv);

void finalizeHWFarm();

void hwfarm(fp worker,int tasks,
			void *input, int inSize,int inLen, 
			void *shared_data, int shared_data_size, int shared_data_len, 
			void *output, int outSize, int outLen, hwfarm_state main_state,
			int mobility);
