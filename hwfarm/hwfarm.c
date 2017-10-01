/**********************************************************************************************
* HWFarm Skeleton using MPI and PThread.
*
* Turkey Alsalkini - Heriot-Watt University, Edinburgh, United Kingdom.
*
*
************************************************************************************************/

#define _GNU_SOURCE
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif 

#include <errno.h>

///
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//Helpers
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <signal.h>
//For RUSAGE
#include <sys/time.h>
#include <sys/resource.h>

//Thread gettid
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

//Top Info
#include <sys/sysinfo.h>
#include <sys/un.h>
#include <unistd.h>

//MPI & PThread
#include <mpi.h>
#include <pthread.h>

//Header File
#include "hwfarm.h"


//Master to workers Tags
#define INIT_LOAD_FROM_WORKER 1
#define MOVE_REPORT_FROM_WORKER 2
#define LATEST_LOAD_REQUEST 3
//#define LOAD_REPORT_FROM_WORKER 6
#define RESULTS_FROM_WORKER 7
#define MOBILITY_CONFIRMATION_FROM_WORKER 9 //Shared with worker tags
#define MOBILITY_NOTIFICATION_FROM_WORKER 10
//Worker tags
#define LOAD_REQUEST_FROM_MASTER 1
#define TERMINATE_THE_WORKER 2
#define LOAD_INFO_FROM_MASTER 3
#define SENDING_CONFIRMATION_FROM_MASTER 4
#define MOBILITY_ACCEPTANCE_FROM_WORKER 5
#define UPDATE_LOAD_REPORT_REQUEST 6
#define TASK_FROM_MASTER 7
#define MOBILITY_REQUEST_FROM_WORKER 8
#define TASK_FROM_WORKER 10
#define SHARED_DATA_FROM_MASTER 11

#define MSG_LIMIT 400000000
#define ESTIMATOR_BREAK_TIME 3
#define NET_LAT_THRESHOLD 1.2

//Global variables
int size; //number of processes
double start_time; //hold start time
int sending_task = 0;//check if the worker is sending a task now
double end_time; // hold end time
MPI_Status status; // store status of a MPI_Recv
MPI_Request request; //capture request of a MPI_Isend
//Master
int isFirstCall = 1;
struct worker_load * w_load_report = 0;
struct worker_load * w_load_report_tmp = 0;
pthread_t w_load_report_th; 
pthread_t w_network_latency_th; 
//Worker 
///Thread to run the worker load agent which collects the  
///worekr load and sends it to the master
pthread_t w_load_pth;
///Thread to run the worker estimator which is responsible 
///for estimating the estimated executing time for the tasks
///which run on the this worker. Then it will send the 
///move reort to the server to make the moving if necessary.
pthread_t w_estimator_pth;

///Pointer for the worker load
struct worker_load * workers_load_report = NULL;

///Worker Main Pointer
struct worker_load_task * w_l_t = NULL;

//Load State
float Master_FREQ;
int load_count = 0;
int currentCores = 0;

///
//double * shared_data;

double load_agent_t1;

double workerT2;


pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

//int master_mutex = 0;

int send_result = 1;

int *masterReceiving;
int *masterSending;
int workerSending = 0;
int workerReceiving = 0;

//int moved = 0;

int BUFFER_SIZE = 102400;
/*
struct timeb {
    time_t   time;
    unsigned short millitm;
    short    timezone;
    short    dstflag;
};
*/
struct workerInfo{
	int tasks_total;
	int tasks_running;
	int tasks_sleeping;
	int tasks_stopped;
	int tasks_zombie;
	float cpuUS;
	float cpuSY;
	float cpuNI;	
	float cpuID;
	float cpuWA;
	float cpuHI;
	float cpuSI;
	float cpuST;
	int mem_total;
	int mem_used;
	int mem_free;
	int mem_buffer;
	int swap_total;
	int swap_used;
	int swap_free;
	int swap_buffer;
};
                 
struct workerReport{
	int load_code;
	int proc_rank;
	float proc_load;
	float proc_load_old;
	int busy;
	int task;
	int current_tasks;
	int totalTask;
	int updated;
	int status;//0:free, 1:working, 2: Requsting, 3: waiting
	int cores;
	float cpuUti;
};

struct workerLoad{
	int cur_load;
	float per;
	float sec;
	struct workerLoad * next;
};

struct checkTime{
	int timeM;
	int taskNum;	
};

struct taskReport{
	int taskNum;
	int proc;
	int procR;
	int start;	
	int end;
	int mobiles;
	int destMobile[20];
	int timeMobile[20];
};

struct task_data{	
	void * input;
	int inputLen;
	void * output;	
	int outputLen;
	int outSize;
	MPI_Datatype mpi_dt;
	int tag;
	void * state;
	int state_size;
	//int parsSize;	
	double start;
	double end;
	int *moving;
};

struct task_to_send{
	void * input;
	int countTask;
	int countData;
	MPI_Datatype taskType;
	int worker;
	int inSize;
};

struct task_cpu{
	int task_id;	
	pthread_t pth_task;
	struct task_data * cur_task;
	struct task_cpu * next;
};

struct result_data{	
	int resultSize;
	MPI_Datatype mpi_dt;
	int tag;
};     

struct other_estimated_costs{
	int w_no;
	float * costs;
	float move_cost;
};

struct estimated_cost{
	int task_no;
	float cur_EC;
	float spent_here;
	float *cur_EC_after;
	struct other_estimated_costs* other_ECs;
	int to_w;
	float to_EC;
};

struct task_cpu * tasksOnCPU = 0;            

struct task_data *td;

struct taskReport * tReport;

struct workerReport * lReport;

struct workerLoad * w1Load;

int *count;

int rank;

int *mainIndex;
int totalIndex;

///*****************************************************************************************
/*
typedef struct hwfarm_state{
	int counter;
	int max_counter;
	void* state_data;
	int state_len;
} hwfarm_state;
	
typedef struct hwfarm_task_data{
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
*/
//void(*fp1)(void*,int,void*,int,void*,int);


//MAX_MOVES: The maximum number of moves(mobilities)
#define MAX_MOVES 20

typedef enum {M_TO_W, W_TO_W, W_TO_M} sendType;

pthread_mutex_t mutex_w_sending = PTHREAD_MUTEX_INITIALIZER;
int worker_sending = 0;

struct task_move_stats{
	double start_move_time;
	double end_move_time;
	double move_time;
	double R_source;
	double R_dest;
	double R_1;
	double R_2;
	double net_time;
	int data_size;
};

//mobile_task: data structure for storing all details about the task
struct mobile_task{
	void * input;							//input data buffer
	int input_len;							//input data length
	int input_unit_size;					//input data unit size(ex: int: 4, double: 8)
	void * shared;							//input data buffer
	int shared_len;							//input data length
	int shared_unit_size;					//input data unit size(ex: int: 4, double: 8)
	void * output;							//output data buffer
	int output_len;							//output data length
	int output_unit_size;					//output data unit size
	MPI_Datatype mpi_dt;					//MPI data type
	int m_task_id;							//tag for task index
	long shift;								//shift value from the begening data
	void * state;							//paramemters buffer
	int state_size;							//paramemters length
	//int pars_unit_size;						//paramemters total size
	int main_counter;						//index of the main variabl in pars buffer(The main index iterations)
	int main_counter_max;						//the total number of iteration for one task
	int moves;								//numbers of moves for the task
	int m_dest[MAX_MOVES];					//the workers who processed the task
	double m_start_time[MAX_MOVES];			//the start times at the workers who processed the task
	double m_end_time[MAX_MOVES];			//the end times at the workers who processed the task
	float m_avg_power[MAX_MOVES];			//The average computing power when the task leave the machine
	float m_work_start[MAX_MOVES];			//The start work when the task arrive
	int moving;								//label for checking the task if it is on moving state
	int done;								//label to set 1 if the task is done
	struct task_move_stats move_stats;		//stats of moving task to predict the trasfoer time
	fp *task_fun; 							//pointer to the task function
};

//mobile_task_report: data structure to store the run-time task information at the master
struct mobile_task_report{
	int task_no;							//Task No
	int task_status;						//Current task status(0: waiting; 1: on processing; 2: completed; 3: on move )
	double task_start;						//The time of start execution
	double task_end;						//The time of end execution
	int task_worker;						//The current worker which processes the task
	int mobilities;							//Number of mobilites for the task
	double m_dep_time[MAX_MOVES];				//The departure time from the source worker
	double m_arr_time[MAX_MOVES];				//The arrival time to the destination worker
	struct mobile_task * m_task;
};

struct task_pool{
	struct mobile_task_report * m_task_report;
	struct task_pool * next;
};

struct worker_local_load{
	float per;
	float sec;
	float load_avg;
	int est_load_avg;
	long double w_cpu_uti;
	int w_running_procs;
	struct worker_local_load*  next;
};

struct estimation{
	float * estimation_costs;
	int chosen_dest;
	float gain_perc;
	int done;
	int on_dest_recalc;
};

struct worker_task{
	int task_id;	
	pthread_t task_pth;
	pthread_t moving_pth;
	double w_task_start;						//The time of start execution
	double w_task_end;						//The time of end execution
	struct mobile_task * m_task;
	struct worker_local_load * w_l_load;
	float local_R;
	int move;
	int go_move;
	int go_to;
	int move_status;
	struct estimation * estimating_move;
	struct worker_task * next;
};
//Struct to hold the values of netowrk messaging times
struct network_times{
	double init_net_time;
	double cur_net_time;
};

struct worker_load{
	int w_rank;
	char w_name[MPI_MAX_PROCESSOR_NAME];
	int w_load_no;	
	int current_tasks;
	int total_task;
	int updated;
	int status;//0:free, 1:working, 2: Requsting, 3: waiting, 4: envolved in moving
	int w_cores;		//Static metric
	int w_cache_size;	//Static metric
	float w_cpu_speed;	//Static metric
	float w_load_avg_1;
	float w_load_avg_2;
	int estimated_load;
	long double w_cpu_uti_1;
	long double w_cpu_uti_2;
	int w_running_procs;
	int locked;
	struct network_times net_times;
};

struct worker_move_report{
	int w_id;
	int num_of_tasks;
	int * list_of_tasks;
};

struct worker_hold{
	int on_hold;							//set to indicate that this worker is on hold to complete the move from the source worker
	int holded_on;							//number of tasks which the worker is waiting for
	int holded_from;						//the worker who i am holded to
	float hold_time;	 					//the time of hold. for cancelation if the request timed out
};

struct worker_load_task{	
	struct worker_hold hold;	
	pid_t worker_tid;
	pid_t status_tid;
	pid_t estimator_tid;
	struct worker_load w_local_loads;
	struct worker_load * w_loads;
	struct worker_task * w_tasks;
	struct worker_move_report * move_report;
};

///-----Worker-------
struct worker_task * w_tasks;

///-----Master-------

struct task_pool * addTasktoPool(struct task_pool * pool, int task_no, void * input_data, 
	int input_data_len, int input_data_unit_size, void * output_data, int output_data_len, 
	int output_data_unit_size, int tag, MPI_Datatype mpi_dt, int shift, 
				void * state, int state_size, int main_index, int main_index_max){
					
	struct task_pool * pl = pool;
	
	if(pool == 0){
		pool = (struct task_pool *)malloc(sizeof(struct task_pool));
		pool->m_task_report = (struct mobile_task_report *)malloc(sizeof(struct mobile_task_report));
		pool->m_task_report->task_no = task_no;
		pool->m_task_report->task_status = 0;
		pool->m_task_report->task_start = 0;
		pool->m_task_report->task_end = 0;
		pool->m_task_report->task_worker = 0;
		pool->m_task_report->mobilities = 0;
		
		pool->m_task_report->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));
		if(input_data != NULL){
			pool->m_task_report->m_task->input = input_data + shift;
			pool->m_task_report->m_task->input_len = input_data_len;
			pool->m_task_report->m_task->input_unit_size = input_data_unit_size;
		}
		pool->m_task_report->m_task->output = output_data;
		pool->m_task_report->m_task->output_len = output_data_len;
		pool->m_task_report->m_task->output_unit_size = output_data_unit_size;
		pool->m_task_report->m_task->m_task_id = tag;
		pool->m_task_report->m_task->mpi_dt = mpi_dt;
		pool->m_task_report->m_task->shift = shift;
		pool->m_task_report->m_task->state = state;
		pool->m_task_report->m_task->state_size = state_size;
		//pool->m_task_report->m_task->pars_unit_size = pars_data_unit_size;
		pool->m_task_report->m_task->main_counter = main_index;
		pool->m_task_report->m_task->main_counter_max = main_index_max;
		pool->m_task_report->m_task->moves = 0;		
		pool->m_task_report->m_task->done = 0;		
		pool->m_task_report->m_task->moving = 0;
		
		pool->next = 0;		

	}else{
		while(pl->next != 0){
			pl = pl->next;		
		}

		pl->next = (struct task_pool *)malloc(sizeof(struct task_pool));
		pl = pl->next;
		
		pl->m_task_report = (struct mobile_task_report *)malloc(sizeof(struct mobile_task_report));
		pl->m_task_report->task_no = task_no;
		pl->m_task_report->task_status = 0;
		pl->m_task_report->task_start = 0;
		pl->m_task_report->task_end = 0;
		pl->m_task_report->task_worker = 0;
		pl->m_task_report->mobilities = 0;
		
		pl->m_task_report->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));
		if(input_data != NULL){
			pl->m_task_report->m_task->input = input_data + shift;
			pl->m_task_report->m_task->input_len = input_data_len;
			pl->m_task_report->m_task->input_unit_size = input_data_unit_size;
		}
		pl->m_task_report->m_task->output = output_data;
		pl->m_task_report->m_task->output_len = output_data_len;
		pl->m_task_report->m_task->output_unit_size = output_data_unit_size;
		pl->m_task_report->m_task->m_task_id = tag;
		pl->m_task_report->m_task->mpi_dt = mpi_dt;
		pl->m_task_report->m_task->shift = shift;
		pl->m_task_report->m_task->state = state;
		pl->m_task_report->m_task->state_size = state_size;
		//pl->m_task_report->m_task->pars_unit_size = pars_data_unit_size;
		pl->m_task_report->m_task->main_counter = main_index;
		pl->m_task_report->m_task->main_counter_max = main_index_max;
		pl->m_task_report->m_task->moves = 0;		
		pl->m_task_report->m_task->done = 0;		
		(pl->m_task_report->m_task->moving) = 0;
		
		pl->next = 0;		
	}
	
	return pool;
}

///This function is used to check the mobility and perform a checkpointing
///if there is a need to transfer this computation
void checkForMobility(){
	//printf("[%d] %u is ready to move...\n", rank, pth_id);
	pthread_t pth_id = pthread_self();
	if(w_tasks == NULL) return;
	struct worker_task * wT = w_tasks->next;
	for(;wT!=0;wT=wT->next){
		if(pth_id == wT->task_pth)
			if((wT->move == 1) && (sending_task == 0)){	
				wT->go_move = 1;
				
				printf("[%d] thread (%zu) is waiting for a confirmation(Time: %f)...\n", rank, pthread_self(), MPI_Wtime());
				//sleep(10);
				
				while(wT->move_status == 0) usleep(1);	
				
				printf("[%d] thread (%zu) has been confirmed(Time: %f)...\n", rank, pthread_self(), MPI_Wtime());
				if(wT->move_status == 1){	
					printf("[%d] thread (%zu) is exiting...\n", rank, pthread_self());
					pthread_exit(NULL);
				}
			}
	}
}


struct task_pool * create_task_pool(int tasks, 
									void* input, int input_len, int input_size, 
									void* output, int output_len, int output_size, MPI_Datatype taskType, 
									void* state, int state_size, 
									int main_index, int chunk_size){
	
	int task_i = 0;
	int task_shift = 0;
	
	struct task_pool * pool = 0;

	//printf("create_task_pool \n");
	
	while(task_i < tasks){
		//printf("Create a task %d \n", task_i+1);
		task_shift = (task_i * input_len) * input_size;
		pool = addTasktoPool(pool, task_i, input, input_len, input_size, 
			output, output_len, output_size, task_i, taskType, task_shift, 
			state, state_size, main_index, chunk_size);
		task_i++;
	}	
	return pool;	
}

void printMobileTask(struct mobile_task* mt){
	printf("Task id: %d @ %d\n-----------------------------------------------\n", mt->m_task_id, rank);
	printf("Input:  (len: %d) - (u_size: %d)\n", mt->input_len, mt->input_unit_size);
	printf("Output: (len: %d) - (u_size: %d)\n", mt->output_len, mt->output_unit_size);
	printf("State: () - (u_size: %d)\n", mt->state_size);
	printf("Main Counter: (init: %d) - (max: %d)\n", mt->main_counter, mt->main_counter_max);
	
	/*
	printf("Input: (add: %x)-(len: %d)-(u_size: %d)\n", mt->input, mt->input_len, mt->input_unit_size);
	int i=0;
	printf("******************************************************\n");
	for(;i<mt->input_len; i++)
		printf("%5.1f\t", *(((double *)mt->input)+i));	
	printf("\n");
	printf("Output: (add: %x)-(len: %d)-(u_size: %d)\n", mt->output, mt->output_len, mt->output_unit_size);
	for(i=0;i<mt->output_len; i++)
		printf("%5.1f\t", *(((double *)mt->output)+i));	
	printf("\n");
	*/
	///printf("Params: (add: %x)-(len: %d)-(u_size: %d)\n", mt->pars, mt->pars_len, mt->pars_unit_size);
	///for(i=0;i<mt->pars_len; i++)
	///	printf("%5d\t", *(((int *)mt->pars)+i));	
	///printf("\n******************************************************\n");

	///printf("Task Shift: %ld - main_par_index: %d - main_par_max: %d\n", mt->shift, mt->main_par_index, mt->main_par_max);	
	///printf("Task Moves: %d - Task Moving: %d\n+++++++++++++++++++++++++++++++++++++++++\n", mt->moves, (mt->moving));
	double final_ex_time = 0.0;
	int i;
	for(i=0;i<mt->moves; i++){
		final_ex_time += mt->m_end_time[i] - mt->m_start_time[i];
		printf("--@ %d (F: %f - to: %f [%f])\n", mt->m_dest[i], mt->m_start_time[i], mt->m_end_time[i], mt->m_end_time[i] - mt->m_start_time[i]);
	}
	printf("--@ X (Total Ex Time: %f)\n", final_ex_time);
	for(i=0;i<mt->moves; i++)
		printf("%f\n%f\n", mt->m_start_time[i], mt->m_end_time[i]);
	printf("\n------------------------------------------------------\n");
}

void printTaskPool(struct task_pool * pool){
	struct task_pool * p = pool;
	while(p != 0){
		printMobileTask(p->m_task_report->m_task);
		p = p->next;
	}	
}

void sendMobileMultiMsgs(void *input, int dataLen, int limit, int proc, int tag){
	
	if(dataLen == 0) return;

	///printf("[%d]4\n", rank);
	int msgCount = (dataLen/limit);
	if((dataLen % limit) != 0) msgCount++;
	printf("[%d]. dataLen: %d - limit: %d (%p)\n", rank, dataLen, limit, input);
	
	int msgSize;
	int i=0;

	//printf("[%d]444\n", rank);
	MPI_Ssend(&msgCount, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
	//printf("[%d]4444\n", rank);

	for(i = 0; i<msgCount; i++){
		
		if(dataLen < limit)
			msgSize = dataLen;
		else
			msgSize = limit;
			
		//printf("[%d]. i: %d - msgSize: %d - w : %d, tag: %d\n", rank, i, msgSize, proc, tag);
		
		MPI_Ssend(input + (i * limit), msgSize, MPI_CHAR, proc, tag + i + 1, MPI_COMM_WORLD);

		dataLen = dataLen - msgSize;
	}
}

void recvMobileMultiMsgs(void * input, int dataLen, int limit, int source, int tag){
	
	if(dataLen == 0) return;
	
	int msgSize ;
	int msgCount;
	int i=0;
	
	printf("[%d] Receiving multiple msg from %d\n", rank, source);
	
	MPI_Recv(&msgCount, 1 , MPI_INT, source, tag, MPI_COMM_WORLD, &status);
	
	for(i = 0; i<msgCount; i++){
		if(dataLen < limit)
			msgSize = dataLen;
		else
			msgSize = limit;
	
		//printf("[%d] Receiving part(%d of %d) , %d, msgSize: %d\n", rank, i, msgCount, tag + i + 1, msgSize);
		MPI_Recv(input + (i * limit), msgSize, MPI_CHAR, source, tag + i + 1, MPI_COMM_WORLD, &status);
		dataLen = dataLen - msgSize;
	}
}

void HW_MPI_SEND(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm){
	 //printf("[%d]. Sending(%d)...", rank, MPI_Send(buf, count, datatype, dest, tag, comm));
	 printf("[%d]. Sending (%p-%d)[%d] to %d with %d...\n", rank, buf, *((int*)buf), count, dest, tag);	 	 
	 int MPI_RET = MPI_Ssend(buf, count, datatype, dest, tag, comm);	 
	 printf("-------------------------------------------\n");
	 switch(MPI_RET){
		 case MPI_SUCCESS:{	
			 printf("[%d]Send done...\n", rank);
			 break;
		 }
		 case MPI_ERR_COMM:{	
			 perror("Invalid communicator...\n");
			 exit(1);					 
			 break;
		 }
		 case MPI_ERR_COUNT:{	
			 perror("Invalid count argument...\n");
			 exit(1);					 
			 break;
		 }
		 case MPI_ERR_TYPE:{	
			 perror("Invalid datatype argument...\n");
			 exit(1);					 
			 break;
		 }
		 case MPI_ERR_TAG:{	
			 perror("Invalid tag argument...\n");
			 exit(1);					 
			 break;
		 }
		 case MPI_ERR_RANK:{	
			 perror("Invalid source or destination rank...\n");
			 exit(1);					 
			 break;
		 }
	 }
 }
 
int getTaskResultSize(struct mobile_task* w_mt){
	int task_struct_size = sizeof(struct mobile_task);
	int task_output_size = w_mt->input_len*w_mt->input_unit_size;	
	
	return task_struct_size + task_output_size;
}
 
int getInitialTaskSize(struct mobile_task* w_mt){
	int task_struct_size = sizeof(struct mobile_task);
	int task_input_size = w_mt->input_len*w_mt->input_unit_size;
	int task_state_size = w_mt->state_size;
	
	return task_struct_size + task_input_size + task_state_size;
}

int getTotalTaskSize(struct mobile_task* w_mt){
	int task_output_size = w_mt->output_len*w_mt->output_unit_size;
	int task_init_size = getInitialTaskSize(w_mt);
	
	return task_output_size + task_init_size;
}


void printNetLatValues(double * net_val, int id){
#if MOBILITY_TIMING == 1  
	printf("[%d]--------------------------------------------------------------\n", rank);	
	printf("[%d]. Sending Mobile Task.start(%d): %f\n", rank, id, *net_val);
	printf("[%d]. Sending Mobile Task.send_code: %f\n", rank, *(net_val + 1));
	printf("[%d]. Sending Mobile Task.send_task_id: %f\n", rank, *(net_val + 2));
	printf("[%d]. Sending Mobile Task.send_struct: %f\n", rank, *(net_val + 3));
	printf("[%d]. Sending Mobile Task.send_input: %f\n", rank, *(net_val + 4));
	printf("[%d]. Sending Mobile Task.send_output: %f\n", rank, *(net_val + 5));
	printf("[%d]. Sending Mobile Task.send_state: %f\n", rank, *(net_val + 6));
	printf("[%d]. Sending Mobile Task.send_end: %f\n", rank, *(net_val + 7));
	printf("[%d]--------------------------------------------------------------\n", rank);
	printf("[%d]--------------------------------------------------------------\n", rank);
	int i = 1;
	printf("[%d]. Sending Mobile Task[%d]: %f\n", rank, id, *net_val);
	printf("[%d]. Sending Mobile Task: %f\n", rank, *(net_val + i) - *(net_val + (i-1)));i++;
	printf("[%d]. Sending Mobile Task: %f\n", rank, *(net_val + i) - *(net_val + (i-1)));i++;
	printf("[%d]. Sending Mobile Task: %f\n", rank, *(net_val + i) - *(net_val + (i-1)));i++;
	printf("[%d]. Sending Mobile Task: %f\n", rank, *(net_val + i) - *(net_val + (i-1)));i++;
	printf("[%d]. Sending Mobile Task: %f\n", rank, *(net_val + i) - *(net_val + (i-1)));i++;
	printf("[%d]. Sending Mobile Task: %f\n", rank, *(net_val + i) - *(net_val + (i-1)));
	printf("[%d]. Sending Mobile Task: %f\n", rank, *(net_val + i) - *(net_val));
	printf("[%d]--------------------------------------------------------------\n", rank);
#endif
} 

//t: the type of task sending
void sendMobileTask(struct mobile_task* mt, int w, sendType t){
	double* net_lat_val = (double*)malloc(sizeof(double)*7);
	int i = 0;
	*(net_lat_val + (i++)) = MPI_Wtime();	
	
	printf("[%d]. Sending Mobile Task(sendMobileTask: 1)\n", rank);
	
	int send_code = 0;

	if(t == W_TO_W)
		send_code = TASK_FROM_WORKER;
	else if(M_TO_W)
		send_code = TASK_FROM_MASTER;
	else if(W_TO_M)
		send_code = RESULTS_FROM_WORKER;
		
	if(t == W_TO_M)
		worker_sending = 0;	
		
	printf("[%d]. Sending Mobile Task(sendMobileTask: 2)\n", rank);
	
	
	int xx = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &xx);
	
	printf("[%d]. send_code: %d, w: %d of %d\n", rank, send_code, w, xx);
		
	MPI_Ssend(&send_code, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
	//HW_MPI_SEND(&xx, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);	
	
	printf("[%d]. Sending Mobile Task(sendMobileTask: 3)\n", rank);
	
	*(net_lat_val + (i++)) = MPI_Wtime();
	
	printf("[%d]. Sending Mobile Task(sendMobileTask: 4)\n", rank);
	
	if(t == W_TO_M){
		while(worker_sending == 0) usleep(1);
	}
	
	printf("[%d]. Sending Mobile Task(sendMobileTask: 5)\n", rank);
	
	if(t == M_TO_W){
		while(*(masterReceiving + (w) - 1) == 1) usleep(1);
		*(masterSending + w - 1) = 1;
	}
		
	printf("[%d]. Sending Mobile Task(sendMobileTask: 6[%d])\n", rank, mt->m_task_id);
		
	if(t == W_TO_M) 	
		MPI_Ssend(&(mt->m_task_id), 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
		
	printf("[%d]. Sending Mobile Task(sendMobileTask: 6-1)\n", rank);
	
	*(net_lat_val + (i++)) = MPI_Wtime();

	if(t == M_TO_W) printMobileTask(mt);

	MPI_Ssend(mt, sizeof(struct mobile_task), MPI_CHAR, w, send_code, MPI_COMM_WORLD);
	
	printf("[%d]. Sending Mobile Task(sendMobileTask: 6-2)\n", rank);

	*(net_lat_val + (i++)) = MPI_Wtime();
	
	if(t != W_TO_M) 
		sendMobileMultiMsgs(mt->input, mt->input_len*mt->input_unit_size, MSG_LIMIT, w, send_code);
	
	printf("[%d]. Sending Mobile Task(sendMobileTask: 7)\n", rank);
	
	*(net_lat_val + (i++)) = MPI_Wtime();
	
	if(t != M_TO_W)
		sendMobileMultiMsgs(mt->output, mt->output_len*mt->output_unit_size, MSG_LIMIT, w, send_code);
		
	*(net_lat_val + (i++)) = MPI_Wtime();
	
	if(t != W_TO_M) 
		sendMobileMultiMsgs(mt->state, mt->state_size, MSG_LIMIT, w, send_code);
	
	printf("[%d]. Sending Mobile Task(sendMobileTask: 8)\n", rank);
	
	*(net_lat_val + (i++)) = MPI_Wtime();
	
	if(t == W_TO_W){
		int task_size = 1 + sizeof(struct mobile_task) + mt->input_len*mt->input_unit_size + mt->output_len*mt->output_unit_size + mt->state_size;
		printf("[%d]-- Size of sent task: %d-[send_code: %d, sizeof(struct mobile_task): %ld, input: %d, output: %d, state: %d\n", rank, task_size, send_code, sizeof(struct mobile_task), mt->input_len*mt->input_unit_size, mt->output_len*mt->output_unit_size, mt->state_size);
	}
	
	if(t == M_TO_W){
		*(masterSending + w - 1) = 0;
	}
	
	printf("[%d]. Sending Mobile Task.SD: init: %d, result: %d, total: %d\n", rank, getInitialTaskSize(mt), getTaskResultSize(mt), getTotalTaskSize(mt));
	printNetLatValues(net_lat_val,  mt->m_task_id);
	
	//net_lat = MPI_Wtime() - net_lat;
}


void sendMobileTaskM(struct mobile_task_report* mtr, int w){
	printf("Sending task (id: %d)\n", mtr->task_no);
	mtr->m_task->move_stats.start_move_time = MPI_Wtime();
	printf("[%d]. start_move_time: %f\n", rank, MPI_Wtime());	
	mtr->m_task->move_stats.R_source = Master_FREQ;
	printf("[%d]. start_move_time: %f\n", rank, MPI_Wtime());
	printf("[%d]. Sending Mobile Task.R: %.2f\n", rank, Master_FREQ);
	sendMobileTask(mtr->m_task, w, M_TO_W);
	
	printf("[%d]. Finish Sending Mobile Task...\n", rank);
	
	//modify task info
	mtr->task_status = 1;
	mtr->task_start = MPI_Wtime();
	mtr->task_worker = w;
}

void* recvSharedData(void* shared_data, int * w_shared_len, int source, int send_code){
	MPI_Status status;

	int shared_len;
	
	MPI_Recv(&shared_len, 1, MPI_INT, source, send_code, MPI_COMM_WORLD, &status);
	
	*w_shared_len = shared_len;
	
	//printf("[%d] Before Ready Receiving the shared_data(%p:%d)\n", rank, shared_data, shared_len);
	
	//sleep(1);
	if(shared_len != 0){
		
		//printf("[%d] Ready Before Receive the shared_data(%p:%d)\n", rank, shared_data, shared_len);
		shared_data = (void*)malloc(shared_len);
		//printf("[%d] Ready to Receive the shared_data(%p:%d)\n", rank, shared_data, shared_len);
		recvMobileMultiMsgs(shared_data, shared_len, MSG_LIMIT, source, send_code);	
		//printf("[%d] Finish Receiving the shared_data(%p:%d)\n", rank, shared_data, shared_len);
		return shared_data;
	}
	
	//printf("[%d] Ready else Receiving the shared_data(%p:%d)\n", rank, shared_data, shared_len);
	
	return NULL;
}

//recvType: the type of task receiving
void recvMobileTask(struct mobile_task* w_mt, int source, sendType t, int send_code){	
	MPI_Status status;
	//int send_code;
	//MPI_Recv(&send_code, 1, MPI_INT, source, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
	void * p_input;
	void * p_state;
	if(t == W_TO_M){
		p_input = w_mt->input;
		p_state = w_mt->state;
	}
	
	printf("[%d]. Receiving task 1 (source: %d)\n", rank, source);
	
	MPI_Recv(w_mt, sizeof(struct mobile_task), MPI_CHAR, source, send_code, MPI_COMM_WORLD,&status);
	
	if(t != W_TO_M){
		//Allocating & receiving input data
		//printf("[%d] Allocating & receiving input data...\n", rank);
		w_mt->input = (void*)malloc(w_mt->input_len*w_mt->input_unit_size);
		recvMobileMultiMsgs(w_mt->input, w_mt->input_len*w_mt->input_unit_size, MSG_LIMIT, source, send_code);	
	}else{
		w_mt->input = p_input;
	}	
	
	printf("[%d]. Receiving task 2 (source: %d)(output-size: %d)\n", rank, source, w_mt->output_len*w_mt->output_unit_size);

	//Allocating & receiving output data
	w_mt->output = (void*)malloc(w_mt->output_len*w_mt->output_unit_size);
	
	if(t != M_TO_W)
		recvMobileMultiMsgs(w_mt->output, w_mt->output_len*w_mt->output_unit_size, MSG_LIMIT, source, send_code);	

	printf("[%d]. Receiving task 3 (source: %d)\n", rank, source);

	if(t != W_TO_M){
		printf("[%d]. Receiving task 3-1 (source: %d)(%d)\n", rank, source, w_mt->state_size);
		//Allocating & receiving states data
		w_mt->state = (void*)malloc(w_mt->state_size);
		printf("[%d]. Receiving task 3-1 (source: %d)(%d)\n", rank, source, w_mt->state_size);
		recvMobileMultiMsgs(w_mt->state, w_mt->state_size, MSG_LIMIT, source, send_code);
	}else{
		w_mt->state = p_state;
	}
	
	printf("[%d]. Receiving task 4 (source: %d)\n", rank, source);
	
	if(t != W_TO_M){
		w_mt->moves++;								
		w_mt->m_dest[w_mt->moves-1] = rank;
	}
}

void recvMobileTaskM(struct task_pool* t_p, int w, int msg_code){
	int task_id = -1;
	MPI_Recv(&task_id, 1, MPI_INT, w, msg_code, MPI_COMM_WORLD, &status);	
	
	//printf("Receiving task (id: %d)\n", task_id);
	
	struct task_pool * p = t_p;
	while( p != NULL){
		if(p->m_task_report->task_no == task_id){
			recvMobileTask(p->m_task_report->m_task, w, W_TO_M, msg_code);
			//printMobileTask(p->m_task_report->m_task);
			p->m_task_report->task_status = 2;
			p->m_task_report->task_end = MPI_Wtime();
			p->m_task_report->task_worker = 0;
			
			//printMobileTask(p->m_task_report->m_task);
			break;
		}
		p = p->next;
	}
}

void recvMsgCode(int *recv_w,int *msg_code){
	MPI_Request req;
	int msgType = -1;
	int flag = 0;
	
	//MPI_Status *status2 = (MPI_Status*)malloc(sizeof(MPI_Status));
	//int *msg_code_2 = (int*)malloc(sizeof(int));
	MPI_Status status2;
	
	//MPI_Recv( msg_code, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status2);		

	//if(rank == 0)
	printf("[%d]- Waiting a msg (t: %f)...\n", rank, MPI_Wtime());
	//printf("[%d]- Waiting a msg...\n", rank);
	//int i = 0;
	
	///Non-Blocking MPI_Irecv
	MPI_Irecv(&msgType, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &req);
	do {		
		MPI_Test(&req, &flag, &status2);
		//if(rank == 1 && (++i % 1 == 0))
		//printf("++++++++++++++++++++++++++++[%d] testing %d- msgType: %d\n", rank, i, msgType);		
		usleep(10);	
    } while (flag != 1);

	printf("[%d]- Receiving from : %d - code: %d (t: %f)\n", rank, status2.MPI_SOURCE, status2.MPI_TAG, MPI_Wtime());
	//printf("[%d]- Receiving from : %d - code: %d\n", rank, status2.MPI_SOURCE, status2.MPI_TAG);
	*recv_w = status2.MPI_SOURCE;
	*msg_code = status2.MPI_TAG;
}

void *workerMobileTask(void *arg)
{	

	struct mobile_task *w_mt = ((struct mobile_task *)arg);	
	
	w_mt->m_start_time[w_mt->moves-1] = MPI_Wtime();
	
	int i = w_mt->main_counter;
	float Wd_before = ((i * 100) / (float)w_mt->main_counter_max);
	//printf("[%d]. Work done before: %f\n", rank, Wd_before);
	w_mt->m_work_start[w_mt->moves-1] = Wd_before;	
	
	//printMobileTask(w_mt);
	
	hwfarm_task_data * t_data = (hwfarm_task_data *)malloc(sizeof(hwfarm_task_data));
	t_data->input_data = w_mt->input;
	t_data->input_len = w_mt->input_len;
	t_data->shared_data = w_mt->shared;
	t_data->shared_len = w_mt->shared_len;
	t_data->state_data = w_mt->state;
	t_data->state_len = w_mt->state_size;
	t_data->output_data = w_mt->output;
	t_data->output_len = w_mt->output_len;
	t_data->counter = &w_mt->main_counter;
	t_data->counter_max = &w_mt->main_counter_max;
	t_data->task_id = w_mt->m_task_id;
	
	printf("************************ i: %d, max: %d\n", w_mt->main_counter, w_mt->main_counter_max);

	w_mt->task_fun( t_data, checkForMobility);
	
	//printf("m_task_id: %d finished[%lx]...\n", w_mt->m_task_id, w_mt);
	
	w_mt->done = 1;

	w_mt->m_end_time[w_mt->moves-1] = MPI_Wtime();
	
	//w_mt->w_task_end = MPI_Wtime();
	
	//sleep(3);
	//printMobileTask(w_mt);
	
	pthread_mutex_lock( &mutex_w_sending );
	
	sendMobileTask(w_mt, 0, W_TO_M);
	
	pthread_mutex_unlock( &mutex_w_sending );
	
	/*
	 * //print the time cpent on the cores using Sys Call
	#ifdef SYS_gettid
	pid_t tid = syscall(SYS_gettid);
	#else
	#error "SYS_gettid unavailable on this system"
	#endif
	
	//printf("{Worker mobile task: %d}My process ID : %u\n", rank, tid);
    
	char *res4 = (char*)malloc(sizeof(char)*200);
	
	char *command = (char*)malloc(sizeof(char)*200);
	
	sprintf(command, "cat /proc/%u/task/%u/stat", getpid(), tid);
	
	systemCall(command, res4);
	printf("[worker: %d] - {%u(%d)} P-ID: \n%s\n", rank, tid, w_mt->m_task_id, res4);
	*/
	/*
	res4 = (char*)malloc(sizeof(char)*200);
	sprintf(command, "cat /proc/%u/task/%u/status | grep ctxt_switches", getpid(), tid);
	systemCall(command, res4);
	printf("[worker: %d] - {%u(%d)} P-ID: \n%s\n", rank, tid, w_mt->m_task_id, res4);
	*/
	
	/*
	 * //print the time cpent on the cores using getrusage
	res4 = (char*)malloc(sizeof(char)*200);
	//sprintf(command, "ps -u $(whoami)");
	command = "ps -u $(whoami)";
	systemCall(command, res4);
	printf("[worker: %d] - whoami: \n%s\n", rank, res4);
	

	
	//////
	
    struct rusage ru;
    struct timeval utime;
    struct timeval stime;

    getrusage(RUSAGE_THREAD, &ru);
    utime = ru.ru_utime;
    stime = ru.ru_stime;
    printf("RUSAGE :\nru_utime => %lld [sec] : %lld [usec]\nru_stime => %lld [sec] : %lld [usec] \nSWAPS: %ld, SIGNALS: %ld\n",
           (int64_t)utime.tv_sec, (int64_t)utime.tv_usec,
           (int64_t)stime.tv_sec, (int64_t)stime.tv_usec,
           ru.ru_nswap, ru.ru_maxrss);
	
	/////
	*/
	
	free(w_mt->state);
	free(w_mt->output);
	free(w_mt->input);
	
	//free(w_mt);
	
	//if(*(td->moving) == 1)return;

	//sendResult(td->output,td->outputLen * td->outSize,td->mpi_dt,td->tag);
	
	//*count = *count + 1;
	//td->end = MPI_Wtime();

	//free(td);
	//free(td->input);
	//free(td->output);

	return NULL;
}


//int getActualRunningprocessors(float cpu_uti, int est_load_avg, int running_processes, int cores){
float getActualRunningprocessors(float cpu_uti, int est_load_avg, int running_processes, int cores){
	float np_per = -1;
	if(cpu_uti < 75){
		 //np = (cpu_uti * cores / 100);	
		 np_per = cpu_uti ;
	}else{
		if(running_processes <= cores)
			np_per = cpu_uti;
		else{
			//np_per = cpu_uti + ((running_processes - cores) * 100) / (cores * 1.0);
			np_per = (cpu_uti + ((running_processes) * 100) / (cores * 1.0)) / 2;
		}			
		
		/*
		int min = 0, max = 0, dif = 0;
		if(est_load_avg == running_processes)
			np = est_load_avg;
		else if(est_load_avg > running_processes){
			min = running_processes;
			max = est_load_avg;
		}else{
			max = running_processes;
			min = est_load_avg;
		}
		dif = max - min;
		np = (min + dif * max) / (dif+1);
		* */
		
		//if(est_load_avg != -1){
		//	np = (est_load_avg + running_processes)/2;
		//}else{
			//np = running_processes;
		//}
	}
	
	float base = 100 / (cores * 1.0);
	//printf("[%d]: base: %.3f, np_per: %.3f\n", rank, base, np_per);
	int np = (int)(np_per / base);
	//printf("[%d]: np: %d\n", rank, np);
	if(np < (np_per / base))
		np++;
	//printf("[%d]: np: %d\n", rank, np);
	//np = running_processes;
	//printf("[%d]: np: %d\n", rank, np);
	
	//return np;
	return (np_per/base);
}

float getActualRelativePower(float P, float cpu_uti, int est_load_avg, int running_processes, int cores, int added_np, int worker){
	
	//int np = getActualRunningprocessors( cpu_uti,  est_load_avg,  running_processes, cores);		
	float np_f = getActualRunningprocessors( cpu_uti,  est_load_avg,  running_processes, cores);		
	
	//Add/subtract the number of process 
	if(added_np != 0){
	//printf("[%d]. added_np: %d, np: %d\n", rank, added_np, np);
		//np += added_np;
		np_f += added_np;
		//printf("[%d]. added_np: %d, np: %d\n", rank, added_np, np);
	}
	
	//printf("[%d]. Actual Running processes @ %d : %d[P: %.2f, CPU: %.2f, LAVG: %d, RP: %d]\n", rank, worker, np, P, cpu_uti, est_load_avg, running_processes);
	///printf("[%d]. Actual Running processes @ %d : %.2f[P: %.2f, CPU: %.2f, LAVG: %d, RP: %d]\n", rank, worker, np_f, P, cpu_uti, est_load_avg, running_processes);
	
	//The relative computing power for the next time
	//float Rhn = P/np;
	float Rhn = P/np_f;
	float MAX_R = P/cores;
	//If the relative power greater than the maximum => 
	//the relative power equals to maximum core speed
	if(Rhn > MAX_R)
		Rhn = MAX_R;
	
	return Rhn;
}

float getRForWorker(struct worker_load_task *w_l_t, int worker){
	float remote_power = (w_l_t->w_loads + worker - 1)->w_cpu_speed;
	float remote_cpu_uti = (w_l_t->w_loads + worker - 1)->w_cpu_uti_2;
	int remote_ext_load_avg = (w_l_t->w_loads + worker - 1)->estimated_load;
	int remote_running_procs = (w_l_t->w_loads + worker - 1)->w_running_procs;
	int remote_cores = (w_l_t->w_loads + worker - 1)->w_cores;
	float R = getActualRelativePower(remote_power, remote_cpu_uti, remote_ext_load_avg, remote_running_procs + 1, remote_cores, 0, (w_l_t->w_loads + worker - 1)->w_rank);
	
	return R;		
}

float getPredictedMoveTime(struct mobile_task* w_mt, struct worker_load_task * w_l_t, int worker){	
	float R_1 = w_mt->move_stats.R_source;
	if(R_1 > w_mt->move_stats.R_dest)
		R_1 = w_mt->move_stats.R_dest;

	w_mt->move_stats.R_1 = R_1;

	//printf("[%d]. R1_s: %.2f, R1_d: %.2f\n", rank, w_mt->move_stats.R_source, w_mt->move_stats.R_dest);
	int data_size_1 = w_mt->move_stats.data_size;
	//double net_time_1 = w_mt->move_stats.net_time;
	double net_time_1 = w_l_t->w_loads->net_times.init_net_time;
	double move_time_1 = w_mt->move_stats.move_time;
	
	
	int data_size_2 = getTotalTaskSize(w_mt);
	//double net_time_2 = w_l_t->net_times.cur_net_time;
	double net_time_2 = w_l_t->w_loads->net_times.cur_net_time;
	float R_2_s = getRForWorker(w_l_t, rank);
	float R_2_d = getRForWorker(w_l_t, worker);
	
	float R_2 = R_2_s;
	if(R_2 > R_2_d)
		R_2 = R_2_d;
	
	w_mt->move_stats.R_2 = R_2;
	
	//printf("[%d]. R2_s: %.2f, R2_d: %.2f\n", rank, R_2_s, R_2_d);
	//printf("[%d]. rank: %d, worker: %d\n", rank,rank, worker);
	
	double move_time_2 = 0;
	
	float W_DS = 1.043;
	float W_R = 1;
	float W_L = (net_time_2 < NET_LAT_THRESHOLD) ? 1 : 0.685;
	if(net_time_2 < NET_LAT_THRESHOLD){
		/*
		W_DS = W_DS / 2;
		W_R = W_R / 2;
		double R_effect = W_R * (R_1 / R_2);
		printf("[%d]. R_1: %f, R_2: %f, (R_1 / R_2): %f, W_R: %f => R_effect: %f\n", rank, R_1, R_2, R_1 / R_2, W_R, R_effect);
		double SD_effect = W_DS * (1.0*data_size_2 / data_size_1);
		printf("[%d]. data_size_2: %d, data_size_1: %d, (data_size_2 / data_size_1): %f, W_DS: %f => SD_effect: %f\n", rank, data_size_2, data_size_1, 1.0* data_size_2 / data_size_1, W_DS, SD_effect);
		move_time_2 = (R_effect + SD_effect) * move_time_1;
		printf("[%d]. W_DS: %.2f, W_R: %.2f, R_effect: %.2f(R_1: %.3f, R_2: %.3f), SD_effect: %.2f\n", rank, W_DS, W_R, R_effect, R_1, R_2, SD_effect);
	*/
		W_DS = 0.996;
		double W_DS_Constant = 0.004;
		W_R = 0.94;
		double W_R_Constant = 0.06;
		double R_effect = W_R * (R_1 / R_2) + W_R_Constant;
		//printf("[%d]. PRIDECTING MC. R_1: %f, R_2: %f, (R_1 / R_2): %f, W_R: %f => R_effect: %f\n", rank, R_1, R_2, R_1 / R_2, W_R, R_effect);
		double SD_effect = W_DS * (1.0*data_size_2 / data_size_1)+W_DS_Constant;
		//printf("[%d]. PRIDECTING MC. data_size_2: %d, data_size_1: %d, (data_size_2 / data_size_1): %f, W_DS: %f => SD_effect: %f\n", rank, data_size_2, data_size_1, 1.0* data_size_2 / data_size_1, W_DS, SD_effect);
		move_time_2 = (R_effect * SD_effect) * move_time_1;
		//printf("[%d]. PRIDECTING MC. W_DS: %.2f, W_R: %.2f, R_effect: %.2f(R_1: %.3f, R_2: %.3f), SD_effect: %.2f\n", rank, W_DS, W_R, R_effect, R_1, R_2, SD_effect);
		
		
		//printf("[%d]. PRIDECTING MC. R1: %.2f, DS1: %d, NT1: %f [%f], T1: %f\n", rank, R_1, data_size_1, net_time_1, w_l_t->w_loads->net_times.init_net_time, move_time_1);
		//printf("[%d]. PRIDECTING MC. R2: %.2f, DS2: %d, NT2: %f [%f], T2: [%f]\n", rank, R_2, data_size_2, net_time_2, w_l_t->w_loads->net_times.cur_net_time, move_time_2);
		
		W_R = 0.9895;
		W_DS = 0.9724;
		R_effect = 1.1*pow((R_1 / R_2), W_R);
		//printf("[%d]. PRIDECTING MC. R_1: %f, R_2: %f, (R_1 / R_2): %f, W_R: %f => R_effect: %f\n", rank, R_1, R_2, R_1 / R_2, W_R, R_effect);
		SD_effect = 1.1*pow((1.0*data_size_2 / data_size_1), W_DS);
		//printf("[%d]. data_size_2: %d, data_size_1: %d, (data_size_2 / data_size_1): %f, W_DS: %f => SD_effect: %f\n", rank, data_size_2, data_size_1, 1.0* data_size_2 / data_size_1, W_DS, SD_effect);
		move_time_2 = (R_effect * SD_effect) * move_time_1;
		//printf("[%d]. PRIDECTING MC. W_DS: %.2f, W_R: %.2f, R_effect: %.2f(R_1: %.3f, R_2: %.3f), SD_effect: %.2f\n", rank, W_DS, W_R, R_effect, R_1, R_2, SD_effect);

		//printf("[%d]. PRIDECTING MC. R1: %.2f, DS1: %d, NT1: %f [%f], T1: %f\n", rank, R_1, data_size_1, net_time_1, w_l_t->w_loads->net_times.init_net_time, move_time_1);
		//printf("[%d]. PRIDECTING MC. R2: %.2f, DS2: %d, NT2: %f [%f], T2: [%f]\n", rank, R_2, data_size_2, net_time_2, w_l_t->w_loads->net_times.cur_net_time, move_time_2);
		
		//printf("[%d]. PRIDECTING MC. --------------------------------------------------------------------------------------------------------------------------\n", rank);
		//W_R = 0.968;
		//W_R = 1.45;
		//W_R = 1.24;
		W_R = 1.04;
		R_effect = pow((R_1 / R_2), W_R);
		//printf("[%d]. PRIDECTING MC. R_1: %f, R_2: %f, (R_1 / R_2): %f, W_R: %f => R_effect: %f\n", rank, R_1, R_2, R_1 / R_2, W_R, R_effect);
		
		W_DS = 1.023;
		//W_DS = 1.012;
		SD_effect = pow((1.0*data_size_2 / data_size_1), W_DS);
		//printf("[%d]. PRIDECTING MC. data_size_2: %d, data_size_1: %d, (data_size_2 / data_size_1): %f, W_DS: %f => SD_effect: %f\n", rank, data_size_2, data_size_1, 1.0* data_size_2 / data_size_1, W_DS, SD_effect);
		
		move_time_2 = (R_effect * SD_effect) * move_time_1;
		//printf("[%d]. PRIDECTING MC. W_DS: %.2f, W_R: %.2f, R_effect: %.2f(R_1: %.3f, R_2: %.3f), SD_effect: %.2f\n", rank, W_DS, W_R, R_effect, R_1, R_2, SD_effect);

		//printf("[%d]. PRIDECTING MC. R1: %.2f, DS1: %d, NT1: %f [%f], T1: %f\n", rank, R_1, data_size_1, net_time_1, w_l_t->w_loads->net_times.init_net_time, move_time_1);
		//printf("[%d]. PRIDECTING MC. R2: %.2f, DS2: %d, NT2: %f [%f], T2: [%f]\n", rank, R_2, data_size_2, net_time_2, w_l_t->w_loads->net_times.cur_net_time, move_time_2);
		//printf("[%d]. PRIDECTING MC. --------------------------------------------------------------------------------------------------------------------------\n", rank);
		
	}else{
		W_DS = W_DS / 3;
		W_R = W_R / 3;
		W_L = W_L / 3;
		double R_effect = W_R * (R_1 / R_2);
		double SD_effect = W_DS * (1.0*data_size_2 / data_size_1);
		double Net_effect = W_L * (net_time_2 / net_time_1);
		move_time_2 = (R_effect + SD_effect + Net_effect) * move_time_1;
	}
	
	
	
	return 1.0;	
}

float getMoveTime(struct mobile_task* w_mt){
	
	//double m_time = w_mt->task_move_time;	
	double m_time = w_mt->move_stats.move_time;
	
	int pure_move_time = getInitialTaskSize(w_mt);
	
	int task_output_size = w_mt->output_len*w_mt->output_unit_size;
	int total_move_time = pure_move_time + task_output_size;
	
	return m_time * total_move_time / pure_move_time;
}
	
void taskOutput(struct task_pool* t_p, void* output, int outLen, int outSize){
	printf("[%d]. outLen*outSize: %d\n", rank, outLen*outSize);
	int task_i = 0, output_i = 0, task_output_i = 0;
	int output_shift;
	struct task_pool * p = t_p;
	while( p != NULL){
		output_shift = (task_i * outLen) * outSize;
		for(task_output_i = 0; task_output_i < outLen*outSize; task_output_i++){
			*((char*)output + output_i++) = *((char*)p->m_task_report->m_task->output + task_output_i);
		}			
		p = p->next;
		task_i++;
	}		
}

void printWorkerLoad(struct worker_load w_load){
	printf("---------------------------------------------------------------------------------------------------------\n");
	printf("[M/W]- W | no | name | ts | tot_t | s | cores | cache |  CPU Freq  | l.avg1 | l.avg2 | est | uti.1 | uti.2 | run_pro\n");
	printf("---------------------------------------------------------------------------------------------------------\n");
	printf("[%d]- %3d | %6s | %2d | %2d |  %2d   | %c |  %2d   |  %2d   | %.2f  | "
		" %2.2f  |  %2.2f  | %2d  | %3.2Lf  |  %3.2Lf |  %d\n", 
	rank, w_load.w_rank, w_load.w_name, w_load.w_load_no,
	w_load.current_tasks, w_load.total_task, 
	(w_load.status == 1 ? 'B' : 'F'), w_load.w_cores, w_load.w_cache_size,
	w_load.w_cpu_speed, w_load.w_load_avg_1, w_load.w_load_avg_2, w_load.estimated_load, 
	w_load.w_cpu_uti_1, w_load.w_cpu_uti_2, w_load.w_running_procs);
	printf("---------------------------------------------------------------------------------------------------------\n");
}

void printWorkerLoadReport(struct worker_load * report, int n){
	char * t_s = NULL;
	int i;
	if(rank == 0){
		t_s = (char *)malloc(sizeof(char)*1000);
		sprintf(t_s, "%.3f", (MPI_Wtime() - startTime));
	}

	printf("---------------------------------------------------------------------------------------------------------\n");
	printf("[M/W]- W | no | ts | tot_t | Locked | s | cores | cache |  CPU Freq  | l.avg1 | l.avg2 | est | uti.1 | uti.2 | run_pro [%.3f] \n", (MPI_Wtime() - startTime));
	printf("---------------------------------------------------------------------------------------------------------\n");
	for(i=1; i<n; i++){		
		//printf("[%d]- %3d(%s)(%.3f|%.3f) | %2d | %2d |  %2d   | %c |  %c  |  %2d   |  %2d   | %.2f  | "
		//	" %2.2f  |  %2.2f  | %2d  | %3.2Lf  |  %3.2Lf |  %d\n", 
		printf("[%d]- %3d | %2d | %2d |  %2d   | %c |  %c  |  %2d   |  %2d   | %.2f  | "
			" %2.2f  |  %2.2f  | %2d  | %3.2Lf  |  %3.2Lf |  %d\n", 
		//rank, report[i-1].w_rank, report[i-1].w_name, report[i-1].net_times.init_net_time, report[i-1].net_times.cur_net_time, report[i-1].w_load_no,
		rank, report[i-1].w_rank, report[i-1].w_load_no,
		report[i-1].current_tasks, report[i-1].total_task, (report[i-1].locked == 1 ? 'Y' : 'N'),
		((report[i-1].status == 1) ? 'B' : ((report[i-1].status == 4) ? 'M' : 'F')), report[i-1].w_cores, report[i-1].w_cache_size,
		report[i-1].w_cpu_speed, report[i-1].w_load_avg_1, report[i-1].w_load_avg_2, report[i-1].estimated_load, 
		report[i-1].w_cpu_uti_1, report[i-1].w_cpu_uti_2, report[i-1].w_running_procs);
		//
		if(rank == 0){
			char * t_s_1 = (char *)malloc(sizeof(char)*1000);
			char * t_s_2 = (char *)malloc(sizeof(char)*1000);
			sprintf(t_s_1, "%s", t_s);
			sprintf(t_s_2, "%d", report[i-1].current_tasks);
			sprintf(t_s, "%s, %s", t_s_1, t_s_2);
			free(t_s_1);
			free(t_s_2);
		}
	}
	printf("---------------------------------------------------------------------------------------------------------\n");
	if(rank == 0){
		printf("MASTER CURRENT TASKS: %s\n", t_s);
		printf("---------------------------------------------------------------------------------------------------------\n");
		free(t_s);
	}
}

void updateWorkerStatus(struct worker_load * report, int n, int worker, int new_status){
	int i;
	for(i=1; i<n; i++){				 
		if(report[i-1].w_rank == worker)
			report[i-1].status = new_status;		
	}
}

void updateWorkerStatusMoving(struct worker_load * report, int n, int source, int dest){
	updateWorkerStatus(report, n, source, 4);
	updateWorkerStatus(report, n, dest, 4);
}

void updateWorkerStatusWorking(struct worker_load * report, int n, int source, int dest){
	updateWorkerStatus(report, n, source, 1);
	updateWorkerStatus(report, n, dest, 1);
}

void recvMovingNotification(struct worker_load * report, int n, int w_source, int msg_code){
	int* data = (int*)malloc(sizeof(int)*2);
	MPI_Recv(data, 2, MPI_INT, w_source, msg_code, MPI_COMM_WORLD, &status);
	printf("[%d]. master received a notification of moving task %d from %d to %d \n", rank, *data, w_source, *(data+1));
	
	printWorkerLoadReport(report, n);
	//
	updateWorkerStatusMoving(report, n, w_source, *(data+1));
	//
	printWorkerLoadReport(report, n);
}

//dynamic: type of distributing the tasks among workers
// = 1: depends on the maximum capacity of tasks
// = 2: depends first on even distribution
void getBestWorker(struct worker_load * report, int *w, int n, int dynamic){	
	//printWorkerLoadReport(report, n);
	int i=0;
	if(dynamic == 1){
		for(i=1; i<n; i++){		
			if((report[i-1].current_tasks + report[i-1].w_running_procs) < report[i-1].w_cores){
				*w = i;
				return;
			}
		}
		*w = -1;
	}else if(dynamic == 2){
		
	}else{
		for(i=1; i<n; i++){		
			if((report[i-1].current_tasks + report[i-1].w_running_procs) < report[i-1].w_cores){
				*w = i;
				return;
			}
		}
		
		*w = -1;
		//int r = (rand() % (n-1)) + 1;
		////printf("r: %d\n", r);
		//*w = r;
		//*w = 1;
		return;
	}
}

//Get the worker who will recieve the next task
void getValidWorker(int * tasksPerWorker, int n, int *w){	
	int w_i = 0;
	for(w_i = 0; w_i < n; w_i++){
		if(tasksPerWorker[w_i] > 0){
			tasksPerWorker[w_i]--;
			*w = w_i+1;
			return;
		}
	}
}

struct mobile_task_report * getReadyTask( struct task_pool* t_p){
	struct task_pool * p = t_p;
	while( p != NULL){
		if(p->m_task_report->task_status == 0){						
			return p->m_task_report;
		}
		p = p->next;
	}
	return NULL;
}

///Send terminator message to the finished worker
void terminateWorker(int w){	
	int msg_code = TERMINATE_THE_WORKER;
	MPI_Send(&msg_code, 1, MPI_INT, w, msg_code, MPI_COMM_WORLD);
}

///Send Terminator message to all processes
void terminateWorkers(int ws){
	int i=0;
	for( i=1; i < ws; i++)
		terminateWorker(i);
}

struct worker_task* newWorkerTask(struct worker_task * w_t_header, struct worker_task * w_t){
	if(w_t_header == NULL){
		w_t_header = w_t;
		w_t_header->next = NULL;
		return w_t_header;
	}
	
	//printf("[%d]. Ready(newWorkerTask)...\n", rank);
	
	struct worker_task * p = w_t_header;
	while(p->next != NULL){
		//printf("[%d]. Ready(newWorkerTask)(p: %p , next: %p)...\n", rank, p, p->next);
		p = p->next;
	}
		
	//printf("[%d]. Ready(newWorkerTask)(p: %p , next: %p)...\n", rank, p, p->next);
	
	p->next = w_t;
	p->next->next = NULL;
	
	//printf("[%d]. Ready(newWorkerTask)(p: %p , next: %p)...\n", rank, p, p->next);
	
	//printf("w_t_header: %lx - w_t: %lx\n", w_t_header, w_t->m_task);
	
	return w_t_header;
}


void systemCall(b,res)
char* b;
char* res;
{
	FILE *fp1;
	char *line = (char*)malloc(130 * sizeof(char));
	fp1 = popen(b, "r");  
	if(fp1 == 0)
		perror(b);
	res[0]='\0';
	while ( fgets( line, 130, fp1))
	{ 
		strcat(res, line);
	}  
	res[strlen(res)-1]='\0'; 
	pclose(fp1); 
	free(line); 
	return;
}


//send ping message to the selected worker
double sendPing(char * node_name){
	
	char * pre_ping_command = "ping %s -c 1 | grep \"rtt\" | awk '{split($0,a,\" \"); print a[4]}' | awk '{split($0,b,\"/\"); print b[2]}'";
	
	char *ping_command = (char*)malloc(sizeof(char)*200);
	
	sprintf(ping_command, pre_ping_command, node_name);
	
	//printf("[%d]. sendPing: Commnad: %s\n", rank, ping_command);
	
	char * rtt_latency_str = (char*)malloc(sizeof(char)*20);
	systemCall(ping_command, rtt_latency_str);
	
	//printf("[%d]. sendPing: Reply: %s\n", rank, rtt_latency_str);
	double net_latency = atof(rtt_latency_str);
	
	if(net_latency != 0.0){		
		free(ping_command);
		free(rtt_latency_str);
		return net_latency;
	}else{		
		//printf("[%d]. sendPing: ERROR Ping: Command: %s, Res: %s\n", rank, ping_command, rtt_latency_str);
				
		free(ping_command);
		free(rtt_latency_str);
		return 0.0;
	}
		
	
	
	//printf("[%d]. finish sendPing\n", rank);
}

//----------------------------------------------------------------------
//------------------Start of getting load amount------------------------
//----------------------------------------------------------------------

int readLine(FILE * f,char a[],int n){
	int i=0; 
	char c = getc(f);
	if(c == EOF)return EOF;
	while(c!='\n' && c!= EOF)
	{
		a[i++] = c;	
		c = getc(f);
		if(i == n)break;		
	}
	if(i == n)
	{
		c=getc(f);
		while(c!='\n'&&c!= EOF)c=getc(f);
	}	
	a[i]='\0';
	return i;
}

int contains(char target[],int m, char source[] ,int n){
	int i,j=0;
	for(i=0;i<n;i++){		
		if(target[j] == source[i])
			j++;		
		else j=0;
		if(j == m)return 1;
	}
	return 0;
}

float getCoresFreq(){
    FILE *f;
    int LINE_MAX = 128;
    char *line = (char*)malloc(LINE_MAX * sizeof(char));
    char *tmp = (char*)malloc(10 * sizeof(char));
    
    float total_cores_freq = 0, total_cores_freq2 = 0;

    f = fopen("/proc/cpuinfo","r");	
    if(f == 0){
		perror("Could open the file :/proc/cpuinfo");
		return 0;
	}

	float core_freq;
	float min_core_freq = 0;
	int cores_cnt = 0;
	char c;
	while(fgets(line, LINE_MAX, f)){
		if(contains("cpu MHz", 7, line, LINE_MAX)){
			sscanf(line, "%s %s %c %f\n", tmp, tmp, &c, &core_freq);
			if(cores_cnt == 0)
				min_core_freq = core_freq;
			else if(min_core_freq > core_freq)
				min_core_freq = core_freq;
			total_cores_freq += core_freq;
			cores_cnt++;
			//printf("Freq: %f - Total: %f\n", core_freq, total_cores_freq);
		}
	}
	
	total_cores_freq2 = min_core_freq * cores_cnt;
	printf("[%d]. Min: %.2f, cnt: %d, NEW Freq: %f\n", rank, min_core_freq, cores_cnt, total_cores_freq2);
	
	fclose(f);
	
	free(tmp);
	free(line);
		
	//return total_cores_freq; 
	return total_cores_freq2;
}

int getNumberOfCores(){
	long nprocs = -1;
	long nprocs_max = -1;
	#ifdef _WIN32
		#ifndef _SC_NPROCESSORS_ONLN
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			#define sysconf(a) info.dwNumberOfProcessors
			#define _SC_NPROCESSORS_ONLN
		#endif
	#endif
	#ifdef _SC_NPROCESSORS_ONLN
		nprocs = sysconf(_SC_NPROCESSORS_ONLN);
		if (nprocs < 1)
		{
			fprintf(stderr, "Could not determine number of CPUs online:\n%s\n", 
			strerror (errno));
			return nprocs;
			//exit (EXIT_FAILURE);
		}
		nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
		if (nprocs_max < 1)
		{
			fprintf(stderr, "Could not determine number of CPUs configured:\n%s\n", 
			strerror (errno));
			return nprocs;
			//exit (EXIT_FAILURE);
		}
		//printf ("%ld of %ld processors online\n",nprocs, nprocs_max);
		//exit (EXIT_SUCCESS);
		return nprocs;
	#else
		fprintf(stderr, "Could not determine number of CPUs");
		//exit (EXIT_FAILURE);
		return nprocs;
	#endif
}

int getProcessState(int p_id, int is_task, int t_id){
    FILE *f;
    char *line = (char*)malloc(1000 * sizeof(char));
    
    char * stateFile = (char*)malloc(sizeof(char)*200);
    if(is_task == 1)		
		sprintf(stateFile, "/proc/%u/task/%u/stat", p_id, t_id);
	else
		sprintf(stateFile, "/proc/%u/stat", p_id);
		
	f = fopen(stateFile,"r");
	
    //if(f == 0)
		//perror(stateFile);
	if(f == 0)
		return 0;

	int state = 0;
	int i = 0;

	do{
		int numOfI = -1;
		numOfI = fscanf(f, "%s\n", line);	
		if(strlen(line) == 1){
			char c = *((char *)line);
			if((c == 'S') || (c == 'D') || (c == 'T') || (c == 'W') || (c == 'Z')){
				state = 0;				
				break;
			}
			if(c == 'R'){
				state = 1;				
				break;
			}
		}
	}while(i++ != 10);
	
	fclose(f);

	free(line);
	free(stateFile);
	
	return state; 
}

int getRunningProc(){
    FILE *f;
    char *line = (char*)malloc(1000 * sizeof(char));
    int run_proc = 0;

    f = fopen("/proc/stat","r");
	
    if(f == 0)
		perror("/proc/stat");

	int l = 0, numOfI = 0;
	do{
		l = fscanf(f, "%s\n", line);	
		if(strcmp(line, "procs_running") == 0){
			numOfI = fscanf(f, "%d\n", &run_proc);				
			break;
		}	
	}while(l != 0);
	
	fclose(f);

	free(line);
		
	return run_proc; 
} 
 
void getCPUValues(long double * a){
	FILE *f;

    f = fopen("/proc/stat","r");
    int r = -1;
    r = fscanf(f,"%*s %Lf %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3],&a[4]);
    
    fclose(f);    
}

double calculateCPUUti(long double * a, long double * b){
	//printf("A: %Lf %Lf %Lf %Lf %Lf\n",a[0],a[1],a[2],a[3],a[4]);
	//printf("B: %Lf %Lf %Lf %Lf %Lf\n",b[0],b[1],b[2],b[3],b[4]);
	//long double bb = b[0]+b[1]+b[2]+b[4];
	//long double aa = a[0]+a[1]+a[2]+a[4];
	long double user = b[0] - a[0];
	long double nice = b[1] - a[1];
	long double system = b[2] - a[2];
	long double idle = b[3] - a[3];
	long double wa = b[4] - a[4];
	long double total = user + nice + system + idle + wa;
	long double per = 0.0;
	if(total != 0.0)		
		per = (( user + nice + system) * 100) / total;
	
	//printf("bb: %Lf\n", bb);
	//printf("aa: %Lf\n", aa);
	//printf("user: %Lf\n", user);
	//printf("nice: %Lf\n", nice);
	//printf("system: %Lf\n", system);
	//printf("idle: %Lf\n", idle);
	//printf("wa: %Lf\n", wa);
	//printf("total: %Lf\n", total);
	//printf("per: %Lf\n", per);

    return per;
}

double getCPUUti(){
 	long double a[5],b[5];
    FILE *fp;

	int r = -1;
    fp = fopen("/proc/stat","r");
    r = fscanf(fp,"%*s %Lf %Lf %Lf %Lf %Lf",&a[0],&a[1],&a[2],&a[3],&a[4]);
    //printf("A: %.2Lf %.2Lf %.2Lf %.2Lf %.2Lf\n", a[0],a[1],a[2],a[3],a[4]);
    fclose(fp);
    sleep(3);
    fp = fopen("/proc/stat","r");
    r = fscanf(fp,"%*s %Lf %Lf %Lf %Lf %Lf",&b[0],&b[1],&b[2],&b[3],&b[4]);
    //printf("B: %.2Lf %.2Lf %.2Lf %.2Lf %.2Lf\n", b[0],b[1],b[2],b[3],b[4]);
    fclose(fp);

	//long double bb = b[0]+b[1]+b[2]+b[4];
	//long double aa = a[0]+a[1]+a[2]+a[4];
	long double user = b[0] - a[0];
	long double nice = b[1] - a[1];
	long double system = b[2] - a[2];
	long double idle = b[3] - a[3];
	long double wa = b[4] - a[4];
	long double total = user + nice + system + idle + wa;
	long double per = (( user + nice + system) * 100) / total;
	//printf("u: %.2Lf, n: %.2Lf, s: %.2Lf, i: %.2Lf, total: %.2Lf, PERCENTRAGE: %.2Lf\n", user, nice, system, idle, total, per);
	//printf("PERCENTRAGE: %.2Lf\n",per);

    return per;
}

float getLowingLoad(float current_load, int max_load){
	return log((double)(30 - max_load) / (current_load - max_load)) / 0.0161;
}

float getRisingLoad(float current_load, int max_load){
	return log((double)(0 - max_load) / (current_load - max_load)) / 0.0161;
}

float getLoad(){
	struct sysinfo sys_info;

	if(sysinfo(&sys_info) != 0)
		perror("sysinfo");
		
	return sys_info.loads[0]/65536.0;
}

int getLoadValue2(float L1, float L2){
	int minLoadVal = 30;
	float minLoadTime = 1000.0;

	//printf("%.4f - %0.4f\n----------------------------------\n",  L1, L2);

	int L = 0;
	float est1 = 0.0, est2 = 0.0, dif = 0.0;
	if(L1 < L2){
	 // printf("Rising Load:\n----------------------------------\n");
	  for(L = 0; L < 30; L++){
		 if((L1 < L)  && (L2 < L))
		 {
			 est1 = getRisingLoad(L1, L);
			 est2 = getRisingLoad(L2, L);
			 dif = est2 - est1;
			 if(dif > 5.1){
				 if(minLoadTime > (dif - 5.1)){
					 minLoadTime = (dif - 5.1);
					 minLoadVal = L;
				 }
			 }else{
				 if(minLoadTime > (5.1 - dif)){
					 minLoadTime = (5.1 - dif);
					 minLoadVal = L;
				 }
			 }
			 //printf("stimation values(L : %d): (%.3f) - (%.3f) = [%.3f]\n", L, est1, est2, (est2 - est1));
		 }			
	  }
	}else if(L1 > L2){
	  //printf("Lowing Load:\n");
	  for(L = 0; L < 30; L++){
		 if((L1 > L)  && (L2 > L))
		 {
			 est1 = getLowingLoad(L1, L);
			 est2 = getLowingLoad(L2, L);
			 dif = est2 - est1;
			 if(dif > 5.1){
				 if(minLoadTime > (dif - 5.1)){
					 minLoadTime = (dif - 5.1);
					 minLoadVal = L;
				 }
			 }else{
				 if(minLoadTime > (5.1 - dif)){
					 minLoadTime = (5.1 - dif);
					 minLoadVal = L;
				 }
			 }
			 //printf("stimation values(L : %d): (%.3f) - (%.3f) = [%.3f]\n", L, est1, est2, (est2 - est1));
		 }			
	  }
	}else
		return (-1);
	
	return minLoadVal;
}

int getLoadValue(){
	float L1 = 0.0;
	float L2 = 0.0;
	int minLoadVal = 30;
	float minLoadTime = 1000.0;

	//printf("%.4f - %0.4f\n----------------------------------\n",  L1, L2);

	int L = 0;
	float est1 = 0.0, est2 = 0.0, dif = 0.0;
	if(L1 < L2){
	 // printf("Rising Load:\n----------------------------------\n");
	  for(L = 0; L < 30; L++){
		 if((L1 < L)  && (L2 < L))
		 {
			 est1 = getRisingLoad(L1, L);
			 est2 = getRisingLoad(L2, L);
			 dif = est2 - est1;
			 if(dif > 5.1){
				 if(minLoadTime > (dif - 5.1)){
					 minLoadTime = (dif - 5.1);
					 minLoadVal = L;
				 }
			 }else{
				 if(minLoadTime > (5.1 - dif)){
					 minLoadTime = (5.1 - dif);
					 minLoadVal = L;
				 }
			 }
			 //printf("stimation values(L : %d): (%.3f) - (%.3f) = [%.3f]\n", L, est1, est2, (est2 - est1));
		 }			
	  }
	}else if(L1 > L2){
	  //printf("Lowing Load:\n");
	  for(L = 0; L < 30; L++){
		 if((L1 > L)  && (L2 > L))
		 {
			 est1 = getLowingLoad(L1, L);
			 est2 = getLowingLoad(L2, L);
			 dif = est2 - est1;
			 if(dif > 5.1){
				 if(minLoadTime > (dif - 5.1)){
					 minLoadTime = (dif - 5.1);
					 minLoadVal = L;
				 }
			 }else{
				 if(minLoadTime > (5.1 - dif)){
					 minLoadTime = (5.1 - dif);
					 minLoadVal = L;
				 }
			 }
			 //printf("stimation values(L : %d): (%.3f) - (%.3f) = [%.3f]\n", L, est1, est2, (est2 - est1));
		 }			
	  }
	}else
		return L1;
	
	return minLoadVal;
}

//----------------------------------------------------------------------
//---------------------End of getting load amount-----------------------
//----------------------------------------------------------------------

void sendMoveCommand(int source_task_no, int source_worker, int target_worker){		
	int msg_code = 5;
	printf("[%d]Send Command to source_worker %d for %d\n", rank, source_worker, source_task_no);
	MPI_Send(&msg_code, 1, MPI_INT, source_worker, msg_code, MPI_COMM_WORLD);
	int move_command[2];
	move_command[0] = source_task_no;
	move_command[1] = target_worker;
	MPI_Send(move_command, 2, MPI_INT, source_worker, msg_code, MPI_COMM_WORLD);
	msg_code = 6;
	printf("[%d]Send Command to target_worker %d\n", rank, target_worker);
	//MPI_Send(&msg_code, 1, MPI_INT, target_worker, msg_code, MPI_COMM_WORLD);
	//MPI_Send(&source_worker, 1, MPI_INT, target_worker, msg_code, MPI_COMM_WORLD);
}

void sendInitialWorkerLoad(struct worker_load * l_load, int master){
	int msg_code = INIT_LOAD_FROM_WORKER;
	printf("[%d]. -----++++ Time when Sending info to master: %f\n", rank, MPI_Wtime());
	MPI_Send(&msg_code, 1, MPI_INT, master, msg_code, MPI_COMM_WORLD);
	MPI_Send(l_load, sizeof(struct worker_load), MPI_CHAR, master, msg_code, MPI_COMM_WORLD);
}

void sendWorkerLoad(void * d, int load_info_size, int master){
	int msg_code = 1;
	MPI_Send(&msg_code, 1, MPI_INT, master, msg_code, MPI_COMM_WORLD);
	MPI_Send( &load_info_size, 1, MPI_INT, master, msg_code, MPI_COMM_WORLD);
	MPI_Send( d, load_info_size, MPI_CHAR, master, msg_code, MPI_COMM_WORLD);
}

void obtainLatestLoad(int master){
	int msg_code = LATEST_LOAD_REQUEST;
	printf("[%d]. ************* t: %f **************** \n", rank, MPI_Wtime());
	MPI_Send(&msg_code, 1, MPI_INT, master, msg_code, MPI_COMM_WORLD);
}

void sendMoveReport(int* move_report, int report_size, int target){
	int msg_code = MOVE_REPORT_FROM_WORKER;
	MPI_Send(&msg_code, 1, MPI_INT, target, msg_code, MPI_COMM_WORLD);
	MPI_Send( &report_size, 1, MPI_INT, target, msg_code, MPI_COMM_WORLD);
	MPI_Send( move_report, report_size, MPI_INT, target, msg_code, MPI_COMM_WORLD);
}

void sendMoveReportToWorker(int target, int numTasks, int load_no){
	//double send_start = MPI_Wtime();
	//printf("[%d]. Mobility Overhead Estimation(sendMoveReportToWorker(target:%d)): %.6f\n", rank, target, (MPI_Wtime() - send_start));
	int msg_code = MOBILITY_REQUEST_FROM_WORKER;
	//printf("[%d]. Mobility Overhead Estimation(sendMoveReportToWorker(target:%d)): %.6f\n", rank, target, (MPI_Wtime() - send_start));
	//printf("[%d]. Move Request to(%d) : %d, w_num: %d, msg_code: %d\n", rank, load_no, target, numTasks, msg_code);	
	MPI_Send(&msg_code, 1, MPI_INT, target, msg_code, MPI_COMM_WORLD);
	//printf("[%d]. Mobility Overhead Estimation(sendMoveReportToWorker(target:%d)): %.6f\n", rank, target, (MPI_Wtime() - send_start));
	//Compose the move request where it has two main parts: the number of tasks to be moved and the load number
	int*msg_numTasks_load_no = (int*)malloc(sizeof(int)*2);
	*msg_numTasks_load_no = numTasks;
	*(msg_numTasks_load_no + 1) = load_no;
	//printf("[%d]. Mobility Overhead Estimation(sendMoveReportToWorker(target:%d)): %.6f\n", rank, target, (MPI_Wtime() - send_start));
	//MPI_Send( &numTasks, 1, MPI_INT, target, msg_code, MPI_COMM_WORLD);
	MPI_Send( msg_numTasks_load_no, 2, MPI_INT, target, msg_code, MPI_COMM_WORLD);
	//printf("[%d]. Mobility Overhead Estimation(sendMoveReportToWorker(target:%d)): %.6f\n", rank, target, (MPI_Wtime() - send_start));
	free(msg_numTasks_load_no);
}

void sendMoveReportToWorkers(struct worker_move_report * w_m_report, struct worker_load * report){
	int i_w = 0;
	for(;i_w<numprocs-1; i_w++){
		if(i_w != rank-1){
			if((w_m_report + i_w)->num_of_tasks > 0){		
				int w = (w_m_report + i_w)->w_id;						
				sendMoveReportToWorker((w_m_report + i_w)->w_id+1, (w_m_report + i_w)->num_of_tasks, (report + w)->w_load_no);
			}		
			//sendMoveReportToWorker((w_m_report + i_w)->w_id+1, 0, 0);
		}
	}
}

//type: 1 -- mobile request recieved
//type: 2 -- mobile confirmation received
void updateMobileTaskReport(struct task_pool * t_pool, int task_no, int type, int w){
	struct task_pool * p = t_pool;

	for(;p!=NULL; p=p->next){
		if(p->m_task_report->task_no == task_no){
			if(type == 1){
				//printf("Changing the status of %d to on Move...\n", task_no);
				p->m_task_report->task_status = 3;
				p->m_task_report->m_dep_time[p->m_task_report->mobilities] = MPI_Wtime();
				break;
			}else if(type == 2){
				//printf("Changing the status of %d to on Processing again...\n", task_no);
				p->m_task_report->task_status = 1;
				p->m_task_report->m_arr_time[p->m_task_report->mobilities] = MPI_Wtime();
				p->m_task_report->mobilities++;
				p->m_task_report->task_worker = w;
			}
		}
	}
}

void recvMoveReport(struct task_pool * t_pool, int w, int msg_code){
	MPI_Status status;
	int rep_size = 0;
	MPI_Recv(&rep_size, 1, MPI_INT, w, msg_code, MPI_COMM_WORLD, &status);
	int * move_rep = (int*)malloc(sizeof(int)*rep_size);
	MPI_Recv(move_rep, rep_size, MPI_INT, w, msg_code, MPI_COMM_WORLD, &status);
	
	int i=0;
	printf("Report from worker: %d\n", w);
	for(i=0; i < rep_size/2; i++){
		//printf("Move the task: %d to %d\n",*(move_rep+(2*i)), *(move_rep+(2*i)+1));
	
	//Send Move Commands to the source worker and the targer worker
	sendMoveCommand(*(move_rep+(2*i)), w, *(move_rep+(2*i)+1));
	
	//update task report
	updateMobileTaskReport(t_pool, *(move_rep+(2*i)), 1, w);
}
	/*
	//Send Move Commands to the source worker and the targer worker
	if(w == 1)
	{
		sendMoveCommand(0, 1, 2);
		
		//update task report
		updateMobileTaskReport(t_pool, 1, 1, w);
	}else{
		sendMoveCommand(0, 2, 1);
		
		//update task report
		updateMobileTaskReport(t_pool, 2, 2, w);
	}
	*/
}

struct worker_local_load * addWorkerLocalLoad(struct worker_local_load * wlLoad, 
									float t_per, float t_sec, float t_load_avg, 
									int t_est_load_avg, long double t_cpu_uti, int t_run_proc){
		
	if(wlLoad == 0){
		wlLoad = (struct worker_local_load *)malloc(sizeof(struct worker_local_load));
		wlLoad->per = t_per;
		wlLoad->sec = t_sec;
		wlLoad->load_avg = t_load_avg;
		wlLoad->est_load_avg = t_est_load_avg;
		wlLoad->w_cpu_uti = t_cpu_uti;
		wlLoad->w_running_procs = t_run_proc;
		wlLoad->next = NULL;		
	}else{
		struct worker_local_load * pl = wlLoad;
		while(pl->next != NULL){
			pl = pl->next;		
		}		
		pl->next = (struct worker_local_load *)malloc(sizeof(struct worker_local_load));
		pl->next->per = t_per;
		pl->next->sec = t_sec;
		pl->next->load_avg = t_load_avg;
		pl->next->est_load_avg = t_est_load_avg;
		pl->next->w_cpu_uti = t_cpu_uti;
		pl->next->w_running_procs = t_run_proc;
		pl->next->next = 0;
	}
	return wlLoad;
}

void recordLocalR(struct worker_task * w_ts, float P, int cores, double t_cpu_uti, int t_run_proc){
	struct worker_task * p = w_ts->next;
	while(p != NULL){
		if(!p->m_task->done && p->move_status != 1)
		{
			//printf("[%d]. Load Overhead Record(Details: %.2f)", rank, p->local_R);
			if(p->local_R == 0)
				p->local_R = getActualRelativePower(P, t_cpu_uti, 0, t_run_proc, cores, 0, rank);
			else
				p->local_R = (p->local_R + getActualRelativePower(P, t_cpu_uti, 0, t_run_proc, cores, 0, rank))/2;
		}
		p = p->next;
	}
}

void recordLocalLoad(struct worker_task * w_ts, float t_load_avg, 
					 int t_est_load_avg, long double t_cpu_uti, int t_run_proc){
	//printf("[%d]. recordLocalLoad(%lx)...\n", rank, w_ts);
	struct worker_task * p = w_ts->next;
	float t_per = 0;
	float t_sec = 0;
	while(p != NULL){
		if(!p->m_task->done && p->move_status != 1)
		{
			int i = p->m_task->main_counter;
			t_per = ((i * 100) / (float)p->m_task->main_counter_max);
			t_sec = MPI_Wtime() - p->w_task_start;
			//printf("[%d]. task_id: %d, i: %.2f%%  - sec: %.2f - rp: %d\n", rank, p->task_id, t_per, t_sec, t_run_proc);
			//if(rank == 1)
			//printf("[%d]. task_id: %d, done: %d- sec: %.2f - per: %.2f\n", rank, p->task_id, p->m_task->done, t_per, t_sec);
			p->w_l_load = addWorkerLocalLoad(p->w_l_load, t_per, t_sec, t_load_avg, t_est_load_avg, t_cpu_uti, t_run_proc);
			//printMobileTask(p->m_task);
			//						
			//p->w_l_load = addWorkerLocalLoad(p->w_l_load, t_per, t_sec, 
			//					t_load_avg, t_est_load_avg, t_cpu_uti, t_run_proc)
			//
		}
		p = p->next;
	}
}

//OLD
float getPreviousR(struct worker_local_load * l_p, float P, int cores){
	float total_relative_power = 0;
	int count = 0;
	float partial_relative_power = 0;
	
	while(l_p != NULL){
		//printf("[%d]. sec: %.2f - per: %.2f - load_avg: %.2f - est_load_avg: %d - w_cpu_uti: %.2Lf - w_running_procs: %d\n", 
		//	rank, l_p->sec, l_p->per, l_p->load_avg, l_p->est_load_avg, l_p->w_cpu_uti, l_p->w_running_procs);
		//
		partial_relative_power = getActualRelativePower(P, l_p->w_cpu_uti, l_p->est_load_avg, l_p->w_running_procs, cores, 0, rank);
		total_relative_power = total_relative_power + (partial_relative_power);
		count++;
		//printf("[%d]- partial_relative_power: %.3f, total_relative_power: %.3f\n", rank, partial_relative_power, total_relative_power);
		//
		l_p = l_p->next;
	}
	//printf("-----------------------------------------------------------\n");
	
	//The relative computing power for the previous time
	if(count > 0)
		return (total_relative_power / count);
	return 0;
}

float* getEstimatedExecutionTime(struct worker_load_task * w_l_t, int n, struct worker_task * p , int * dest_nps, struct estimated_cost * e_c, int cur_tasks){
	//double estimator_start = MPI_Wtime();
	
	struct  worker_load w_local_loads = w_l_t->w_local_loads;
	int i = p->m_task->main_counter;
	//The work done here
	float Wd_before = p->m_task->m_work_start[p->m_task->moves-1];
	float Wd = ((i * 100) / (float)p->m_task->main_counter_max) - p->m_task->m_work_start[p->m_task->moves-1];
	//The time spent here to process Wd
	float Te = MPI_Wtime() - p->w_task_start;
	//the work left
	float Wl = 100 - Wd - Wd_before;
	//printf("[%d]- work done: old: %.3f, new: %.3f\n", rank, Wd, p->m_task->m_work_start[p->m_task->moves-1]);	//printf("[%d]- Time elapsed: %.3f\n", rank, Te);
	
	
	//Total machine power(P=speed*cores)
	float P = w_local_loads.w_cpu_speed;
	//Number of cores for this machine
	int cores = w_local_loads.w_cores;
	
	//Number of running processes	
	//int np = 0;
	float CPU_UTI = w_local_loads.w_cpu_uti_2;
	int ESTIMATED_LOAD_AVG = w_local_loads.estimated_load;
	int RUNNING_PROCESSES = w_local_loads.w_running_procs;
	
	float Rhn = getActualRelativePower(P, CPU_UTI, ESTIMATED_LOAD_AVG, RUNNING_PROCESSES, cores, *(dest_nps + rank -1), rank);		

	//*************************************************************************************
	//Calculating the relative computing power for the previous time HERE
	
	//float Rhe = getPreviousR(p->w_l_load, P, cores);
	float Rhe = p->local_R;

	//*************************************************************************************
	
	
	float Th = (Wl * Rhe * Te)/(Wd * Rhn);
	
	//printf("[%d]- Local Freq: %.2f, Relative Power: %.2f, Averagve Relative Power: %.2f, Work left: %.3f%%, Work done: %.3f%%, Task time: %.3f\n", rank, P, Rhn, Rhe, Wl, Wd, Te);
		
	//*************************************************************************************
	
	//printf("[%d]. Mobility Overhead Estimation EC(INIT): %.6f\n", rank, (MPI_Wtime() - estimator_start));
	
	//n: number of workers
	//T: Array of all estimated execution times
	float *T = (float *)malloc(sizeof(float) * n);
	
	struct worker_load * w_loads = w_l_t->w_loads;
	
	int i_T = 0;
	float Tn;
	for(; i_T < n; i_T++){
		if(i_T == rank-1){
			T[i_T] = Th;
		}else{
			float remote_power = (w_loads + i_T)->w_cpu_speed;
			float remote_cpu_uti = (w_loads + i_T)->w_cpu_uti_2;
			int remote_ext_load_avg = (w_loads + i_T)->estimated_load;
			int remote_running_procs = (w_loads + i_T)->w_running_procs;
			int remote_cores = (w_loads + i_T)->w_cores;
			float Rnn = getActualRelativePower(remote_power, remote_cpu_uti, remote_ext_load_avg, remote_running_procs + 1, remote_cores, *(dest_nps + i_T), (w_loads + i_T)->w_rank);
			Tn = (Wl * Rhe * Te)/(Wd * Rnn);
			T[i_T] = Tn;
		}
	}
	
	//printf("[%d]. Mobility Overhead Estimation EC(Possiblities): %.6f\n", rank, (MPI_Wtime() - estimator_start));
	
	//printf("[%d]. (TASK: %d)The Estimated Eecuting Time: %.3f, Finish Time: %.3f\n", rank, p->task_id, Th, (Te+Th));
	
	///*******************************************************
	e_c->task_no = p->m_task->m_task_id;
	e_c->cur_EC = Th;
	e_c->spent_here = Te;
	int i_t = 0, i_w = 0;
	for(i_t = 0; i_t < cur_tasks-1; i_t++){
		
		float Rhn = getActualRelativePower(P, CPU_UTI, ESTIMATED_LOAD_AVG, RUNNING_PROCESSES, cores, (i_t*-1)-1, rank);		

		float Th = (Wl * Rhe * Te)/(Wd * Rhn);
		if((Wd * Rhn) != 0)
			e_c->cur_EC_after[i_t] = Th;
		else
			e_c->cur_EC_after[i_t] = 0;
	}
	
	//printf("[%d]. Mobility Overhead Estimation EC(Possiblities 1): %.6f\n", rank, (MPI_Wtime() - estimator_start));
	
	for(i_T = 0; i_T < n; i_T++){
		if(i_T != rank-1 && (w_loads + i_T)->locked == 0){
			//printf("[%d]worker: %d---\n", rank, i_T);
			(e_c->other_ECs + i_w)->w_no = i_T;	
			(e_c->other_ECs + i_w)->move_cost = getMoveTime(p->m_task);
			getPredictedMoveTime(p->m_task, w_l_t, i_T + 1);
			float remote_power = (w_loads + i_T)->w_cpu_speed;
			float remote_cpu_uti = (w_loads + i_T)->w_cpu_uti_2;
			int remote_ext_load_avg = (w_loads + i_T)->estimated_load;
			int remote_running_procs = (w_loads + i_T)->w_running_procs;
			int remote_cores = (w_loads + i_T)->w_cores;
					
			//All Estimated costs for all workers
			for(i_t = 0; i_t < cur_tasks; i_t++){		
				
				float Rnn = getActualRelativePower(remote_power, remote_cpu_uti, remote_ext_load_avg, remote_running_procs, remote_cores, (i_t + 1), (w_loads + i_T)->w_rank);				
				if((Wd * Rnn) != 0)
					Tn = (Wl * Rhe * Te)/(Wd * Rnn);
				else
					Tn = 0;
				//printf("[%d]. EC: %.3f @ %d where Rnn: %.3f\n", rank, Tn, i_w , Rnn);
				(e_c->other_ECs + i_w)->costs[i_t] = Tn;				
			}
			
			i_w++;			
		}		
	}
	
	//printf("[%d]. Mobility Overhead Estimation EC(Possiblities 2): %.6f\n", rank, (MPI_Wtime() - estimator_start));
	///*******************************************************
	
	//printf("[%d]. (TASK: %d)The Estimated Eecuting Time: %.3f, Finish Time: %.3f\n", rank, p->task_id, Th, (Te+Th));
	
	return T;	
}

void printEstimationCost(struct estimated_cost * e_c, int cur_tasks, int other_worker_count){	
	//PRINT THE ESTIMATION COST
	int i_t = 0, i_t2 = 0, i_w = 0;
	printf("************************************************\n");
	printf("********  ESTIMATION COST FOR %d ***************\n", rank);
	printf("************************************************\n");
	for(i_t=0; i_t<cur_tasks; i_t++){
		printf("task: %d , EC: %.3f, Spent Here: %.3f\n", (e_c + i_t)->task_no, (e_c + i_t)->cur_EC, (e_c + i_t)->spent_here);
		printf("\tEC HERE: ");
		for(i_t2=0; i_t2<cur_tasks-1; i_t2++)	
			printf("%10.3f", (e_c + i_t)->cur_EC_after[i_t2]);
		printf("\n");
		for(i_w=0; i_w<other_worker_count; i_w++){
			printf("\tECs @ %d :", (((e_c + i_t)->other_ECs + i_w)->w_no));
			for(i_t2=0; i_t2<cur_tasks; i_t2++){
				printf("%10.3f", (((e_c + i_t)->other_ECs + i_w)->costs[i_t2]));
			}
			printf(" [Move Cost: %.4f]", ((e_c + i_t)->other_ECs + i_w)->move_cost);
			printf("\n");
		}
		printf("\tNew Allocation: w: %d, EC: %.3f\n", (e_c + i_t)->to_w, (e_c + i_t)->to_EC);
	}
	printf("\n************************************************\n\n");
}

void findBestTaskMapping(struct estimated_cost * e_c, int cur_tasks, int w_count){
	int * task_mapping = (int*)malloc(sizeof(int)*(w_count));
	int i=0, i_t=0, i_new_w=0;
	//initialize task mapping
	for(i=0; i<w_count+1; i++)
		task_mapping[i]=0;
	task_mapping[rank-1] = cur_tasks;
	
	//initialize the task allocation
	for(i_t=0; i_t<cur_tasks; i_t++){
		(e_c + i_t)->to_w = rank -1;
		(e_c + i_t)->to_EC = (e_c + i_t)->cur_EC;
		//Add the move cost
		//printf("[%d]Adding move cost\n", rank);
		for(i=0; i<w_count; i++){
			for(i_new_w=0; i_new_w<cur_tasks; i_new_w++){
				//printf("[%d]w: %d, EC: %.3f + %.3f = %.3f \n", rank, i_new_w, ((e_c + i_t)->other_ECs+i)->costs[i_new_w], ((e_c + i_t)->other_ECs+i)->move_cost, ((e_c + i_t)->other_ECs+i)->costs[i_new_w] + ((e_c + i_t)->other_ECs+i)->move_cost);
				((e_c + i_t)->other_ECs+i)->costs[i_new_w] = ((e_c + i_t)->other_ECs+i)->costs[i_new_w] + ((e_c + i_t)->other_ECs+i)->move_cost;
			}
		}
	}
	//Print task mapping
	//printf("[%d]. Task Mapping: ", rank);
	//for(i=0; i<w_count; i++)
	//	printf("%5d", task_mapping[i]);
	//printf("\n");
	//
	int trial = 0;
	int improved = 1;
	do{
		//Find the slowest task;
		//int s_t = e_c->task_no;
		//float s_ec = e_c->to_EC;
		int s_t = -1;
		float s_ec = -1;
		//To select the first task which is the slwoest
		for(i_t=0; i_t<cur_tasks; i_t++){
			//to garantee that the current task has spent over than 2 sec for having good estimation cost
			if((e_c + i_t)->spent_here > 2){				
				s_t = (e_c + i_t)->task_no;
				s_ec = (e_c + i_t)->to_EC;
				//printf("[%d]. task_no: %d, %.3f\n", rank, s_t, s_ec);
				break;
			}			
		}
		
		//break if there is no task whose spent time here noe exceed 2 sec
		if(s_t == -1) break;
		
		for(i_t=0; i_t<cur_tasks; i_t++){			
			if((e_c + i_t)->spent_here > 2){
				//printf("[%d]. task_no: %d, %.3f\n", rank, (e_c + i_t)->task_no, (e_c + i_t)->to_EC);
				if((e_c + i_t)->to_EC > s_ec){
					s_ec = (e_c + i_t)->to_EC;
					s_t = (e_c + i_t)->task_no;					
				}
			}
		}
		
		//printf("[%d]The slowest task is: %d with %.3f\n", rank, s_t, s_ec);
		
		//Find the best new location for the slowest task
		for(i_t=0; i_t<cur_tasks; i_t++){
			if((e_c + i_t)->task_no == s_t){
				int new_w = (e_c + i_t)->to_w;
				float new_EC = s_ec;
				for(i=0; i<w_count; i++){
					int w = ((e_c + i_t)->other_ECs + i)->w_no;
					//printf("[%d]w: %d, new_EC: %.3f, task_mapping[w]: %d, other: %.3f\n", rank, w, new_EC, task_mapping[w], ((e_c + i_t)->other_ECs + i)->costs[task_mapping[w]]);
					if (((e_c + i_t)->other_ECs + i)->costs[task_mapping[w]] < new_EC){
						new_EC = ((e_c + i_t)->other_ECs + i)->costs[task_mapping[w]];
						new_w = w;
					}
				}
				
				if(new_w != (e_c + i_t)->to_w){
					//printf("CONGRATULATION for finding new location for the task: %d with EC: %.3f\n", new_w, new_EC);
					(e_c + i_t)->to_w = new_w;
					(e_c + i_t)->to_EC = new_EC;	
					//change the mapping depending on the new findings
					task_mapping[rank-1] = task_mapping[rank-1] - 1;
					task_mapping[(e_c + i_t)->to_w] = task_mapping[(e_c + i_t)->to_w] + 1;
				}else
					improved = 0;
				break;
				
			}
		}
		
		//Print task mapping
		//printf("[%d]. New Task Mapping: ", rank);
		//for(i=0; i<w_count+1; i++)
		//	printf("%5d", task_mapping[i]);
		//printf("\n");
		//
		
		if(improved == 1){
			//Update the task allocation details depending on the new findings
			for(i_t=0; i_t<cur_tasks; i_t++){
				//printf("[%d]. task: %d\n", rank, (e_c + i_t)->task_no);
				///if((e_c + i_t)->spent_here > 3)
				if((e_c + i_t)->spent_here > 2)
					if((e_c + i_t)->task_no != s_t){
						//printf("[%d]. to_w: %d\n", rank, (e_c + i_t)->to_w);
						if((e_c + i_t)->to_w == rank -1){
							//printf("[%d]. task_mapping[(e_c + i_t)->to_w]: %d\n", rank, task_mapping[(e_c + i_t)->to_w]);					
							//printf("[%d]. (e_c + i_t)->cur_EC_after[cur_tasks - task_mapping[(e_c + i_t)->to_w] - 1]: %f\n", rank, (e_c + i_t)->cur_EC_after[cur_tasks - task_mapping[(e_c + i_t)->to_w] - 1]);
							(e_c + i_t)->to_EC = (e_c + i_t)->cur_EC_after[cur_tasks - task_mapping[(e_c + i_t)->to_w] - 1];
						}else{						
							int i_new_ec = 0;
							for(i_new_ec=0; i_new_ec<w_count; i_new_ec++){
								//printf("[%d]. i_new_ec: %d - w_no: %d, task_mapping[%d]: %d, \n", rank, i_new_ec, ((e_c + i_t)->other_ECs + i_new_ec)->w_no, (e_c + i_t)->to_w, task_mapping[(e_c + i_t)->to_w]);
								if(((e_c + i_t)->other_ECs + i_new_ec)->w_no == task_mapping[(e_c + i_t)->to_w]){
									//printf("[%d]. (e_c + i_t)->to_w: %d\n", rank, (e_c + i_t)->to_w);	
									//printf("[%d]. task_mapping[(e_c + i_t)->to_w]: %d\n", rank, task_mapping[(e_c + i_t)->to_w]);	
									//printf("[%d]. i_new_ec: %d\n", rank, i_new_ec);	
									//printf("[%d]. ((e_c + i_t)->other_ECs + i_new_ec)->costs[task_mapping[(e_c + i_t)->to_w] - 1]: %.3f\n", rank, ((e_c + i_t)->other_ECs + i_new_ec)->costs[task_mapping[(e_c + i_t)->to_w] - 1]);	
									(e_c + i_t)->to_EC = ((e_c + i_t)->other_ECs + i_new_ec)->costs[task_mapping[(e_c + i_t)->to_w] - 1];
								}						
							}
	//						(e_c + i_t)->to_EC = ((e_c + i_t)->other_ECs + (e_c + i_t)->to_w)->costs[task_mapping[(e_c + i_t)->to_w] - 1];
						}
					}
			}
		}

		trial++;
		
		//printEstimationCost(e_c, cur_tasks, w_count);
		
	}while(improved == 1);
}


void calculatingMoveTime(struct mobile_task* w_mt, struct worker_load w_l){
	
	
	w_mt->move_stats.end_move_time = MPI_Wtime();		
	
	w_mt->move_stats.move_time = (w_mt->move_stats.end_move_time - w_mt->move_stats.start_move_time);	
	
	w_mt->move_stats.R_dest = getActualRelativePower(w_l.w_cpu_speed, w_l.w_cpu_uti_2, w_l.estimated_load, w_l.w_running_procs, w_l.w_cores, 0, rank);
	w_mt->move_stats.data_size = getInitialTaskSize(w_mt);
	
	printf("[%d].[RECV] ping time: %f, move time: %f, Rs: %.2f, Rd: %.2f, data size: %d\n", rank, w_mt->move_stats.net_time, w_mt->move_stats.move_time, w_mt->move_stats.R_source, w_mt->move_stats.R_dest, w_mt->move_stats.data_size);
	printf("[%d].[RECV] ping time: %f, move time: %f, Rs: %.2f, Rd: %.2f, data size: %d\n", rank, w_l.net_times.init_net_time, w_mt->move_stats.move_time, w_mt->move_stats.R_source, w_mt->move_stats.R_dest, w_mt->move_stats.data_size);	
	//printf("[%d] The time recorded at the master: %f , %f = %f\n", rank, m_start, m_end, (m_end - m_start));
	
}

int checkNetworkLatency(struct worker_move_report * w_m_report, struct worker_load * report, int n){
	int i_w = 0;
	int valid_net_lat = 1;
	for(;i_w<numprocs-1; i_w++){
		if(i_w != rank-1){
			if((w_m_report + i_w)->num_of_tasks > 0){		
				printf("[%d]. checkNetworkLatency: name: %s\n", rank, report[i_w].w_name);
				double net_lat = sendPing(report[i_w].w_name);
				report[i_w].net_times.cur_net_time = net_lat;
				if(net_lat > NET_LAT_THRESHOLD)
					valid_net_lat = valid_net_lat && 0;
				printf("[%d]. checkNetworkLatency: net_lat: %.3f, valid_net_lat: %d\n", rank, net_lat, valid_net_lat);
			}		
		}
	}
	return valid_net_lat;
}

void *worker_estimator(void * arg){
	//double estimator_start = MPI_Wtime();
	double estimator_mid = 0;
	
	struct worker_load_task * w_l_t = (struct worker_load_task *)(arg);
	struct worker_task * p;
	int cur_tasks=0;
	
	#ifdef SYS_gettid
	pid_t tid = syscall(SYS_gettid);
	#else
	#error "SYS_gettid unavailable on this system"
	#endif
	
	w_l_t->estimator_tid = tid;
	//printf("[%d]. Mobility Overhead Estimation(1): %.7f\n", rank, (MPI_Wtime() - estimator_start));
	
	//printWorkerLoadReport(w_l_t->w_loads, numprocs);
	
	//printf("[%d]. Mobility Overhead Estimation(11): %.7f\n", rank, (MPI_Wtime() - estimator_start));
	
	struct worker_move_report * w_m_report = w_l_t->move_report;	
	
	//printf("[%d]. Mobility Overhead Estimation(INIT): %.7f\n", rank, (MPI_Wtime() - estimator_start));
	estimator_mid = MPI_Wtime();	
		
	int i_d = 0;

	{
	//sleep(ESTIMATOR_BREAK_TIME);		
		
		p = w_l_t->w_tasks->next;
		cur_tasks = 0;
		while(p != NULL){
			//printf("******************************************[%d], t_id: %d\n", rank, p->m_task->m_task_id);
			if(!p->m_task->done && (p->move_status != 1))
				cur_tasks++;
			p = p->next;			
		}
		
			
		//printf("[%d]. cur_tasks: %d\n", rank, cur_tasks);
			
		p = w_l_t->w_tasks->next;
		if(cur_tasks > 0){
			
			//new wights after changing the task mapping
			int * dest_nps = (int*)malloc(sizeof(int)*(numprocs-1));
			
			for(i_d=0; i_d<numprocs-1; i_d++)
				*(dest_nps + i_d) = 0;
				
			//init the estimation report
			int i_t = 0, i_w = 0;
			//int other_worker_count = (numprocs-2);
			int other_worker_count = 0;
			for(i_w=0; i_w<numprocs-1; i_w++){
				if(i_w != rank-1)
					if((w_l_t->w_loads + i_w)->locked == 0)
						other_worker_count++;
			}
			
			//printf("[%d]. other_worker_count: %d\n", rank, other_worker_count);
			struct estimated_cost * e_c = (struct estimated_cost *)malloc(sizeof(struct estimated_cost) * cur_tasks); 
			for(i_t=0; i_t<cur_tasks; i_t++){
				(e_c + i_t)->cur_EC_after = (float*) malloc(sizeof(float) * (cur_tasks - 1));
				(e_c + i_t)->other_ECs = (struct other_estimated_costs*) malloc(sizeof(struct other_estimated_costs) * other_worker_count);
				for(i_w=0; i_w<other_worker_count; i_w++){
					//((e_c + i_t)->other_ECs + i_w)->w_no = i_w;
					((e_c + i_t)->other_ECs + i_w)->costs = (float *) malloc(sizeof(float) * cur_tasks);
				}
			}
			///*************************************
			
			
			//printf("[%d]. Mobility Overhead Estimation(INIT): %.6f\n", rank, (MPI_Wtime() - estimator_start));
			//estimator_mid = MPI_Wtime();
			
			i_t = 0;
			int report_done = 0;
			do{
				p = w_l_t->w_tasks->next;
				i_t = 0;
				while(p != NULL){					
					if(!p->m_task->done && (p->move_status != 1))
					{	
						if(p->estimating_move == NULL){
							//get the estimated execution time for this task
							float * T = getEstimatedExecutionTime(w_l_t, numprocs-1, p, dest_nps, (e_c + (i_t++)), cur_tasks);						
						
							//Set the estimation costs on all other worker for this task
							p->estimating_move = (struct estimation *)malloc(sizeof(struct estimation));
							p->estimating_move->estimation_costs = T;							
							p->estimating_move->done = 0;
							p->estimating_move->chosen_dest = -1;
							p->estimating_move->gain_perc = 0;
							p->estimating_move->on_dest_recalc = -1;
						}else{
							if(p->estimating_move->done != 1){
								//get the estimated execution time for this task
								float * T = getEstimatedExecutionTime(w_l_t, numprocs-1, p, dest_nps, (e_c + (i_t++)), cur_tasks);						
							
								//Set the estimation costs on all other worker for this task
								p->estimating_move = (struct estimation *)malloc(sizeof(struct estimation));
								p->estimating_move->estimation_costs = T;							
								p->estimating_move->done = 0;
								p->estimating_move->chosen_dest = -1;
								p->estimating_move->gain_perc = 0;
								p->estimating_move->on_dest_recalc = -1;
							}
						}
						
						//printf("[%d]. Mobility Overhead Estimation(EC:t_id:%d): %.6f\n", rank, p->m_task->m_task_id ,(MPI_Wtime() - estimator_start));
						
						///---------------------------------------------------------------------------------
						//if(p->estimating_move->done != 1){		
							//printf("[%d]- ESTIMATED for %d\n", rank, p->task_id);				
							//int k = 0;
							////float move_cost = getMoveTime(p->m_task);
							//for(k=0 ;k<numprocs-1; k++){
							//	if(k != rank-1){
									//p->estimating_move->estimation_costs[k] = p->estimating_move->estimation_costs[k] + move_cost;
									//printf("[%d]- MOVE COST: %f\n", rank, move_cost);
							//	}
							//	float ec = p->estimating_move->estimation_costs[k];
							//	float tc = (MPI_Wtime() - p->m_task->m_start_time[0]) + ec;
							//	printf("[%d]- EC for task %d @ %d is %.3f[TOTAL: %.3f]\n", rank, p->task_id, k, ec, tc);
							//}
						//}
						
						///---------------------------------------------------------------------------------
						
						
						
					}	
					
					p = p->next;				
				}
				
				//printf("[%d]. Mobility Overhead Estimation(EC): %.6f\n", rank, (MPI_Wtime() - estimator_mid));
				estimator_mid = MPI_Wtime();
				
				//printf("[%d]. Mobility Overhead Estimation(5): %.7f\n", rank, (MPI_Wtime() - estimator_start));
				
				//if(new_estimation == 0)
				{					
					//printEstimationCost(e_c, cur_tasks, other_worker_count);
					
					//printf("[%d]. Mobility Overhead Estimation(6): %.7f\n", rank, (MPI_Wtime() - estimator_start));
					
					findBestTaskMapping(e_c, cur_tasks, other_worker_count);
					
					//printEstimationCost(e_c, cur_tasks, other_worker_count);
					///*************************************
					
					///*************************************
					
					//printf("[%d]. Mobility Overhead Estimation(BEST): %.6f\n", rank, (MPI_Wtime() - estimator_mid));
					estimator_mid = MPI_Wtime();
					
					//Init move report
					int i_r = 0;
					for(i_r=0; i_r<numprocs-1; i_r++){
						(w_m_report + i_r)->w_id = i_r;
						(w_m_report + i_r)->num_of_tasks = 0;
						(w_m_report + i_r)->list_of_tasks = (int *)malloc(sizeof(int)*cur_tasks);
					}
					
					for(i_t=0; i_t<cur_tasks; i_t++){
						for(i_r=0; i_r<numprocs-1; i_r++){
							if((w_m_report + i_r)->w_id == (e_c + i_t)->to_w){
								(w_m_report + i_r)->list_of_tasks[(w_m_report + i_r)->num_of_tasks] = (e_c + i_t)->task_no;					
								(w_m_report + i_r)->num_of_tasks = (w_m_report + i_r)->num_of_tasks + 1;							
							}
						}
					}
					
					
					
					///-----------------------------------------------------------------------------------------
					//printf("[%d]. Mobility Overhead Estimation(8): %.7f\n", rank, (MPI_Wtime() - estimator_start));
					
					//Printing the move report
					//int i_tt = 0;
					//printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
					//printf("[%d]. *-*-*-*-*-*-* MOVE REPORT *-*-*-*-*-*-*\n",rank);
					//printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
					//printf("w_id | w_num | Nos...\n");
					//printf("*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
					//for(i_r=0; i_r<numprocs-1; i_r++){
					//	printf("[%d]  %d  |   %d   |  ", rank, (w_m_report + i_r)->w_id, (w_m_report + i_r)->num_of_tasks);
					//	for(i_tt=0;i_tt<(w_m_report + i_r)->num_of_tasks; i_tt++){
					//		printf("%6d ", (w_m_report + i_r)->list_of_tasks[i_tt]);
					//	}
					//	printf("\n");
					//}
					//printf("\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n");
					///-----------------------------------------------------------------------------------------
					
					report_done = 1;
				}				

			}while(report_done == 0);
			
			//printf("[%d]. Mobility Overhead Estimation(9): %.7f\n", rank, (MPI_Wtime() - estimator_start));
			
			//create move report
			p = w_l_t->w_tasks->next;
			while(p != NULL){
				if(!p->m_task->done && (p->move_status != 1))
				{	
					//printf("task: %d, done: %d, destination: %d, gain: %.3f\n", p->task_id, p->estimating_move->done, p->estimating_move->chosen_dest, p->estimating_move->gain_perc);					
					p->estimating_move = NULL;
				}
				p = p->next;
			}
			
		}
		
		//printWorkerLoadReport(w_l_t->w_loads, numprocs);
		
		//printf("[%d]. Mobility Overhead Estimation(10): %.7f\n", rank, (MPI_Wtime() - estimator_start));
		
		//double report_start = MPI_Wtime();
		
		if(cur_tasks > 0){			
			//if(checkNetworkLatency(w_l_t->move_report, w_l_t->w_loads, numprocs) == 1)				
			sendMoveReportToWorkers(w_l_t->move_report, w_l_t->w_loads);
			
			//printWorkerLoadReport(w_l_t->w_loads, numprocs);
			//else 
			//					
		}		
		
		//printf("[%d]. Mobility Overhead Estimation(REPROT): %.6f\n", rank ,(MPI_Wtime() - report_start));
		//estimator_mid = MPI_Wtime();
	}
	
	//printf("[%d]. worker_estimator.\n", rank);
	
	//double estimator_end = MPI_Wtime();
	//printf("[%d]. Mobility Overhead Estimation: %.6f\n", rank, (estimator_end - estimator_start));

	return NULL;
}

void setLocalLoadInfo(long double * a, long double * b, long double * per, int *rp, float * load_avg){
	
	double load_overhead_1 = 0.0;	
	double load_overhead_2 = 0.0;	
	double load_overhead_sum = 0.0;
	
	long double tmp_per = 0;
	int tmp_rp = 0;	
	
	int READ_COUNT = 5;
	int READ_DELAY = 1000000 / 	READ_COUNT;
	
	int i=0;
	for(i = 0; i < READ_COUNT; i++){	
		usleep(READ_DELAY);
	
		a[0] = b[0];
		a[1] = b[1];
		a[2] = b[2];
		a[3] = b[3];
		a[4] = b[4];
		
		load_overhead_1 = MPI_Wtime();
		getCPUValues(b);

		tmp_per += calculateCPUUti(a, b);		
		
		tmp_rp += getRunningProc()-1;
		
		load_overhead_2 = MPI_Wtime();
		
		load_overhead_sum += (load_overhead_2 - load_overhead_1);		
		
		
		//printf("[%d].{%d} LOCAL LOAD INFO: CPU_UTI: %Lf, RP: %d\n", rank, i, tmp_per, tmp_rp);
	}			
	
	///double t1 = MPI_Wtime();
	
	
	*per = tmp_per / READ_COUNT;
	
	//state_worker = getProcessState(w_l_t->worker_tid, 0, 0);
	//state_estimator = getProcessState(w_l_t->worker_tid, 1, w_l_t->estimator_tid);
	*rp = tmp_rp / READ_COUNT;
	
	load_overhead_1 = MPI_Wtime();
	
	*load_avg = getLoad();
	
	load_overhead_2 = MPI_Wtime();
	
	//load_overhead_sum += (load_overhead_2 - load_overhead_1);
	//printf("[%d]. Load Overhead OBTAINING: %.6f \n", rank, (load_overhead_sum));	
	
	///double t2 = MPI_Wtime();
	
	///printf("[%d]. TIME-11: %.6f (%.6f)\n", rank,MPI_Wtime(), t2-t1);
	
	///printf("[%d]. LOCAL LOAD INFO: CPU_UTI: %Lf, RP: %d, load avg: %.3f\n", rank, *per, *rp, *load_avg);
}

void *worker_status(void *arg)
{	
	
	struct worker_load_task * w_l_t = (struct worker_load_task *)(arg);
		
	//struct worker_task * w_tasks = (struct worker_task *)(arg);
	//Get the thread number (Thread/Process ID)
	#ifdef SYS_gettid
	pid_t tid = syscall(SYS_gettid);
	#else
	#error "SYS_gettid unavailable on this system"
	#endif
	
	w_l_t->status_tid = tid;
	
	//printf("[%d]: Worker Status: %u\n", rank, tid);
	
	struct worker_load * local_load = (struct worker_load *)malloc(sizeof(struct worker_load));	
	
	float load_avg = getLoad(), prev_load_avg;
	
	//systemCall("cat /proc/cpuinfo | grep processor | wc -l",res);
	//int w_cores = atoi(res);	
	
	int w_cores = getNumberOfCores();
	
	float cpu_freq = getCoresFreq();
		
	local_load->w_rank = rank;
	strcpy(local_load->w_name, processor_name);
	local_load->w_load_no = 0;	
	local_load->current_tasks = 0;
	local_load->total_task = 0;
	local_load->updated = 0;
	local_load->status = 0;//0:free, 1:working, 2: Requsting, 3: waiting
	local_load->w_cores = w_cores;		//Static metric
	local_load->w_cache_size = 0;	//Static metric
	local_load->w_cpu_speed = cpu_freq;	//Static metric
	local_load->w_load_avg_1 = load_avg;
	local_load->w_load_avg_2 = load_avg;
	local_load->locked = 0;
	local_load->estimated_load = getLoadValue2(load_avg, load_avg);	
	
	local_load->w_cpu_uti_2 = 0;
	//
	//printf("[%d]: Main thread: %u\n", rank, w_l_t->worker_tid);
	//printf("[%d]: Status thread: %u\n", rank, w_l_t->status_tid);
	//printf("[%d]: Estimator thread: %u\n", rank, w_l_t->estimator_tid);
	int state_worker = getProcessState(w_l_t->worker_tid, 0, 0);
	int state_estimator = getProcessState(w_l_t->worker_tid, 1, w_l_t->estimator_tid);
	//printf("[%d]- %d - %d\n", rank, state_worker, state_estimator);
	//
	local_load->w_running_procs = getRunningProc();
	
	local_load->w_running_procs = local_load->w_running_procs - state_worker - state_estimator - 1;
	if(local_load->w_running_procs < 0)local_load->w_running_procs = 0;

	long double * a = (long double *)malloc(sizeof(long double) * 5);
	long double * b = (long double *)malloc(sizeof(long double) * 5);
	/*
	getCPUValues(a);
	//printf("%Lf %Lf %Lf %Lf %Lf\n",a[0],a[1],a[2],a[3],a[4]);
	int milisec = 10; // length of time to sleep, in miliseconds
	struct timespec req = {0};
	req.tv_sec = 0;
	req.tv_nsec = milisec * 1000000L;
	nanosleep(&req, (struct timespec *)NULL);
*/
	getCPUValues(b);
	//printf("%Lf %Lf %Lf %Lf %Lf\n",b[0],b[1],b[2],b[3],b[4]);
	long double per = 0;
	int rp;
	
	//per = calculateCPUUti(a, b);
	//printf("CPU UTI: %3.2Lf\n", per);
	
	local_load->w_cpu_uti_1 = per;
	local_load->w_cpu_uti_2 = per;
	
	w_l_t->w_local_loads = *local_load;
	
	//printf("[r: %d, w_cores: %d, load_avg: %5.2f, rp: %d, UTI: %3.2Lf]\n", rank, w_cores, load_avg, local_load->w_running_procs, local_load->w_cpu_uti_1);

	//printWorkerLoad(w_l_t->w_local_loads);
	
	sendInitialWorkerLoad(local_load, 0);
	
	prev_load_avg = load_avg;
	
	//recordLocalLoad(w_tasks, load_avg, local_load->estimated_load, per, local_load->w_running_procs);
	
	recordLocalR(w_tasks, cpu_freq, w_cores, per, local_load->w_running_procs);
	
	//return;
	
	int load_info_size = sizeof(int) + sizeof(int) + sizeof(float) + sizeof(long double);		

	int int_size = sizeof(int);
	int float_size = sizeof(float);
	
	int est_load = -1;
	
	void * d = (void*)malloc(load_info_size);
	struct worker_task * p;
	int cur_tasks=0;
	
	//Load values for storing the current load behaviour
	//The current load
	long double per_1 = per;
	//The previous load
	long double per_2 = per;
	//The tow previous load
	long double per_3 = per;
	
	//Actual Relative Power for the previous times
	
	//double start_worker_status = MPI_Wtime();
	float POWER_BASE = cpu_freq / w_cores;	
	float POWER_LIMIT = 95.0;
	float cur_arp = -1;
	float arp_1 = -1;
	float arp_2 = -1;

	while(1){
		
		
		//printf("[%d]. TIME-1: %.6f (%.6f , %.6f)\n", rank,MPI_Wtime(), start_worker_status, (MPI_Wtime() - start_worker_status));
		//i_status++;
		//sleep(1);
		//usleep(500);
		
		prev_load_avg = load_avg;
		
		//Obtaining the local load
		setLocalLoadInfo(a, b, &per, &rp, &load_avg);
		
		///double t1 = MPI_Wtime();
		
		///printf("[%d]. TIME-2: %.6f\n", rank, t1);
		/*
		if(prev_load_avg != load_avg){
			est_load = getLoadValue2(prev_load_avg, load_avg);		
		}*/
		//printf("[%d]- %d - %d : %d\n", rank, state_worker, state_estimator, rp);
		
		//printf("[%d]- Prev-Lavg: %f - Current-Lavg: %f\n", rank, prev_load_avg, load_avg);
		
		///double t22 = MPI_Wtime();
		
		if(w_l_t){
			w_l_t->w_local_loads.w_load_avg_1 = prev_load_avg;
			w_l_t->w_local_loads.w_load_avg_2 = load_avg;
			w_l_t->w_local_loads.estimated_load = est_load;
			w_l_t->w_local_loads.w_running_procs = rp;
			w_l_t->w_local_loads.w_cpu_uti_1 = w_l_t->w_loads[rank-1].w_cpu_uti_2;
			w_l_t->w_local_loads.w_cpu_uti_2 = per;
		}
		
		//printf("[%d] print the loal load report at the load thread...\n",rank);
		//printWorkerLoad(w_l_t->w_local_loads);
		
		//
		per_3 = per_2;		
		per_2 = per_1;
		per_1 = per;
		///printf("[%d] Load Info: 1: %Lf, 2: %Lf, 3: %Lf\n", rank, per_1, per_2, per_3);
		
		/*
		arp_3 = arp_2;
		arp_2 = arp_1;
		arp_1 = getActualRelativePower(cpu_freq, per, local_load->estimated_load, local_load->w_running_procs, w_cores, 0);
		printf("[%d] Processing Power Info: 1: %f, 2: %f, %f\n", rank, arp_1, arp_2, arp_3);
		*/
		
		///printf("[%d]. TIME-22: %.6f\n", rank, MPI_Wtime() - t22);
		
		//
		
		//printWorkerLoadReport(w_l_t->w_loads, numprocs);
		
		//if(i_status % 2 == 0)
		{
			
			
			
			*((int*)d) = rp;
			*((int*)(d + int_size)) = est_load;
			*((float*)(d + 2 * int_size)) = load_avg;
			*((long double*)(d + 2 * int_size + float_size)) = per;		
			
			//if(rank == 1)
				//printf("[%d]. prev_load_avg: %5.2f, load_avg: %5.2f{est_load: %d} rp: %d, UTI: %3.2Lf]\n", rank, prev_load_avg, load_avg, est_load, rp, per);		
			//printf("[r: %d, load_avg: %5.2f, rp: %d, UTI: %3.2Lf]\n\n", rank, *((float*)(d + int_size)), *((int*)d), *((long double*)(d + int_size + float_size)));	

			//sendWorkerLoad(d, load_info_size, 0);
			
			//if(prev_load_avg == load_avg)
				
			//double load_overhead_record = MPI_Wtime();
			
			//recordLocalLoad(w_tasks, load_avg, est_load, per, rp);	
			
			recordLocalR(w_tasks, cpu_freq, w_cores, per, rp);
			
			//printf("[%d]. Load Overhead Record: %.6f \n", rank, (MPI_Wtime() - load_overhead_record));
			
			///double t44 = MPI_Wtime();	
			
			
			//Get the cuurent running tasks		
				
			p = w_l_t->w_tasks->next;
			cur_tasks = 0;
			///printf("--------------------------------------------------------\n[%d]: [", rank);		
			while(p != NULL){
				//printf("******************************************[%d], t_id: %d\n", rank, p->m_task->m_task_id);			
				if(!p->m_task->done && (p->move_status != 1)){
					//printf("%d, ", p->m_task->m_task_id);
					cur_tasks++;
				}
				p = p->next;			
			}		
			///printf("] = %d tasks\n", cur_tasks);
			///printf("--------------------------------------------------------\n");
			
			
			//trigger to obtain the latest load info from the master.
			
			///printf("\n[%d]. TIME-2234: %.6f\n", rank, MPI_Wtime() - t44);
			
			///double t33 = MPI_Wtime();
			
			if(cur_tasks > 0){
				//printf("*[%d]. UTI: %3.2Lf, current tasks: %d, time: %.3f]\n", rank, per, cur_tasks, MPI_Wtime() - start_worker_status);
				//obtainLatestLoad(0);
				
				//double load_overhead_findR = MPI_Wtime();
				
				cur_arp = getActualRelativePower(cpu_freq, per, est_load, rp, w_cores, 0, rank);
				
				//printf("[%d]. Load Overhead FindR: %.6f \n", rank, (MPI_Wtime() - load_overhead_findR));
				
				if((cur_arp * 100 / POWER_BASE) < POWER_LIMIT){
					
					if(arp_1 == -1){
						arp_1 = cur_arp;
					}else{
						if(arp_2 == -1){
							arp_2 = cur_arp;
						}else{
							printf("\n[%d] ---//// TRIGGER \\\\---\n", rank);
							//TRIGGER
							obtainLatestLoad(0);
							//
							arp_1 = -1;
							arp_2 = -1;
						}
					}
					/*
					if(arp_2 != -1){
						printf("\n[%d] ---//// TRIGGER \\\\---\n", rank);
						//TRIGGER
						obtainLatestLoad(0);
						//
						arp_1 = -1;
						arp_2 = -1;
					}else{
						if(arp_1 == -1){
							arp_1 = cur_arp;
						}else{
							if(cur_arp <= arp_1){
								printf("\n[%d] ---//// TRIGGER \\\\---\n", rank);
								//TRIGGER
								obtainLatestLoad(0);
								//
								arp_1 = -1;
								arp_2 = -1;
							}else{
								arp_2 = cur_arp;
							}
						}
					}*/
				}else{
					arp_1 = -1;
					arp_2 = -1;
				}
				
				///printf("[%d] Actual Relative Power: Cur: %.2f[%.2f%%], 1st: %.2f, 2nd: %.2f\n", rank, cur_arp, (cur_arp * 100 / POWER_BASE), arp_1, arp_2);
			}			
			
			///printf("[%d]. TIME-223: %.6f\n", rank, MPI_Wtime() - t33);
			
		}
		
		
		
		///double t2 = MPI_Wtime();
		
		///printf("[%d]. TIME-111: %.6f (%.6f)\n", rank, t2, t2-t1);	
		
	}	
}

///Send get initial load request
void notifyMaster(int t_id, int target_w, int master){
	int send_code = MOBILITY_NOTIFICATION_FROM_WORKER;	
	int * data = (int*)malloc(sizeof(int)*2);
	*data = t_id;
	*(data + 1) = target_w;
	printf("[%d]. send move notification to master (t: %d, w: %d)\n", rank, t_id, target_w);
	MPI_Send(&send_code, 1, MPI_INT, master, send_code, MPI_COMM_WORLD);	
	MPI_Send(data, 2, MPI_INT, master, send_code, MPI_COMM_WORLD);
}

void * move_mobile_task(void * arg){
	
	//int send_code = 10;	

	struct worker_task * w_t = (struct worker_task *)arg;
	
	printf("[%d] Thread is starting to move the task %d to %d - (TIME: %f)\n", rank, w_t->task_id, w_t->go_to, MPI_Wtime());
	
	while(w_t->go_move == 0) usleep(1);						
						
	printf("[%d] wT->task_id %d is ready to move... - (TIME: %f) \n", rank, w_t->task_id, MPI_Wtime());
	
	w_t->m_task->m_end_time[w_t->m_task->moves-1] = MPI_Wtime();

	pthread_mutex_lock( &mutex_w_sending );
	
	notifyMaster(w_t->task_id, w_t->go_to, 0);
	
	sending_task = 1;

	
	
	printf("[%d]. Sending Mobile Task.R: %.2f\n", rank, (w_t->m_task->move_stats.R_2));
	sendMobileTask(w_t->m_task, w_t->go_to, W_TO_W);
	
	sending_task = 0;

	pthread_mutex_unlock( &mutex_w_sending );
	
	printf("[%d]Finish sending the mobile task %d to  %d...(TIME: %f)\n", rank, w_t->task_id, w_t->go_to, MPI_Wtime());
	//sleep(10);

	return NULL;
}

///Send get initial load request
void sendInitialWorkerLoadRequest(int proc){
	int send_code = LOAD_REQUEST_FROM_MASTER;		
	MPI_Ssend(&send_code, 1, MPI_INT, proc, send_code, MPI_COMM_WORLD);	
}

//type: 0 -> send task
//type: 1 -> recv task
void modifyWorkerLoadReportM(struct worker_load * report, int source, int target){
	report[source-1].current_tasks--;
	//report[source-1].w_running_procs++;	
	report[source-1].w_load_no++;
	report[source-1].status = 1;
	if(report[source-1].current_tasks == 0)
		report[source-1].status = 0;
	report[target-1].current_tasks++;
	//report[target-1].w_running_procs++;
	report[target-1].w_load_no++;
	report[target-1].status = 1;
}

//type: 0 -> send task
//type: 1 -> recv task
void modifyWorkerLoadReport(int w, struct worker_load * report, int n, int type){
	if(type == 0){
		report[w-1].current_tasks++;
		report[w-1].status = 1;
	}else if(type == 1){
		report[w-1].current_tasks--;
		report[w-1].total_task++;
		if(report[w-1].current_tasks == 0)
			report[w-1].status = 0;
	}
}

//send ping message to the selected worker
void getInitNetworkLatency(struct worker_load * w_report){
	//printf("[%d]. sendPing: w: %d, name: %s\n", rank, w_report->w_rank, w_report->w_name);	
	
	double ping_value = sendPing(w_report->w_name);
	if(ping_value != 0.0)
		w_report->net_times.init_net_time = ping_value;

}

void getInitialWorkerLoad(struct worker_load * report, int n){
	
	int i=0;
	//int sender = -1;				
	//int workerMsgType = -1;
	//printf("----[%d]Master sent somethng to Ws, code: 1 (T: %f)...\n", rank, MPI_Wtime());
	//double init_net_lat = MPI_Wtime();
	for( i=1; i<n; i++)	{
		sendInitialWorkerLoadRequest(i);
	}
	//init_net_lat = MPI_Wtime() - init_net_lat;
	//printf("[%d]. Init net lat: %f\n", rank, init_net_lat/(n-1));	
	
	int msg_code = -1;
	int w = 0;
	for(i=1; i<n; i++){		
		MPI_Recv(&msg_code, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);		
		//printf("----[%d]Master recv somethng from W: %d, code: %d (T: %f)...\n", rank, i, msg_code, MPI_Wtime());
		w = status.MPI_SOURCE;
		if(msg_code == 1){
			MPI_Recv(&report[w-1], sizeof(struct worker_load), MPI_CHAR, w, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			getInitNetworkLatency(&report[w-1]);
		}
	}	
	
	//report->init_net_time = init_net_lat/(n-1);
	
	printWorkerLoadReport(report, n);
}

void setWorkerLoad(struct worker_load * report, int n, int source, int tag){
	//int i=0;
	int load_info_size = 0;
	MPI_Recv(&load_info_size, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
	void * load_info = (void *)malloc(load_info_size);
	MPI_Recv(load_info, load_info_size, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);

	report[source-1].w_load_no++;
	report[source-1].w_load_avg_1 = report[source-1].w_load_avg_2;
	report[source-1].w_load_avg_2 = *((float *)(load_info + 2 * sizeof(int)));
	
	report[source-1].estimated_load = *((int *)(load_info + sizeof(int)));

	report[source-1].w_cpu_uti_1 = report[source-1].w_cpu_uti_2;
	report[source-1].w_cpu_uti_2 = *((long double *)(load_info + 2 * sizeof(int) + sizeof(float)));
	
	report[source-1].w_running_procs = *((int *)load_info);
	
}
/*
void sendWorkerLoadReport(struct worker_load * report, int n){
	int i=0;
	int send_code = 3;				
	for( i=1; i<n; i++){
		MPI_Send(&send_code, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
		MPI_Send(report, sizeof(struct worker_load)*(n-1), MPI_CHAR, i, send_code, MPI_COMM_WORLD);
	}
}
*/

void updatenetworkLatency(struct worker_load * report, double new_lat){
	/*
	if(report->cur_net_time > 0)
		report->cur_net_time = (report->cur_net_time + new_lat)/2;
	else 
		report->cur_net_time = new_lat;
	printf("[%d]. NEW NET LAT: %f (new_net_lat: %f)\n", rank, report->cur_net_time, new_lat);
	* */
	//
}

void circulateWorkerLoadReportInitial(struct worker_load * report, int n){
	int i = 1;
	int send_code = UPDATE_LOAD_REPORT_REQUEST;		
	int report_size = sizeof(struct worker_load)*(n-1);		
	
	//check if the master is receiving a msg from i worker
	
	
	
	while(*(masterReceiving + (i) - 1) == 1) usleep(1);
	//luck sending recv confirmation before ending sending the laod resport
	*(masterSending + i - 1) = 1;

	//printf("----[%d]Master circulate the load to to w: %d, code: %d (T: %f)...\n", rank, i, send_code, MPI_Wtime());

	//
	
	//printf("[%d]. LOAD_AGENT: %.6f\n", rank, MPI_Wtime());
	
	load_agent_t1 = MPI_Wtime();
	//double load_overhead_circulate = load_agent_t1;
	//
	double new_net_lat = MPI_Wtime();	
	MPI_Ssend(&send_code, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
	new_net_lat = MPI_Wtime() - new_net_lat;	
	MPI_Send(&report_size, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
	//Update the network latency
	//updatenetworkLatency(report, new_net_lat);
	//
	MPI_Send(report, report_size, MPI_CHAR, i, send_code, MPI_COMM_WORLD);
	
	//unlock sending recv confirmation before ending sending the laod resport
	*(masterSending + i - 1) = 0;
	
	//printf("[%d]. Load Overhead CIRCULATE: %.6f \n", rank, (MPI_Wtime() - load_overhead_circulate));
}


void sendLatestLoad(struct worker_load * report, int n, int worker){
	int i = worker;
	int send_code = LOAD_INFO_FROM_MASTER;		
	int report_size = sizeof(struct worker_load)*(n-1);	
	//
	//report->cur_net_time = MPI_Wtime();
	//printf("[%d]. report->cur_net_time: %f\n", rank, report->cur_net_time);
	//	
	
	//check if the master is receiving a msg from i worker
	while(*(masterReceiving + (i) - 1) == 1) usleep(1);
	//luck sending recv confirmation before ending sending the laod resport
	*(masterSending + i - 1) = 1;
	
	//printf("----[%d]. Master sent somethng to w: %d, code: %d (T: %f)...\n", rank, worker, send_code, MPI_Wtime());

	MPI_Send(&send_code, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
	MPI_Send(&report_size, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
	MPI_Send(report, report_size, MPI_CHAR, i, send_code, MPI_COMM_WORLD);
	
	//unlock sending recv confirmation before ending sending the laod resport
	*(masterSending + i - 1) = 0;
}

void circulateWorkerLoadReport(struct worker_load * report, int to_worker, int n){
	int i = to_worker;
	int send_code = UPDATE_LOAD_REPORT_REQUEST;		
	int report_size = sizeof(struct worker_load)*(n-1);	
	
	//printf("[%d]. circulateWorkerLoadReport to %d- report_size: %d\n", rank, i, report_size);
	
	//while(workerSending == 0) usleep(1);
	//workerSending = 0;
	
	pthread_mutex_lock( &mutex1 );
	
	//printf("[%d]. circulateWorkerLoadReport to %d- report_size: %d\n", rank, i, report_size);

	//double new_net_lat = MPI_Wtime();
	MPI_Send(&send_code, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
	//new_net_lat = MPI_Wtime() - new_net_lat;	
	MPI_Send(&report_size, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
	//Update the network latency
	//updatenetworkLatency(report, new_net_lat);
	
	//printf("[%d]. circulateWorkerLoadReport to %d- report_size: %d\n", rank, i, report_size);
	
	MPI_Send(report, report_size, MPI_CHAR, i, send_code, MPI_COMM_WORLD);
	
	pthread_mutex_unlock( &mutex1 );
	
	//workerSending = 1;
}

void recvWorkerLoadReport(struct worker_load * report, int source, int tag){
	int report_size = 0;
	MPI_Recv(&report_size, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
	MPI_Recv(report, report_size, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
	
	//printWorkerLoadReport(report, numprocs);
}

int selectWorkerToCheckLatency(struct worker_load * report, int n){
	int w = -1;
	int i;
	for(i = 0; i < n; i++){	
		printf("[%d]. selectWorkerToCheckLatency(i: %d of %d, w: %d)\n", rank, i, n, w);
		//check if the status of the worker is not mobility
		if(report[i].status != 4){
			w = report[i].w_rank;
			break;
		}
	}
	return w;
}

void* networkLatencyFun(void * arg){	
	struct worker_load * w_load_report = (struct worker_load *)arg;
	
	double new_net_lat = 0;

	int w = -1;

	while(1){
		/*		
		w = selectWorkerToCheckLatency(w_load_report, numprocs - 1);
		printf("[%d]. selected worker: %d\n", rank, w);
		if(w > 0){
			//new_net_lat_2 = MPI_Wtime();
			//w_load_report->cur_net_time = MPI_Wtime();
			
			//new_net_lat_2 = MPI_Wtime() - new_net_lat_2;
			//printf("[%d]. NEW NET LAT L1: %.6f\n", rank, new_net_lat_2);
			
			//updatenetworkLatency(w_load_report, new_net_lat_2);
			
		}*/
		
		for(w=0;w<numprocs-1;w++){
			new_net_lat = sendPing(w_load_report[w].w_name);
			w_load_report[w].net_times.cur_net_time = new_net_lat;
		}
		
		printWorkerLoadReport(w_load_report, numprocs);
		
		sleep(3);		
	}	
}


void* workerLoadReportFun(void * arg){
	struct worker_load * w_load_report = (struct worker_load *)arg;
	
	int i =0;
	
	while(1){
		sleep(1);
		//usleep(500);
		//printWorkerLoadReport(w_load_report, numprocs);		
		//sendWorkerLoadReport(w_load_report, numprocs);		
		double circ_start = MPI_Wtime();
		circulateWorkerLoadReportInitial(w_load_report, numprocs);		
		
		if( i++ == 3)
			sendLatestLoad(w_load_report, numprocs, 1);
			
		double circ_end = MPI_Wtime();
		printf("[%d]. circ_time: %lf\n", rank, circ_end - circ_start);
		//return;
	}
}

///send confirmation to worker for accepting receiving the result from the worker
void sendRecvConfirmation(int w){
	int send_code = SENDING_CONFIRMATION_FROM_MASTER;
	MPI_Send(&send_code, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
}

//void sendMobileConfirmationToWorker(struct worker_load w_l_l, int w, int num_tasks, int load_no){
void sendMobileConfirmationToWorker(int w, int permitted_tasks){
	int send_code = MOBILITY_ACCEPTANCE_FROM_WORKER;
		
	//Send confirmation to the source worker	
	MPI_Send(&send_code, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);			
	MPI_Send(&permitted_tasks, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
}

void sendMobileConfirmation(int t_id, int source, int master){
	int send_code = MOBILITY_CONFIRMATION_FROM_WORKER;
	//Send confirmation to the master
	MPI_Send(&send_code, 1, MPI_INT, master, send_code, MPI_COMM_WORLD);
	int move_confirmation[2];
	move_confirmation[0] = source;
	move_confirmation[1] = t_id;
	MPI_Send(move_confirmation, 2, MPI_INT, master, send_code, MPI_COMM_WORLD);

	printf("[%d]. Send Confirmation to the source worker: %d...\n", rank, source);
	//Send Confirmation to the source worker
	MPI_Send(&send_code, 1, MPI_INT, source, send_code, MPI_COMM_WORLD);
	MPI_Send(&t_id, 1, MPI_INT, source, send_code, MPI_COMM_WORLD);
}

void recvMobileConfirmationM(struct worker_load * report, struct task_pool * pool, int w, int msg_code){
	int move_confirmation[2];
	MPI_Recv(move_confirmation, 2, MPI_INT, w, msg_code, MPI_COMM_WORLD, &status);

	//Update the worker report.
	modifyWorkerLoadReportM(report, move_confirmation[0], w);
	
	//Update the task pool report
	updateMobileTaskReport(pool, move_confirmation[1], 2, move_confirmation[0]);
	
	printf("**-MASTER-**: Move Confirmation for task %d from %d to %d\n", move_confirmation[1], move_confirmation[0], w);
}


///*********************************************************************************************
///*********************************************************************************************
///*********************************************************************************************
///*********************************************************************************************


char* getProcessName(pid_t pid)
{
    FILE *fp;
    char filename[30];
    char line[100];
    char key[20];
    char *processName = (char*)malloc(sizeof(char) * 50);
    sprintf(filename, "/proc/%d/status", pid);
    fp = fopen(filename, "r");
    if(fp == NULL)
        return NULL;
    if(fgets(line, 100, fp) != NULL) {
		sscanf(line, "%s %s\n", key,processName);
        if(strcmp(key,"Name:")==0)
		{	
			return processName;
		}
    }
    fclose(fp);
    return NULL;
}


int substring(char *s1,char *s2)
{
	int f=0;
	for(;*s1!='\0';){
		if(*s2=='\0')
			break;
		for(;*s2!='\0';){
			if(*s1==*s2){
				f=1;
				s1++;
				s2++;
			}
			else{
				f=0;
				s1++;
				break;
			}
		}
   }
   if(f==0)
        return 0;
    else
        return 1;
}

void systemCallNoRes(b)
char* b;
{
  //Execute System call
  FILE *fp;
  fp = popen(b, "r");   
  pclose(fp);
}
                
int readFile(inputFile,outputData)
char* inputFile;
char* outputData;
{  
	FILE* fp;
	inputFile[strlen(inputFile)-1]='\0';
	printf("Input file, Length [ %ld ] : %s\n",strlen(inputFile),inputFile);
	if((fp=fopen(inputFile,"r")) == NULL) {
	printf("Cannot open file.\n");
	return 0;
	}
	int n = fread( outputData, sizeof( char ), 1000, fp );  

	outputData[n]='\0'; 
	return 1; 
}

void printTime(void){
	struct tm *local;
	//struct timespec *tms;
	time_t t;
	t = time(NULL);
	local = localtime(&t);
	struct timeb tp;
	ftime(&tp);
	if(tp.millitm > 99)
		printf("the current time is - %d:%d:%d.%d\n", local->tm_hour, local->tm_min, local->tm_sec,tp.millitm);
	else if(tp.millitm > 9)
		printf("the current time is - %d:%d:%d.0%d\n", local->tm_hour, local->tm_min, local->tm_sec,tp.millitm);
	else if(tp.millitm > 0)
		printf("the current time is - %d:%d:%d.00%d\n", local->tm_hour, local->tm_min, local->tm_sec,tp.millitm);
}

int getTimeMil(void){
	struct tm *local;
	//struct timespec *tms;
	time_t t;
	t = time(NULL);
	local = localtime(&t);
	struct timeb tp;
	ftime(&tp);
	return tp.millitm + (local->tm_sec * 1000) + (local->tm_min * 1000 * 60) + (local->tm_hour * 1000 * 60 * 60);
}

void printDuration(int d){
	int h,m,s,mil;
	h = d / (1000 * 60 * 60) ;
	m = d / (1000 * 60) - (h * 60 * 60);
	s = d / 1000 - (m * 60);
	mil = d % 1000;
	if(mil > 99)
		printf("the duration is - %d:%d:%d.%d\n", h, m, s,mil);
	else if(mil > 9)
		printf("the duration is - %d:%d:%d.0%d\n", h, m, s,mil);
	else if(mil >= 0)
		printf("the duration is - %d:%d:%d.00%d\n", h, m, s,mil);  
}

void printDuration2(int d){
	int h,m,s,mil;
	h = d / (1000 * 60 * 60) ;
	m = d / (1000 * 60) - (h * 60 * 60);
	s = d / 1000 - (m * 60);
	mil = d % 1000;

	if(mil > 99)
		printf("%d, %d:%d:%d.%d\n", d, h, m, s,mil);
	else if(mil > 9)
		printf("%d, %d:%d:%d.0%d\n", d, h, m, s,mil);
	else if(mil >= 0)
		printf("%d, %d:%d:%d.00%d\n", d, h, m, s,mil);  
}

int validCommand(char *b){
	char validRes[130000];   
	systemCall(b,validRes);  
	if(strcmp(validRes , "")==0)	
		return 0;   
	else
		return 1;   
}

void writeTaskInfo(int rank, double sec, int task, int a, int b, int c, int d, int moving){
	char* task_file = (char*)malloc(sizeof(char)*45);
	sprintf(task_file,"task_info/task_info_%d.csv", task);
	FILE* f = fopen(task_file,"a+");	
	fprintf(f, "rank: %d, sec: %.2f, moving: %d,  params: %d\t%d\t%d\t%d\n", rank, sec, moving, a, b, c, d);
	fclose(f);
}

float getloadPer(int load_val){
	return 0.39668*load_val*load_val + 7.3936*load_val + 10.843;
}

float getEstimated(float cur_est_time, int cur_load, int new_load, int cores){
	if(cur_load == new_load) return cur_est_time;
	float cur_load_per = 1.0;
	if((cur_load - cores) > 0){
		cur_load_per = getloadPer(cur_load - cores);
		cur_load_per = (cur_load_per / 100) + 1;
	}
	
	float new_load_per = 1.0;
	if((new_load - cores) > 0){
		new_load_per = getloadPer(new_load - cores);
		new_load_per = (new_load_per / 100) + 1;
	}
	
	printf("cur_load: %d, cur_est_time: %.3f, new_load: %d, new_val: %.3f\n", cur_load, cur_est_time, new_load, (new_load_per * cur_est_time) / cur_load_per);
	
	return (new_load_per * cur_est_time) / cur_load_per;	
}

void estimateCurrentTime(){
	float elapsed = (MPI_Wtime() - startTaskTime);
	if(mainIndex != NULL){
		float per = (*mainIndex)*100.0/totalIndex;	
		float prev_time = 0;
		float prev_per = 0;
		struct workerLoad * p;
		p = w1Load;
		while(p->next != NULL)
			p = p->next;
		float cur_load = p->cur_load;
		p->per = per;
		p->sec = elapsed;
		
		float total_est = 0;
		
		p = w1Load;
		while(p != NULL){			
			float new_est = getEstimated((100*(p->sec - prev_time))/(p->per - prev_per), p->cur_load, cur_load, currentCores);
			printf("Load(%d, %.2f, %.2f, %d, Estimation: %.3f, new_est: %.2f)\n", p->cur_load, p->per, p->sec, currentCores, (100*(p->sec - prev_time))/(p->per - prev_per), new_est);
			
			total_est += new_est * (((p->sec - prev_time) * 100 / elapsed) / 100);
			
			prev_time = p->sec;
			prev_per = p->per;
			p = p->next;	
		}	
		printf("*-*--- Elapsed Time: %.3f, Per: %.2f, Estimated Time: %.2f --*-*\n", elapsed, per, total_est);
	}
}


///Send the task to a ready worker(Master -> Worker)
void sendCommand(int input,int countData,int proc){
	int sendType = 0;
	MPI_Send(&sendType, 1, MPI_INT, proc, sendType, MPI_COMM_WORLD);
}


void recvMsgType(int *recvProc,int *type){
	//MPI_Status *status = (MPI_Status *)malloc(sizeof(MPI_Status));
	int msgType = -1;
	//printf("[%d]M----1-------1\n",rank);
	MPI_Recv(&msgType,1,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);	
	//printf("[%d]M----1-------2\n",rank);
	*recvProc = status.MPI_SOURCE;
	*type = status.MPI_TAG;
}

///Receive the report by master(Worker -> Master)
void recvReport(void *msg,int source,int sendType){
	MPI_Status status;
	MPI_Recv(msg,3,MPI_DOUBLE,source,sendType,MPI_COMM_WORLD,&status);
	//printf("Load received: %f\n", *((double *)msg));
}

///Receive the report by master(Worker -> Master)
void recvReport1(void *msg,int source,int sendType){
	MPI_Status status;
	MPI_Recv(msg,1,MPI_DOUBLE,source,sendType,MPI_COMM_WORLD,&status);
}

void recvMoveRequest(int *dest_worker, int source,int sendType){
	MPI_Status status;
	MPI_Recv(dest_worker,1,MPI_INT,source,sendType,MPI_COMM_WORLD,&status);
}

void recvMoveDone(int *moved_task, int *source_worker, int source,int sendType){
	MPI_Status status;
	MPI_Recv(source_worker,1,MPI_INT,source,sendType,MPI_COMM_WORLD,&status);
	MPI_Recv(moved_task,1,MPI_INT,source,sendType,MPI_COMM_WORLD,&status);
}


void setWorkersMovingDone(struct workerReport * report,int n,int sender,int source, int moved_task){
	int i=0;
	for(i=0;i<n-1;i++)
	{	
		if(report[i].proc_rank == sender){
			report[i].current_tasks++;
			report[i].status = 1;
			report[i].busy = 1;
			report[i].task = moved_task - 1;
			
		}
		if(report[i].proc_rank == source){
			report[i].current_tasks--;
			if(report[i].current_tasks > 0){
				report[i].status = 1;
				report[i].busy = 1;
			}else{
				report[i].status = 0;
				report[i].busy = 0;
			}
			report[i].task = -1;
		}
	}
}

void addTaskMobility(struct taskReport * rep,int tNum, int dest, int curTime){	
	rep[tNum-2].destMobile[rep[tNum-2].mobiles] = dest;
	rep[tNum-2].timeMobile[rep[tNum-2].mobiles] = curTime;
	//printf("start: %d, curTime: %d\n",rep[tNum-2].start, curTime);
	rep[tNum-2].mobiles++;
	//printf("Add Mobility: %d\n",tNum);
}

void checkAvailability(struct workerReport * report,int n,int dest, int *confirmation){
	int i=0;
	for(i=0;i<n-1;i++)
	{	
		if(report[i].proc_rank == dest){
			if(report[i].status == 3)
				*confirmation = 0;
			//else if(report[i].status == 1)
			//	*confirmation = 1;			
			else if(report[i].status == 0)
				*confirmation = 1;			
			break;
		}
	}
}

void setWorkersMovingStatus(struct workerReport * report,int n,int sender,int dest){
	int i=0;
	for(i=0;i<n-1;i++)
	{	
		if(report[i].proc_rank == sender){
			report[i].status = 2;
		}
		if(report[i].proc_rank == dest){
				report[i].status = 3;
		}
	}
}

void viewLoadReport(struct workerReport * report,int n){
	int i=0;
	float val=0;
	printf("\nLOAD NO: %d\n",report[0].load_code);
	for(i=0;i<n-1;i++){
		///if(report[i].busy == 1){
			if(report[i].proc_load_old != 0){
				if(report[i].proc_load >= 1)
					val = (report[i].proc_load/report[i].proc_load_old) * report[i].proc_load*8;
			}else
				val = 0;
				//r = (load / 8) * (uti / 8)
				float r = (((report[i].proc_load + report[i].proc_load_old)/2) / report[i].cores) * (report[i].cpuUti / report[i].cores);
			//printf("worker [%d] has load : %.3f, old Load: %.3f, load_code: %d, busy, taskID : %d, total : %d\n",report[i].proc_rank,report[i].proc_load,report[i].proc_load_old,report[i].load_code,report[i].task,report[i].totalTask);			
			printf("[%d]worker [%d] load : %.3f,\t old: %.3f,\t Percentage: %.3f, N: %.3f,\tTASK:%d, cores: %d, CPU: %0.2f, r: %0.2f\n",rank,report[i].proc_rank,report[i].proc_load,report[i].proc_load_old,report[i].proc_load/report[i].proc_load_old,val,report[i].task, report[i].cores, report[i].cpuUti,r);
		///}
			//printf("worker [%d] has load : %.3f , busy, taskID : %d, total : %d\n",report[i].proc_rank,report[i].proc_load,report[i].task,report[i].totalTask);			
		//else
		//	printf("worker [%d] has load : %.3f , free, total : %d\n",report[i].proc_rank,report[i].proc_load,report[i].totalTask);
	}
}

///Send Load Report to all workers
void sendLoadReport(struct workerReport *input,int workers){
	int sendType = 5;	
	int i=0;
	///printf("---------------------------\n");
	for(i=0;i<workers;i++){
		//printf("Send Report to %d is avaialble : %d\n", (i+1), *(masterReceiving + (i+1) - 1));
		while(*(masterReceiving + (i+1) - 1) == 1) usleep(2);
		
		MPI_Send(&sendType, 1, MPI_INT, i+1, sendType, MPI_COMM_WORLD);
		MPI_Send((char *)input, (sizeof (struct workerReport))*workers, MPI_CHAR, i+1, sendType, MPI_COMM_WORLD);
	}	
	///printf("Load Report succefullty sent\n");
}

void setNewLoad(struct workerReport * report,int n,int sender,float* load){
	int i=0;
	for(i=0;i<n-1;i++)
	{	
		if(report[i].proc_rank == sender){
			report[i].proc_load_old = report[i].proc_load;
			report[i].proc_load = *load;			
			report[i].updated = 1;
			//report[i].cores = load[1];
			//report[i].cpuUti = load[2];
			//report[i].cpuUti = load[2]
			report[i].load_code++;
			break;
		}
	}	
	//int load_no = report[0].loadNo;
	//printf("load From (%d) : report[i].loadNo: %d- load-val: %lf\n", sender, report[sender - 1].loadNo, *load);
	//load_count++;
	//if((load_count % ((numprocs -1) * 2)) != 0)
	//	return;
	//for(i=0;i<n-1;i++)
	//	if(report[i].loadNo != load_no)
	//		return;
	//viewLoadReport(report, n);
	//Send Load report to All workers
	//Mobile Process
	///-----------Mobility mode ----------
	//sendLoadReport(report, n-1);
	///---------------------------------
}

void getUpdatedLoad(struct workerReport * report,int n){
	int i=0;
	int sender = -1;				
	int workerMsgType = -1;
	for(i=1;i<n;i++)
	{	
		sendCommand(1, 1, i);
	}
	
	double *loadWorker = (double*)malloc(sizeof(double)*3);
	for(i=1;i<n;i++)
	{	
		///recvMaster(loadWorker,1,MPI_DOUBLE,sizeof(double),&lw,&workerMsgType);
		recvMsgType(&sender,&workerMsgType);
		if(workerMsgType == 0)
			recvReport(loadWorker,sender,workerMsgType);
		printf("Load From(%d)(n:%d)\n",sender, n);
		if(report[sender-1].proc_rank != sender){
			report[sender-1].proc_rank = sender;
			report[sender-1].proc_load = loadWorker[0];
			report[sender-1].proc_load_old = report[sender-1].proc_load;
			report[sender-1].load_code = 0;
			report[sender-1].busy = 0;
			report[sender-1].task = -1;
			report[sender-1].totalTask = 0;
			report[sender-1].current_tasks = 0;
			report[sender-1].status = 0;
			report[sender-1].cores = (int)loadWorker[1];
			report[sender-1].cpuUti = (double)loadWorker[2];
		}
	}		
}

void setInitialValues(struct taskReport* rep,int n){
	int i=0;
	for(i=0;i<n;i++){
		rep[i].end = 0;
		rep[i].start = 0;
		rep[i].proc = 0;
		rep[i].procR = 0;
		rep[i].taskNum = 0;
		rep[i].mobiles = 0;
	}	
}

void print2DArrayFrom1D(void * arr, int rows, int cols){
	//printf("RANK: %d\n",rank);
	int ii, jj , k=0;		
	for (ii = 0; ii < rows; ii++) {
		for (jj = 0; jj < cols; jj++) {
			printf("%8.1f",((double *)arr)[k++]);
		}
		printf("\n");
	}
}

int getBestFreeWorker(struct workerReport * report,int n){
	int bestWorker=0;
	int i=0;
	float minLoad;
	for(i=0;i<n;i++)
	{	
		//if(report[i].busy == 0){
			minLoad = report[i].proc_load;
			bestWorker = report[i].proc_rank;
			break;
		//}
	}
	for(;i<n;i++)
	{	
		//printf("i : %d , rank : %d , load : %f \n",i,report[i - 1].proc_rank,report[i - 1].proc_load);
		//if(report[i].busy == 0){		
			if(report[i].proc_load < minLoad){
				minLoad = report[i].proc_load;
				bestWorker = report[i].proc_rank;
			}
		//}
	}
	return bestWorker;
}		

void sortLoadReport(struct workerReport * report,int n){
	int i=0,j;
	struct workerReport temp;
	for(i=0;i<n-1;i++){
		for(j=0;j<(n-2-i);j++)
		{
			if(report[j].proc_load > report[j+1].proc_load)			
			{				
				temp = report[j];
				report[j] = report[j+1];
				report[j+1] = temp;
			}
		}
	}
}

///Send terminator message to the finished worker
void sendTerminator(MPI_Datatype taskType,int proc){	
	int send_code = 2;
	MPI_Send(&send_code, 1, MPI_INT, proc, send_code, MPI_COMM_WORLD);
	//printf("Termination tag is sent to [%d]\n",proc);
}

///Send Terminator message to all processes
void terminateAllWorkers(MPI_Datatype taskType,int procNum){
	//double d = 0;
	int i=0;
	for(i=1;i<procNum;i++)
		sendTerminator(taskType,i);
}

void printTaskReport(struct taskReport * rep,int n){
	int i=0, j=0;
	for(i=0;i<n;i++){
		if(rep[i].end != 0)
		{
			printf("task[%d], time: %d, proc: %d, procR: %d, Mobilies: %d---\n",rep[i].taskNum,(rep[i].end - rep[i].start), rep[i].proc, rep[i].procR, rep[i].mobiles);
			//printf("-1: %d\t %.2f\n", rep[i].proc, (rep[i].end  - rep[i].start)/1000.0);
			for(j=0;j<rep[i].mobiles;j++){
				//printf("Mobile(%d) To: %d, at: %d\n", j, rep[i].destMobile[j], (rep[i].timeMobile[j]  - rep[i].start));
				printf("%d: %d\t %.2f\n", j, rep[i].destMobile[j], (rep[i].timeMobile[j]  - rep[i].start)/1000.0);
			}
			printDuration(rep[i].end - rep[i].start);
			//printf("task [%d] , start : %d , end : %d\n",rep[i].taskNum,rep[i].start,rep[i].end);
		}
	}
}

void sendMultiMsgs(void *input, int dataLen, int limit, int proc, int tag, MPI_Datatype dataType){
//MPI_Send(&dataLen, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
//printf("Send dataLen: %d\n", dataLen);
//MPI_Send(input, dataLen, dataType, proc, tag, MPI_COMM_WORLD);

	///	
	int msgCount = (dataLen/limit);
	if((dataLen % limit) != 0) msgCount++;
	//printf("-----[%d]dataLen: %d, limit: %d,msgCount: %d,tag: %d\n",rank,dataLen, limit, msgCount,tag);
	int msgSize;
	int i=0;
	
	//printf("(%d)*-*-*-*\t 1 - 1  --  1\n", tag - 2);

	//printf("333: 1\n");
	//if(rank == 0) while(*(masterReceiving + proc - 1) == 1) usleep(4);
	//else while(workerReceiving == 1) usleep(3);
	//if(rank != 0) printf("Sub Send[%d], msgCount: %d, dataLen: %d, limit: %d \n",rank , msgCount, dataLen, limit);
	//if(rank == 0) while(*(masterReceiving + proc - 1) == 1) 
	//	usleep(1);
	//else
	//	while(workerReceiving == 1) usleep(1);

	MPI_Ssend(&msgCount, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
/*
	if(rank != 0) printf("[%d]333: 1(i: %d)\n", rank, i);
	else
		printf("[%d]444: 1(i: %d)\n", rank, i);
*/
	//printf("444: 1\n");
	for(i = 0; i<msgCount; i++){
		//if(rank != 0) printf("333: 2(i: %d)\n", i);
		if(dataLen < limit)
			msgSize = dataLen;
		else
			msgSize = limit;
			
		//printf("[%d]333: 3(i: %d)\n", rank,i);
/*
		if(rank != 0) printf("[%d]333: 3(i: %d)\n", rank,i);		
		else printf("[%d]444: 2(i: %d)\n", rank, i);
		* */
		//sleep(1);
		//printf("1[%d]dataLen: %d, limit: %d,msgCount: %d of %d, msgSize: %d, input: %x + (i * limit): %x, tag: %d, msgTag: %d, proc: %d\n",rank, dataLen, limit, i, msgCount, msgSize,input, input + (i * limit), tag, (tag + i + 1), proc);

		//if(rank == 0) printf("Before Check, rank: %d, task: %d, subMsg: %d, msgCount: %d of %d, sender: %d, receiver: %d, STATUS: %d\n", rank, tag, (tag + i + 1), i, msgCount, rank, proc, *(masterReceiving + proc - 1));
		//else printf("Before Check, rank: %d, task: %d, subMsg: %d, msgCount: %d of %d, sender: %d, receiver: %d, STATUS: %d\n", rank, tag, (tag + i + 1), i, msgCount, rank, proc, workerReceiving);
		/*
		if(rank == 0){
			//while(*(masterReceiving + proc - 1) == 1) usleep(1);
			printf("MasterReceiving : %d ( proc: %d)\n", *(masterReceiving + proc - 1), proc);
}*/
		//}else{ 
		//	while(workerReceiving == 1) usleep(3);
		//}

		//if(rank != 0) printf("Before Send Sub, rank: %d, task: %d, subMsg: %d, msgCount: %d of %d, sender: %d, receiver: %d\n", rank, tag, (tag + i + 1), i, msgCount, rank, proc);
		//else printf("After Check, rank: %d, task: %d, subMsg: %d, msgCount: %d of %d, sender: %d, receiver: %d, STATUS: %d\n", rank, tag, (tag + i + 1), i, msgCount, rank, proc, workerReceiving);
		//if(rank == 0) while(*(masterReceiving + proc - 1) == 1) usleep(1);
 		//else while(workerReceiving == 1) usleep(1);		

		MPI_Ssend(input + (i * limit), msgSize, dataType, proc, tag + i + 1, MPI_COMM_WORLD);
		
		//printf("After Sending, rank: %d, task: %d, subMsg: %d, msgCount: %d of %d, sender: %d, receiver: %d, STATUS: %d\n", rank, tag,(tag + i + 1), i, msgCount, rank, proc, b);

		dataLen = dataLen - msgSize;
	}
}

///Send the task to a ready worker(Master -> Worker)
///input : array of data should be sent to the worker, countTask: the index of current Task
///countData : the number of data should be send to the worker ,proc : the worker
///size : the size of the datatype
///tag : represent the index of task for sorting the results
void sendTask(void *input,int countTask,int countData,MPI_Datatype taskType,int proc,int tag,int size){
	//printf("[%d]Sending Task: %d, To : %d\n", rank, countTask+1, proc);
	int sendType = 1;	
	///printf("[%d]sendType:%d, countTask:%d, countData:%d, size:%d(*:%d)\n",rank,sendType,countTask,countData,size,(countTask * countData) * size);

	//printf("(%d)*-*-*-*\t 1 - 1\n", tag -2 );

	//printf("MasterReceiving : %d ( proc : %d)\n", *(masterReceiving + proc - 1), proc);
	
	MPI_Send(&sendType, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
	///
	long dataShift = (countTask * countData) * size;

	//printf("(%d)*-*-*-*\t 1 - 2\n", tag -2);
	
	while(*(masterReceiving + proc - 1) == 1) usleep(2);

	//printf("(%d)*-*-*-*\t 1 - 3\n", tag - 2);

	//printf("[%d]dataShift: %d, (countData*size): %d, MSG_LIMIT: %d, proc: %d, tag: %d\n",rank, dataShift,countData*size, MSG_LIMIT, proc, tag);
	sendMultiMsgs(input + dataShift, countData*size, MSG_LIMIT, proc, tag, taskType);	
	
	//*(masterSending + proc - 1) = 0;	

//printf("Task Sent: %d, To : %d\n", countTask+1, proc);
	///printf("SENT DONE...tag: %d\n",tag);
	///
	/*
	///
	long k = (countTask * countData) * size;	
	printf("countTask: %d, countData: %d, size: %d, k: %d\n",countTask, countData, size, k);

	int error_code = MPI_Send(input + k,countData*size,taskType,proc,tag,MPI_COMM_WORLD);
	printf("error_code: %d\n",error_code);
	if (error_code != MPI_SUCCESS) {
		char error_string[BUFSIZ];
	   int length_of_error_string;

	   MPI_Error_string(error_code, error_string, &length_of_error_string);
	   fprintf(stderr, "----------\n%3d: %s\n----------\n", rank, error_string);
	}
	* */
	///printf("[%d]Task Sent: %d, To : %d\n", rank, countTask+1, proc);	
}

void setStartTask(struct taskReport * rep,int tNum,int procSent){	
	rep[tNum].taskNum = tNum+1;
	rep[tNum].proc = procSent;
	rep[tNum].start = getTimeMil();
}

void updateProcInfo(struct workerReport * rep,int n,int wRank,int busy,int taskN){
	int i=0;
	for(i=0;i<n-1;i++){
		if(rep[i].proc_rank == wRank){
			if(busy == 1){
				rep[i].current_tasks++;
			}else
				rep[i].current_tasks--;	
			if(rep[i].current_tasks == 0){
				rep[i].busy = 0;
				rep[i].status = 0;	
			}else{
				rep[i].busy = 1;
				rep[i].status = 1;	
			}	
				
			rep[i].busy = busy;
			rep[i].status = busy;
			rep[i].task = taskN;
			if(busy == 0) rep[i].totalTask++;
			//printf("rep[i].proc_rank: %d, sender: %d, busy: %d, taskN: %d, totalTask:%d, tasks: %d\n", rep[i].proc_rank, wRank, rep[i].busy, rep[i].task, rep[i].totalTask, rep[i].current_tasks);
			return;
		}					
	}
}

void setEndTask(struct taskReport * rep,int tNum, int sender){
	//printf("taskNum : %d, proc : %d, start : %d\n",rep[tNum].taskNum,rep[tNum].proc, rep[tNum].start);
	rep[tNum].end = getTimeMil();
	rep[tNum].procR = sender;
}

void recvMultiMsgs(void * input, int dataLen, int limit, int source, MPI_Datatype dataType, int tag){
//MPI_Send(&msgCount, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
//printf("Recv dataLen: %d\n", dataLen);
//MPI_Recv(input, dataLen, dataType, source, tag, MPI_COMM_WORLD, &status);
	
	int msgSize ;
	int msgCount;// = (dataLen/limit)+1;
	int i=0;
	//MPI_Send(&msgCount, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
	//printf("tag: %d\n", tag);
	//printf("444: 1\n");
	MPI_Recv(&msgCount, 1 , MPI_INT, source, tag, MPI_COMM_WORLD, &status);
	//if(rank == 0) printf("Sub RECV[%d], msgCount: %d, dataLen: %d, limit: %d \n",rank , msgCount, dataLen, limit);	
	//printf("444: 2\n");
	for(i = 0; i<msgCount; i++){
		///printf("444: 3(i:%d)\n",i);
		if(dataLen < limit)
			msgSize = dataLen;
		else
			msgSize = limit;
		//MPI_Send(input + dataShift + (i * msgSize), dataLen%msgSize,taskType,proc,tag + i,MPI_COMM_WORLD);
		
		//if(rank == 0) //printf("1 Result[%d]Recv sub Result : %d, (i * limit): %d, msgSize: %d, tag: %d, source: %d\n",rank,i,(i * limit),msgSize, (tag + i + 1),source );
		//printf("Results: Recv sub Result : %d, (i * limit): %d, msgSize: %d, tag: %d, source: %d, dataLen: %d, msgCount: %d\n", i, (i * limit), msgSize, (tag + i + 1),source , dataLen, msgCount);
		//else printf("1 Task[%d]Recv sub Result : %d, (i * limit): %d, msgSize: %d, tag: %d\n",rank,i,(i * limit),msgSize, (tag + i + 1));
		MPI_Recv(input + (i * limit), msgSize, dataType, source, tag + i + 1, MPI_COMM_WORLD, &status);
//if(rank != 0) printf("2 Result[%d]Recv sub Result : %d, (i * limit): %d, msgSize: %d, tag: %d, source: %d\n",rank,i,(i * limit),msgSize, (tag + i + 1), source);
		//else printf("2 Task[%d]Recv sub Result : %d, (i * limit): %d, msgSize: %d, tag: %d\n",rank,i,(i * limit),msgSize, (tag + i + 1));
		dataLen = dataLen - msgSize;
	}

}

///Receive the result by master(Worker -> Master)
void recvResult(void *msg,int resSize,MPI_Datatype resultType,int size,int source,int resIndex){

	//pthread_mutex_lock( &mutex1 );
	//MPI_Status status;
	///MPI_Recv(msg + ((resIndex-2) * resSize) * size,resSize*size,resultType,source,resIndex,MPI_COMM_WORLD,&status);	
	//MPI_Send( &resIndex, source, MPI_INT, 0, resIndex, MPI_COMM_WORLD);
	///
	recvMultiMsgs(msg + (((resIndex-2) * resSize) * size), resSize*size, MSG_LIMIT, source, resultType,resIndex);
//pthread_mutex_unlock( &mutex1 );
}


//Send the shared data to a worker
void sendSharedData(void *shared_data, int data_len, int w){	
	printf("[%d]. Sending shared data (%d) to %d...\n", rank, data_len, w);
	
	if(data_len != 0){
		int send_code = SHARED_DATA_FROM_MASTER;
		MPI_Ssend(&send_code, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
		MPI_Ssend(&data_len, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);	
		sendMultiMsgs( shared_data, data_len, MSG_LIMIT, w, send_code, MPI_CHAR);
		printf("[%d]. Finish Sending shared data (%d) to all workers...\n", rank, data_len);
	}
}

//Send the shared data to all workers
void sendSharedDataToAll(void *shared_data, int shared_data_size, int shared_data_len, int n){	
	printf("[%d]. Sending shared data (%d) to all workers...\n", rank, shared_data_size*shared_data_len);
	int i=0;
	for( i=1; i<n; i++)	
		sendSharedData(shared_data, shared_data_size*shared_data_len, i);
}


///-------------------------------------------------
///--------Worker Functions-------------------------
///-------------------------------------------------


void writeWorkerInfo(int rank, double sec, float load, float uti, int cores){
	/*
	char* load_file = (char*)malloc(sizeof(char)*45);
	sprintf(load_file,"worker_info/load_info_%d.csv", rank);
	FILE* f = fopen(load_file,"a+");	
	printf("Sec: %.2f, Load: %.2f, Perc %.2f, Cores %d\n", sec, load, uti, cores);
	fprintf(f, "%.2f, %.2f, %.2f, %d\n", sec, load, uti, cores);
	fclose(f);
	* */
}

///Receive Message  from the Master
void recvWorker(void *out,int countData,MPI_Datatype taskType,int *tag,int *type,int tasks,int inSize,int inLen){
	//printf("[%d]recvWorker 1\n",rank);
	MPI_Status status;
	int sendType = -1;
	///printf("RANK: %d\n", rank);
	//printf("[%d]BEFORE: recvWorker, sendType: %d\n",rank, sendType);
	MPI_Recv(&sendType, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
	//printf("[%d]recvWorker, sendType: %d\n",rank, sendType);
	if(sendType == 1)
	{

		//return;	
		///
	}
	
	//printf("[%d]-*-*- 33\n",rank);
	*type = sendType;
	*tag = status.MPI_TAG;
}

void recvTask(void *out,int countData,MPI_Datatype taskType,int *tag,int inSize){
	//MPI_Recv(out,countData * inSize,taskType,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	int subTag = *tag; 

	//printf("[%d]data received: %d\n", rank, (countData * inSize));
	recvMultiMsgs(out, countData * inSize, MSG_LIMIT, 0, taskType ,subTag);

	*tag = subTag;
	return;
}

///get The three numbers that represent the load
///when the command 'cat /proc/loadavg/' executed
void getLoadAvg(char * str, double * load){
	int i=0;
	//double * load = (double*)malloc(sizeof(double)*3);
	double d=0;	
	char * p = str;
	while(*p != '\n')
	{ 	d = atof(p) + (10 * d);			
		while(*p != ' ')
			p++;
		load[i++] = d;
		if(i==3)
			break;	
		d=0.0;
		p++;
	}
}

///
void getCpuUti(char * str){
	int i=0;
	char * p = str;
	while(i < 10000)
	{ 	
		printf("%c\t", *p);
	}
	//double * load = (double*)malloc(sizeof(double)*3);
	/*
	double d=0;	
	char * p = str;
	while(*p != '\n')
	{ 	d = atof(p) + (10 * d);			
		while(*p != ' ')
			p++;
		load[i++] = d;
		if(i==3)
			break;	
		d=0.0;
		p++;
	}
	* */
}

struct workerLoad * addWorkerLoad(struct workerLoad * w1Load, int c_load, float p, float s){
	struct workerLoad * pl = w1Load;
	if(w1Load == 0){
		//printf("NULL\n");
		w1Load = (struct workerLoad *)malloc(sizeof(struct workerLoad));
		w1Load->cur_load = c_load;
		w1Load->sec = s;
		w1Load->per = p;
		w1Load->next = 0;	
		//w1Load = p;	
	}else{
		//printf("NO NULL\n");
		while(pl->next != 0){
			pl = pl->next;		
		}
		if((pl->cur_load == c_load)){
			pl->sec = s;
			pl->per = p;
		}else{
			pl->next = (struct workerLoad *)malloc(sizeof(struct workerLoad));
			pl->next->cur_load = c_load;
			pl->next->sec = s;
			pl->next->per = p;		
			pl->next->next = 0;
		}
	}
	return w1Load;
}

void printWorkerLoads(struct workerLoad * wLoad){
	struct workerLoad * p;
	p = wLoad;
	while(p != NULL){
		printf("Load(rank: %d, %d, %.2f, %.2f)\n",rank, p->cur_load, p->per, p->sec);	
		p = p->next;	
	}	
}

///Send the response of the command sent by master
void sendResponse(void *res,int resSize,MPI_Datatype resultType){	
	
	int msgType = 0;
	MPI_Send(&msgType,1,MPI_INT,0,msgType , MPI_COMM_WORLD);
	MPI_Send(res,resSize,resultType,0,msgType, MPI_COMM_WORLD);
	//printf("send response Done to Master from [%d]\n",rank);
}

void sendResponse1(float res,int resSize,MPI_Datatype resultType){	
	
	int msgType = 0;
	MPI_Send(&msgType,1,MPI_INT,0,msgType , MPI_COMM_WORLD);
	MPI_Send(&res,resSize,resultType,0,msgType, MPI_COMM_WORLD);
	//printf("send response Done to Master from [%d] - %f\n",rank, res);
}

///Send the result from worker to master(Wroker -> Master)
void sendResult(void *res,int resSize,MPI_Datatype resultType,int tag){	

	//printf("[%d]sending Result: %d\n", rank, tag);
	
	MPI_Send(&tag,1,MPI_INT,0,tag , MPI_COMM_WORLD);

	///printf("[%d]sendResultTag tag: %d\n", rank,tag);

	while(workerSending == 0) usleep(1);
	//while(workerReceiving == 1) usleep(1);

	//printf("[%d]Result sent: res: %x, resSize: %d\n", rank, res,resSize);
	sendMultiMsgs(res, resSize, MSG_LIMIT, 0, tag, resultType);
	///printf("[%d]Result sent: %d\n", rank, tag);
	
	//*****************************************************
	//*****Write the current load to a file****************
	//-----------------------------------------------------
	//printf("[%d]Worker Status...\n", rank);
	
	/*
	struct sysinfo sys_info;
	
	if(sysinfo(&sys_info) != 0)
		perror("sysinfo");

	double load = sys_info.loads[0]/65536.0;
	
	writeWorkerInfo(rank, (MPI_Wtime() - startTime), load, 100, 8);
	*/
	//*****************************************************
	//-----------------------------------------------------
	//*****************************************************
	
	workerT2 = getTimeMil();
	
	workerSending = 0;
}

void *workerStatus(void *arg)
{		
	//w1Load = (struct workerLoad *)malloc(sizeof(struct workerLoad ));
	
	/*
	char* load_file = (char*)malloc(sizeof(char)*45);
	sprintf(load_file,"worker_info/load_info_%d.csv", rank);
	FILE* f = fopen(load_file,"a+");	
	fprintf(f, "\n-----------------------------------------------------------\n");
	fprintf(f, "%d X %d\n", totalIndex, totalIndex);
	fprintf(f, "-----------------------------------------------------------\n");
	fclose(f);
	*/
	
	//printf("[%d]Worker Status...\n", rank);
	char *res = (char*)malloc(sizeof(char) * 10000);
	//char *res2 = (char*)malloc(sizeof(char) * 1000);
	double * load = (double*)malloc(sizeof(double)*3);

	/*
	///----------Static Mode-------------		
	systemCall("cat /proc/loadavg",res);
	getLoadAvg(res, load);
	load[0] = load[0];// + load[1] + load[2];
	sendResponse(load,1,MPI_DOUBLE);		
	///----------------------------------
	*/
	///----------Dynamc, Load or Mobility Mode-------------

	systemCall("cat /proc/loadavg",res);
	getLoadAvg(res, load);
	//
	systemCall("cat /proc/cpuinfo | grep processor | wc -l",res);
	///printf("[%d]Worker Cores: %s\n", rank, res);
	int cores = atoi(res);
	currentCores = cores;
	load[0] = load[0];// + load[1] + load[2];
	load[1] = cores;
	load[2] = 0.0;
	//load[2] = getCPUUti();
	//printf("[%d]Send Laod...\n", rank);
	sendResponse( load, 3, MPI_DOUBLE);
	
	//writeWorkerInfo(rank, (MPI_Wtime() - startTime), load[0], 0, cores);
	/*	
	if(mainIndex == 0)
		writeWorkerInfo(rank, (MPI_Wtime() - startTime), load[0], 0, cores);
	else
		writeWorkerInfo(rank, (MPI_Wtime() - startTime), load[0], (*mainIndex)*100.0/totalIndex, cores);
	//printf("[%d]mainIndex: %.2f\n", rank, ((*mainIndex)*100.0)/totalIndex);
	*/
	//return;


	double * load2 = (double*)malloc(sizeof(double)*3);

	while(1){	
		int l = getLoadValue();
		if(mainIndex != NULL){			
			*load2 = l;// + load[1] + load[2];
			*(load2+1) = cores;
			*(load2+1) = 0.0;
			//printf("[%d]Load Amount: %d, Percentage: %.2f, Time: %.2f\n",rank, l, (*mainIndex)*100.0/totalIndex, (MPI_Wtime() - startTaskTime));
			w1Load = addWorkerLoad(w1Load, l, (*mainIndex)*100.0/totalIndex, (MPI_Wtime() - startTaskTime));
			//printf("Load Sent: %f\n", *((double*)load2));
			sendResponse1(l, 1, MPI_DOUBLE);
		}
	}

/*
    sleep(3);
	while(1){	
		
		systemCall("cat /proc/loadavg",res);
		
		getLoadAvg(res, load);
		//
		//systemCall("cat /proc/cpuinfo | grep processor | wc -l",res);
		///printf("[%d]Worker LOAD: %s\n", rank, res);
		//int cores = atoi(res);
		//systemCall("./getCpu", res);
		//float cpuUti = atof(res);
		//cpuUti = cpuUti / cores;
		//printf("The CPU Uti: %.2f\n", cpuUti);
		//
		//getCpuUti(res2);
		load[0] = load[0];// + load[1] + load[2];
		load[1] = cores;
		
		//load[2] = getCPUUti();
		//printf("[%d]Send Laod...\n", rank);
		//printf("[%d]mainIndex: %.2f \%\n", rank, (*mainIndex)*100.0/totalIndex);
		//writeWorkerInfo(rank, (MPI_Wtime() - startTime), load[0], load[2], cores);
		//writeWorkerInfo(rank, (MPI_Wtime() - startTime), load[0], (*mainIndex)*100.0/totalIndex, cores);
		sendResponse(load, 3, MPI_DOUBLE);		
		sleep(4);
	}	
	*/
	///----------------------------------
	return NULL;
}

void *workerTask2(void *arg)
{	
	startTaskTime = MPI_Wtime();
	
	struct task_data *td = ((struct task_data *)arg);	
	//printf("[%d] input: %x, output: %x, pars: %d\n", rank, td->input, td->output, td->pars);
	//fp1(td->input, td->inputLen, td->output, td->outputLen, td->pars, td->parsSize, td->moving, td->tag);	
	if(*(td->moving) == 1)
		return NULL;
	///if(moved == 1)return;
	//printf("TAG: %d\n",td->tag);
	//print2DArrayFrom1D(td->output,2,10);
	/*
	if(*(td->moving) == 1){
		free(td->input);
		free(td->output);		
		return;
	}*/
	pthread_mutex_lock( &mutex1 );
	//printf("---------------------- outputLen: %d, td->outSize: %d\n",td->outputLen, td->outSize);
	sendResult(td->output,td->outputLen * td->outSize,td->mpi_dt,td->tag);
	*count = *count + 1;
	td->end = MPI_Wtime();
	pthread_mutex_unlock( &mutex1 );
	
	free(td->input);
	free(td->output);
	//*count = *count + 1;
	//free(td->input);
	return NULL;
	//exit(0);
}

void *workerTask(void *arg)
{	
	startTaskTime = MPI_Wtime();
	
	struct task_data *td = ((struct task_data *)arg);	

	//printf("******************* Start Running Tag: %d **\n", td->tag-1);
	//printf("td->moving: %d\n",*(td->moving));
	//fp1( td->input, td->inputLen, td->output, td->outputLen, td->pars, td->parsSize, td->moving, td->tag);	
	///if(moved == 1)return;
	//printf("TAG: %d\n",td->tag);
	//print2DArrayFrom1D(td->output,2,10);
	//printf("***************** Sending Result Tag: %d **\n", td->tag-1);
	if(*(td->moving) == 1)
		return NULL;
	
	//printf("***************** Sending Result Tag: %d **\n", td->tag-1);

	pthread_mutex_lock( &mutex1 );

	///printf(" *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");
	///printf("*-*-*-*-*-Worker starts sending result: %d*-*-*-*-*workerSending: %d \n", td->tag-1, workerSending);
	///printf(" *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");

	//printf("---------------------- outputLen: %d, td->outSize: %d\n",td->outputLen, td->outSize);
	sendResult(td->output,td->outputLen * td->outSize,td->mpi_dt,td->tag);
	
	*count = *count + 1;
	td->end = MPI_Wtime();

	pthread_mutex_unlock( &mutex1 );		
	
	//free(td);
	free(td->input);
	free(td->output);
	//free(td->pars);

	//printf("[tag: %d]E:------------------------------------MPI_Wtime: %.3f\n", td->tag, MPI_Wtime());
	/*
	free(td->pars);
	free(td->output);
	free(td);
	* */

	///printf(" *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");
	///printf("*-*-*-*-*-Worker ends sending result: %d*-*-*-*-* \n", td->tag-1);
	///printf(" *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");

	return NULL;
	//exit(0);
}

void *loadReportFun(void * arg){
	int *n = (int *)arg;
	while(1){
		sleep(3);
		//viewLoadReport(lReport, *n);
		//Send Load report to All workers
		//Mobile Process
		///-----------Mobility mode ----------
		sendLoadReport(lReport, *n-1);		
	}
}

void *sendToWorker(void *arg){
	
	struct task_to_send *tts = ((struct task_to_send *)arg);

	//printf("S *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");
	//printf("*-*-*-*-*-Master starts sending task: %d*-*-*-*-* \n", tts->countTask);
	///printf(" *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");

	//while(*(masterReceiving + tts->worker - 1) == 1) usleep(2);	
	//printf("MasterReceiving : %d ( proc: %d) \n", *(masterReceiving + tts->worker - 1), tts->worker);
	
	//*(masterSending + tts->worker - 1) = 1;

	pthread_mutex_lock( &mutex1 );
	//while(master_mutex == 1) usleep(2);
	//master_mutex = 1;

	//printf("(%d)*-*-*-*\t 1\n", tts->countTask);

	sendTask( tts->input, tts->countTask, tts->countData, tts->taskType, tts->worker, tts->countTask + 2,tts->inSize);
	
	//printf("(%d)*-*-*-*\t 2\n", tts->countTask);

	setStartTask(tReport, tts->countTask, tts->worker);

	//printf("(%d)*-*-*-*\t 3\n", tts->countTask);

	//For report
	updateProcInfo(lReport, numprocs, tts->worker, 1 , tts->countTask);

	//printf("(%d)*-*-*-*\t 4\n", tts->countTask);

	//master_mutex = 0;
	pthread_mutex_unlock( &mutex1 );

	//printf(" *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");
	//printf("*-*-*-*-*-Master end sending task: %d*-*-*-*-* \n", tts->countTask);
	///printf(" *-*-*-*-*-*-**-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* \n");
	
	return NULL;
}

void sendMoveDone(int source, int moved_task){		
	int msgType = 1;
	MPI_Send(&msgType,1,MPI_INT,0,msgType , MPI_COMM_WORLD);
	int move_done = -1;
	MPI_Send(&move_done, 1, MPI_INT, 0, msgType, MPI_COMM_WORLD);
	MPI_Send(&source, 1, MPI_INT, 0, msgType, MPI_COMM_WORLD);
	MPI_Send(&moved_task, 1, MPI_INT, 0, msgType, MPI_COMM_WORLD);
}

float getCurrentLoad(){
	char *res = (char*)malloc(sizeof(char) * 1000);
	systemCall("cat /proc/loadavg",res);
	double * load = (double*)malloc(sizeof(double)*3);
	getLoadAvg(res, load);				
	free(res);
	return load[0];
}

void checkLoadState(float *cur_load, float *old_load,struct workerReport * report,int workers){
	int i = 0;
	float val = 0;
	//float val2 = 0;
	float avg_load = 0;
	float min_free_load = 1000000;
	int min_free_worker = -1;
	int current_worker_i = 0;
	//if(rank ==1)
	//	viewLoadReport(report, workers+1);
	///printf("[%d] - checkLoadState - 1\n",rank);
	for(i=0;i<workers;i++){		
		if((report[i].proc_load_old != 0) || (report[i].proc_load >= 1)){
			//float r = (report[i].proc_load / report[i].cores) * (report[i].cpuUti / report[i].cores);
			val = (report[i].proc_load/report[i].proc_load_old) * (report[i].proc_load / report[i].cores);
			//val2 = (report[i].proc_load/report[i].proc_load_old) * (report[i].cpuUti / report[i].cores);
			//printf("[%d] - checkLoadState - i: %d - r: %.2f [cur: %.2f, old: %.2f, uti: %.2f]\n", rank, i, r, report[i].proc_load, report[i].proc_load_old, report[i].cpuUti);
		}else
			val = 0;
		///printf("[%d] - checkLoadState - i: %d - (%.5f)-val: %.5f\n", rank, i, val2,val);
		if(report[i].proc_rank == rank){	
			current_worker_i = i;	
			if((*cur_load == 0) && (*old_load == 0)){
				*cur_load = val;
				*old_load = val;
			}else{
				*old_load = *cur_load;
				*cur_load = val;				
			}
			///printf("[%d]worker [%d] load : %.3f,\t old: %.3f,\t Percentage: %.3f, N: %.3f,\tTASK:%d, busy: %d\n",rank,report[i].proc_rank,report[i].proc_load,report[i].proc_load_old,report[i].proc_load/report[i].proc_load_old,val,report[i].task,report[i].busy);
			//avg_load = (*cur_load + *old_load + (getCurrentLoad()*8))/3;
			avg_load = (*cur_load + *old_load )/2;
		}
		///printf("[%d] - checkLoadState - 1-2(%d)\n", rank, i);
		if(report[i].busy == 0){
			//printf("[%d]worker [%d] load : %.3f,\t old: %.3f,\t Percentage: %.3f, N: %.3f,\tTASK:%d, busy: %d\n",rank,report[i].proc_rank,report[i].proc_load,report[i].proc_load_old,report[i].proc_load/report[i].proc_load_old,val,report[i].task,report[i].busy);
			//printf("[%d]worker [%d] , busy: %d, val: %.3f, min_free_load: %.3f, min_free_worker: %d\n",rank,report[i].proc_rank, report[i].busy,val,min_free_load,min_free_worker);
			if(min_free_load > val){				
				min_free_load = val;
				min_free_worker = report[i].proc_rank;
			}
		}
		///printf("[%d] - checkLoadState - 1-3(%d)\n", rank, i);
	}
	///printf("[%d] - checkLoadState - 2\n",rank);
	if(report[current_worker_i].busy == 1){
		float percentage = 100*(avg_load/min_free_load);
		//printf("[%d]The Best Free Worker: %d whose load: %.3f, MY AVG LOAD: %.3f(%.0f %),\tpercentage: %.3f,\tbusy: %d,TASK: %d\n",rank,min_free_worker, min_free_load,avg_load, (avg_load * 100),percentage,report[current_worker_i].busy, report[current_worker_i].task);
		//printf("[%d]LOAD NO: %d\n", rank, report[current_worker_i].loadNo);
		//if(rank == 1)
		//	if((report[current_worker_i].loadNo == 4) || (report[current_worker_i].loadNo == 8))
				//sendMoveRequest(2);
		//if(rank == 2)
		//	if((report[current_worker_i].loadNo == 12) || (report[current_worker_i].loadNo == 16))
				//sendMoveRequest(1);
	
		///Move if
		// the percentage of increased load over than 40%
		// and the whole load on this worker is more than 0.62(8 core, 4 are busy ni processing, 1 for mangaing, still 3 so: 5 of 8 (0.625))
		if((percentage > 140.0) && (avg_load > 0.60)){
			//sendMoveRequest(min_free_worker);
		///	printf("[%d]Move request sent to %d\n", rank, min_free_worker);
		}
	}
	///printf("[%d] - checkLoadState - 3\n",rank);
}

//Receive the load report from master by worker
void recvLoadReport(struct workerReport * lReport, int workers,int sendType){
	MPI_Status status;
	MPI_Recv(lReport,sizeof(struct workerReport)*workers,MPI_CHAR,0,sendType,MPI_COMM_WORLD,&status);
}

///Send data from source to destination
void sendMovableTask(struct task_data * myTd, int destinationWorker,int inSize,int inLen,MPI_Datatype inType,int outSize,int outLen,MPI_Datatype outType,int state_size){
	//int sendType = 4;
	int worker_to_worker = 2012;
	//printf("[%d]sendMovableTask:1, destinationWorker:%d\n",rank,destinationWorker);
	sendMultiMsgs(myTd->input, inSize*inLen, MSG_LIMIT, destinationWorker, worker_to_worker, inType); 
	//printf("33: 2\n");
	sendMultiMsgs(myTd->output, outSize*outLen, MSG_LIMIT, destinationWorker, worker_to_worker + 10, outType);
	//printf("33: 3\n");

	//printf("Source(Task=%d)-- PAR[0]: %d\t, PAR[1]: %d\t, PAR[2]: %d\t, PAR[3]: %d\t, PAR[4]: %d\t, PAR[5]: %d\t, PAR[6]: %d\n", myTd->tag, *(((int*)myTd->pars)+0), *(((int*)myTd->pars)+1), *(((int*)myTd->pars)+2),*(((int*)myTd->pars)+3),*(((int*)myTd->pars)+4), *(((int*)myTd->pars)+5),*(((int*)myTd->pars)+6));

	sendMultiMsgs(myTd->state, state_size, MSG_LIMIT, destinationWorker, worker_to_worker + 20, MPI_CHAR);
	//printf("33: 4\n");
	MPI_Send(&myTd->tag, 1,MPI_INT, destinationWorker, 70, MPI_COMM_WORLD);
	//free(td->input);
}

///Receive the movable task
void recvMovableTask(struct task_data * myTd,int sourceWorker, int inSize,int inLen,MPI_Datatype inType,int outSize,int outLen,MPI_Datatype outType,int state_size){
	MPI_Status status;
	//printf("[%d]recvMovableTask:1, sourceWorker:%d\n",rank,sourceWorker);
	int worker_to_worker = 2012;
	//printf("44: 1\n");
	recvMultiMsgs(myTd->input, inSize*inLen, MSG_LIMIT, sourceWorker, inType, worker_to_worker);
	//printf("44: 2\n");
	recvMultiMsgs(myTd->output, outSize*outLen, MSG_LIMIT, sourceWorker, outType, worker_to_worker + 10);
	//printf("44: 3\n");
	recvMultiMsgs(myTd->state, state_size, MSG_LIMIT, sourceWorker, MPI_CHAR, worker_to_worker + 20);
	//printf("44: 4\n");
	MPI_Recv(&myTd->tag,1,MPI_INT,sourceWorker,70,MPI_COMM_WORLD,&status);
}

void findNetLatency(int n_p){
	//double net_lat = 0;
	if(n_p > 1){
		MPI_Status status;	
		int i=0, reps = 3;
		int source, dest, send_status, recv_status;	
		int tag = 1;
		MPI_Barrier(MPI_COMM_WORLD);
		if(rank == 0){	
			source = 1;		
			dest = 1;
			int msg = 1;
			double t1, t2, sum = 0, avg;
			double t1_o, t2_o;
			
			for(i = 0; i< reps; i++){			
				t1 = MPI_Wtime();
				/* send message to worker - message tag set to 1.  */
				/* If return code indicates error quit */
				//printf("Master send to 1 tag 50!\n");
				send_status = MPI_Ssend(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
				//printf("waiting...!\n");
				if (send_status != MPI_SUCCESS) {
					printf("Send error in task 0!\n");
					MPI_Abort(MPI_COMM_WORLD, send_status);
					exit(1);					
				}			
				/* Now wait to receive the echo reply from the worker  */
				/* If return code indicates error quit */
				recv_status = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD, 
						&status);
				if (recv_status != MPI_SUCCESS) {
					printf("Receive error in task 0!\n");
					MPI_Abort(MPI_COMM_WORLD, recv_status);
					exit(1);
				}
				t2 = MPI_Wtime();
				
				sum += (t2 - t1);
			}
			
			avg = (sum*1000000)/reps;
			t1_o = MPI_Wtime();
			t2_o = MPI_Wtime();
			printf("***************************************************\n");
		    printf("*** Avg round trip time = %-7.2f microseconds ****\n", avg);
		    printf("*** Avg one way latency = %-7.2f microseconds ****\n", avg/2);
		    printf("*** MPI_Wtime() overhead= %-7f microseconds ****\n", t2_o - t1_o);
		    printf("***************************************************\n");
			
		}else if(rank == 1){
			source = 0;		
			dest = 0;
			char msg;
			for(i = 0; i< reps; i++){
				//usleep(3000000);
				recv_status = MPI_Recv(&msg, 1, MPI_BYTE, source, tag, MPI_COMM_WORLD, 
                    &status);
				if (recv_status != MPI_SUCCESS) {
					printf("Receive error in task 1!\n");
					MPI_Abort(MPI_COMM_WORLD, recv_status); 
					exit(1);
				}
				send_status = MPI_Send(&msg, 1, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
				if (send_status != MPI_SUCCESS) {
					printf("Send error in task 1!\n");
					MPI_Abort(MPI_COMM_WORLD, send_status);
					exit(1);
				}
			} 
		}	
		
		MPI_Barrier(MPI_COMM_WORLD);
	}
}  
   

void initHWFarm(int argc, char ** argv){ 
	//MPI_Init(&argc, &argv);
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);	
	MPI_Get_processor_name(processor_name, &namelen);
	
	printf("[%d]. MPI_WTIME_IS_GLOBAL: %d, %f\n", rank, MPI_WTIME_IS_GLOBAL, MPI_Wtime());
	
	startTime = MPI_Wtime();
}

void finalizeHWFarm(){
	printf("[%d]. MPI_WTIME_IS_GLOBAL: %d, %f\n", rank, MPI_WTIME_IS_GLOBAL, MPI_Wtime());
	
	MPI_Finalize(); 
}



///---------------------------------------------------
  
  
///The farm function
///worker : the name of function shuld be executed by the workers
///tasks : the size of tasks
///input : the array of data should be processed by the skeleton
///inSize : the datatype of input data
///inLen : the size of input data in one task
///taskType : the type of MPI input data
///output : the array to store the result in it
///outSize : the datatype of output data
///outLen : the size of data in one result from worker
///resultType : the type of MPI output data
void hwfarm(fp worker,int tasks,
			void *input, int inSize,int inLen, 
			void *shared_data, int shared_data_size, int shared_data_len, 
			void *output, int outSize, int outLen, hwfarm_state main_state,
			int mobility){

	printf("[%d]. START SKELETON...\n", rank);
	
	totalIndex = main_state.max_counter;
	
	//findNetLatency(numprocs);
	
	MPI_Barrier(MPI_COMM_WORLD);
	
	//printf("[%d]. LOGGING...\n", rank);
	
	if(rank == 0){	
		int m_cores = getNumberOfCores();
		if(m_cores == 0)
			m_cores = 1;
		Master_FREQ = getCoresFreq() / m_cores;
		
		//printf("[%d]. LOGGING...\n", rank);
		
		srand(time(NULL));
		
		///Get Tha Start Time
	    //int st = getTimeMil();
	    start_time = MPI_Wtime();
		
		int w=0;
		int w_msg_code=0;
		
		if(isFirstCall){
			w_load_report = (struct worker_load *)malloc(sizeof(struct worker_load)* (numprocs - 1));
			w_load_report_tmp = (struct worker_load *)malloc(sizeof(struct worker_load)* (numprocs - 1));
		}		
		
		//printf("[%d]. LOGGING...", rank);
		
		//pthread_create(&w_load_report_th, NULL, workerLoadReportFun, w_load_report);
		
		//double all_overhead = MPI_Wtime();		
		
		//printf("[%d]. Allocation Overhead start: %.6f \n", rank, (MPI_Wtime())); 	

		if(isFirstCall){
			getInitialWorkerLoad(w_load_report, numprocs);			
		}
		
		//printf("[%d]. LOGGING...", rank);
		
		
		//printf("[%d]. Allocation Overhead start: %.6f \n", rank, (MPI_Wtime() - all_overhead));

		struct task_pool * pool = 0;
		pool = create_task_pool(tasks, input, inLen, inSize, 
								output, outLen, outSize, MPI_CHAR, 
								main_state.state_data, main_state.state_len, 
								main_state.counter, main_state.max_counter);
							
		//printf("[%d]. Allocation Overhead start: %.6f \n", rank, (MPI_Wtime() - all_overhead));
				
		masterReceiving = (int *)malloc((sizeof(int) * (numprocs - 1)));
		masterSending = (int *)malloc((sizeof(int) * (numprocs - 1)));
		int mR = 0;
		for(mR = 0; mR < numprocs - 1; mR++){
			*(masterReceiving + mR) = 0;
			*(masterSending + mR) = 0;
		}
		
		//printf("[%d]. Allocation Overhead start: %.6f \n", rank, (MPI_Wtime() - all_overhead));
		
		//printf("[%d]. Allocation Overhead start: %.6f \n", rank, (MPI_Wtime()));
		
		//printTaskPool(pool);		
		//printMobileTask(pool->m_task_report->m_task);
		
		//Send a task to a worker
		int dest_w = -1;
		//int sel_task = -1;
		
		struct mobile_task_report * m_t_r;		
		
		//int w_balanced = 1;
		int processing_tasks = 0;	
		
		//Distribution based on load on workers(number of tasks equal
		//to number of cores)
		//Evenly distribution based on worker's load
		int LOAD_DISTRIBUTION = 1;
		int EVENLY_DISTRIBUTION = 2;
		int CORE_RELATIVE = 3;
		int EXPERIMAENT_MODE = 4;
		int WORKERS_COUNT = numprocs - 1;
		
		//int distrbution_mode = LOAD_DISTRIBUTION;	
		int distrbution_mode = CORE_RELATIVE;	
		//int distrbution_mode = EXPERIMAENT_MODE;	
		int * tasksPerWorker;
		if( distrbution_mode == EVENLY_DISTRIBUTION ){
			tasksPerWorker = (int *)malloc(sizeof(int) * WORKERS_COUNT);
			int w_i = 0;
			//distributed tasks
			int dis_tasks = tasks;
			for(w_i = 0; w_i < WORKERS_COUNT; w_i++){
				tasksPerWorker[w_i] = 0;
			}
			
			int task_limit = 1;
			while( dis_tasks > 0){
				for(w_i = 0; w_i < WORKERS_COUNT; w_i++){
					if(w_load_report[w_i].w_running_procs + tasksPerWorker[w_i] < task_limit){
						tasksPerWorker[w_i]++;
						dis_tasks--;
					}
					if(dis_tasks == 0)break;
				}
				task_limit++;
			}						
			/*
			printf("Task Mapping\n");
			for(w_i = 0; w_i < WORKERS_COUNT; w_i++){
				printf("[W: %d](cur: %d): %d\n", (w_i+1), w_load_report[w_i].w_running_procs, tasksPerWorker[w_i]);
			}
			* */
		}else if( distrbution_mode == CORE_RELATIVE ){
			tasksPerWorker = (int *)malloc(sizeof(int) * WORKERS_COUNT);
			int w_i = 0;
			//distributed tasks
			int dis_tasks = tasks;
			for(w_i = 0; w_i < WORKERS_COUNT; w_i++){
				tasksPerWorker[w_i] = 0;
			}
			
			int C=0;//Total number of cores for all workers
			for(w_i = 0; w_i < WORKERS_COUNT; w_i++){
				C += w_load_report[w_i].w_cores;
				//printf("[W: %d] c: %d, C: %d\n", (w_i+1), w_load_report[w_i].w_cores, C);
			}
			
			//printf("Task Mapping\n");
			if(C > 0 && dis_tasks > 0){				
				for(w_i = 0; w_i < WORKERS_COUNT; w_i++){
					int dis_task_i = ceil(w_load_report[w_i].w_cores * dis_tasks * 1.0 / C);
					//printf("[W: %d] Ci: %d\n", (w_i+1), dis_task_i);
					tasksPerWorker[w_i] = dis_task_i;
					dis_tasks -= dis_task_i;
					C -= w_load_report[w_i].w_cores;
					if(dis_tasks <= 0) break;
				}
			}	
			
			//Adaptivity Expriments.
			//This line is added to assign all tasks to worker 1
			//tasksPerWorker[0] = tasks;
		}
		
		//printf("[%d]. Allocation Overhead start: %.6f \n", rank, (MPI_Wtime()));
		//printf("[%d]. Allocation Overhead start: %.6f \n", rank, (MPI_Wtime() - all_overhead));
		
		///Send the shared data to the workers
		sendSharedDataToAll(shared_data, shared_data_size, shared_data_len, numprocs);
		
		//
		
		///Send the tasks to the workers
		while((m_t_r = getReadyTask(pool)) != NULL){
			///Get the best worker to send a task. This selection
			///depends on random selection (Static) or by dynamic
			///selection.
			if(distrbution_mode == LOAD_DISTRIBUTION){
				getBestWorker(w_load_report, &dest_w, numprocs, 0);	
			}else if( distrbution_mode == EVENLY_DISTRIBUTION || distrbution_mode == CORE_RELATIVE){
				getValidWorker(tasksPerWorker, WORKERS_COUNT, &dest_w);
			}else if(distrbution_mode == EXPERIMAENT_MODE){
				dest_w = 1;
			}
			printf("Best worker: %d\n", dest_w);
			if(dest_w == -1)
				break;
			
			///Send the task to the selected worker 'dest_w'		
			sendMobileTaskM(m_t_r, dest_w);
			
			///Modify the status of the worker
			modifyWorkerLoadReport(dest_w, w_load_report, numprocs, 0);
			processing_tasks++;			
			
			//if(w_balanced)break;
			//if(processing_tasks > 5)break;
			//
		}		
		
		printWorkerLoadReport(w_load_report, numprocs);		
		
		if(isFirstCall){
			pthread_create(&w_load_report_th, NULL, workerLoadReportFun, w_load_report);
			
			pthread_create(&w_network_latency_th, NULL, networkLatencyFun, w_load_report);
			
			isFirstCall = 0;
		}
		
		do{
			///Receive the msg code first; depending on this code, 
			///the type of data will be detected
			recvMsgCode(&w,&w_msg_code);
			
			printf("----Master receives the latest load request from %d(t: %d)\n", w, w_msg_code);

			///Load comes from a worker
			if(w_msg_code == INIT_LOAD_FROM_WORKER){
				setWorkerLoad(w_load_report, numprocs, w, w_msg_code);
			}
			///Move report from workers
			else if(w_msg_code == MOVE_REPORT_FROM_WORKER){
				recvMoveReport(pool, w, w_msg_code);
			}
			else if(w_msg_code == LATEST_LOAD_REQUEST){
				//
				//printf("----Master receives the latest load request from %d(t: %f)\n----", w, MPI_Wtime());
				sendLatestLoad(w_load_report, numprocs, w);
			}
			///Receive Worker load report
			else if(w_msg_code == UPDATE_LOAD_REPORT_REQUEST){
				//double load_overhead_update = MPI_Wtime();
				
				recvWorkerLoadReport(w_load_report_tmp, w, w_msg_code);
				
				int i=0;
				for(i=0; i<numprocs-1; i++)
				{
					w_load_report[i].w_load_no = w_load_report_tmp[i].w_load_no;
					w_load_report[i].w_load_avg_1 = w_load_report_tmp[i].w_load_avg_1;
					w_load_report[i].w_load_avg_2 = w_load_report_tmp[i].w_load_avg_2;
					w_load_report[i].estimated_load = w_load_report_tmp[i].estimated_load;
					w_load_report[i].w_running_procs = w_load_report_tmp[i].w_running_procs;
					w_load_report[i].w_cpu_uti_1 = w_load_report_tmp[i].w_cpu_uti_1;
					w_load_report[i].w_cpu_uti_2 = w_load_report_tmp[i].w_cpu_uti_2;	
					w_load_report[i].locked = w_load_report_tmp[i].locked;	
				}
								
				//printf("[%d]. LOAD_AGENT_RECV[%d]: %.6f\n", rank, w_load_report[0].w_load_no, MPI_Wtime() - load_agent_t1);
				//printf("[%d]. Load Overhead UPDATE: %.6f \n", rank, (MPI_Wtime() - load_overhead_update));
				
				printWorkerLoadReport(w_load_report, numprocs);
			}
			///Result comes from a worker
			else if(w_msg_code == RESULTS_FROM_WORKER){
				
				//printf("[%d]. Master is ready to receive a result from %d...\n", rank, w);
				
				///Lock making any transactions between the master 
				///and the worker 'w'
				*(masterReceiving + w - 1) = 1;
				///Wait if the master sends messages to the worker 'w'
				while(*(masterSending + (w) - 1) == 1) usleep(1);
				///Send confirmation to the worker 'w' to process 
				///sending the result
				//printf("[%d]. Master is ready to receive a result from %d...\n", rank, w);
				sendRecvConfirmation(w);
				//printf("[%d]. Master is ready to receive a result from %d...\n", rank, w);
				
				///modify the load status for the sender;w: worker who 
				///finished executing the task
				modifyWorkerLoadReport(w, w_load_report, numprocs, 1);
				printWorkerLoadReport(w_load_report, numprocs);
				//printf("[%d]. Master is ready to receive a result from %d...\n", rank, w);
				
				processing_tasks--;
				
				///Receive the task results
				recvMobileTaskM(pool, w, w_msg_code);
				
				///Unlock the receiving from that worker 'w'
				*(masterReceiving + w - 1) = 0;
				
				///If there are tasks in the task pool
				if((m_t_r = getReadyTask(pool)) != NULL){					
					//getBestWorker(w_load_report, &dest_w, numprocs, 1);	
					//printf("-----dest_w: %d\n", dest_w);
					dest_w = w;
					//if(dest_w != -1)
					{
						sendMobileTaskM(m_t_r, dest_w);					
						modifyWorkerLoadReport(dest_w, w_load_report, numprocs, 0);
						printWorkerLoadReport(w_load_report, numprocs);	
						processing_tasks++;
					}
				}
				
			}else if (w_msg_code == MOBILITY_CONFIRMATION_FROM_WORKER){
				///Receive a confirmation from the workers who made the
				///movements to modify the worker status
				recvMobileConfirmationM(w_load_report, pool, w, w_msg_code);			
				printWorkerLoadReport(w_load_report, numprocs);
			}else if (w_msg_code == MOBILITY_NOTIFICATION_FROM_WORKER){
				///Receive moving operation occurs now
				recvMovingNotification(w_load_report, numprocs, w, w_msg_code);
			}			
		}while(processing_tasks > 0);	

		printf("Master sends termination...\n");	
		 
		terminateWorkers(numprocs);		
		
		//printTaskPool(pool);
		
		printf("Master prepares the output...\n");
		
		taskOutput(pool, output, outLen, outSize);
		
		free(pool);
		
		//printf("---------------------------------------------------\n");
		//printDuration2(st2 - st); 
		//printf("---------------------------------------------------\n");		
		end_time = MPI_Wtime();
		//printf("\nRunning Time = %f\n\n", end_time - start_time);
		printf("Total Time: %.5f\n", end_time - start_time);
		//
		printf("Master had finished his work...\n");		
	}
	else
	{
    
		//char *res4 = (char*)malloc(sizeof(char)*200);
		
		//char *command = (char*)malloc(sizeof(char)*200);
		
		//sprintf(command, "cat /proc/%u/task/%u/stat", getppid(), getpid());
		
		//systemCall(command, res4);
		//printf("[worker: %d] - {PID: %u}\n", rank, getpid());
	
		//mpicc multicore.c -o mat -lm -lpthread
		//mpirun -n 11 ./mat 8 20 1

		//fp1 = worker;
		
		printf("[%d]. Ready Worker start 1...\n", rank);
		
		
		
		int w = 0;
		int w_msg_code = 0;

		//struct mobile_task * w_mt;
		
		///Pointer for a mobile task 
		//w_mt = (struct mobile_task *)malloc(sizeof(struct mobile_task));	
		
		
		printf("[%d]. Ready Worker start 3...\n", rank);	
		
		///Linked list for holding the mobile task on current worker		
		w_tasks = (struct worker_task*)malloc(sizeof(struct worker_task));
		w_tasks->task_id = -1;	
		w_tasks->next = NULL;	
		
		if(isFirstCall){
			workers_load_report = (struct worker_load *)malloc(sizeof(struct worker_load) * (numprocs-1));
			
			w_l_t = (struct worker_load_task *)malloc(sizeof(struct worker_load_task));
			
			w_l_t->w_loads = workers_load_report;
			w_l_t->move_report = (struct worker_move_report *)malloc(sizeof(struct worker_move_report)*(numprocs-1));
			
			isFirstCall = 0;
		}		
		
		w_l_t->w_tasks = w_tasks;		
		w_l_t->hold.on_hold = 0;
		w_l_t->hold.holded_on = 0;
		w_l_t->hold.holded_from = 0;
		w_l_t->hold.hold_time = 0;				
		w_l_t->worker_tid = getpid();
		
		printf("[%d]. Ready Worker start...\n", rank);
		
		///Pointer to the shared data which will be accebile amongst all tasks
		void *shared_data = NULL;	
		//int w_shared_len = 0;	
		int w_shared_totalsize = 0;	
		
		do{
			//printf("----[%d]Worker Waiting...\n", rank);
			///Receive the message code from the master or from the  
			///source worker who wants to send the task to it.
			recvMsgCode(&w, &w_msg_code);
			
			printf("----[%d]Worker Received somethng from %d, code: %d (T: %f)...\n", rank, w, w_msg_code, MPI_Wtime());
			///Run the worker laod agent
			if(w_msg_code == LOAD_REQUEST_FROM_MASTER){						

				pthread_create(&w_load_pth, NULL, worker_status, w_l_t);		
				
				//if(mobility == 1)
				//	pthread_create(&w_estimator_pth, NULL, worker_estimator, w_l_t);
			}
			///Terminate the worker
			else if(w_msg_code == TERMINATE_THE_WORKER){
				printf("[%d]. TERMINATE_THE_WORKER.\n", rank);
				break;
			}
			///receive the load information about other worker from 
			///the master
			else if(w_msg_code == LOAD_INFO_FROM_MASTER){
				recvWorkerLoadReport(w_l_t->w_loads, w, w_msg_code);
				
				printf("[%d] The worker receive the latest load info from the master...\n", rank);
	
				printWorkerLoadReport(w_l_t->w_loads, numprocs);
				
				if(mobility == 1)
					pthread_create(&w_estimator_pth, NULL, worker_estimator, w_l_t);
			}
			///Receive a confirmation from the master to procceed 
			///sending the results to it. That means locking the 
			/// sending from the worker to the master
			else if(w_msg_code == SENDING_CONFIRMATION_FROM_MASTER){
				worker_sending = 1;
			}
			///Receive the move command from the destination worker
			else if(w_msg_code == MOBILITY_ACCEPTANCE_FROM_WORKER){				
				//int move_command[2];
				//MPI_Recv(move_command, 2, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);				
				//printf("[%d]Move command from the master received(Move task %d from me to %d)...\n", rank, move_command[0], move_command[1]);
				int num_tasks = 0;
				MPI_Recv(&num_tasks, 1, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);				
				//printf("[%d] %d tasks are permitted to move from %d to %d, %d\n", rank, num_tasks, rank, w, w_l_t->move_report);
				printf("[%d] %d tasks are permitted to move from %d to %d\n", rank, num_tasks, rank, w);
				if(num_tasks > 0){
					if(w_l_t->move_report != NULL){
						int cur_w_id = (w_l_t->move_report + w - 1)->w_id;
						int cur_num_of_tasks = (w_l_t->move_report + w - 1)->num_of_tasks;
						int * cur_list_of_tasks = (w_l_t->move_report + w - 1)->list_of_tasks;
						printf("[%d] CONFIRMATION. w_id: %d, w_num: %d\n", rank, cur_w_id, cur_num_of_tasks);
						if(cur_num_of_tasks > 0 && cur_num_of_tasks >= num_tasks)
							if(cur_list_of_tasks != NULL){
								int i_tt = 0;
								for(i_tt=0;i_tt<num_tasks; i_tt++){
									printf("%d\n", *(cur_list_of_tasks + i_tt));
									struct worker_task * wT = w_tasks->next;
									for(;wT!=0;wT=wT->next){
										if((*(cur_list_of_tasks + i_tt) == wT->task_id) && (wT->move_status != 1)){
											wT->move = 1;
											wT->go_to = w;
											//printf("[%d]=-=- Move Task %d FROM -%d- to -%d-(wT->moving_pth: %u)\n", rank, wT->task_id, rank, w, wT->moving_pth);
											printf("[%d]=-=- Move Task %d FROM -%d- to -%d-(wT->moving_pth: %zu)\n", rank, wT->task_id, rank, w, wT->moving_pth);
											if(wT->moving_pth == 0)
												pthread_create(&wT->moving_pth, NULL, move_mobile_task, wT);
											break;
										}
										//if((move_command[0] == wT->task_id) && (wT->move_status != 1)){
											//wT->move = 1;
											//wT->go_to = move_command[1];
											
											//pthread_create(&wT->moving_pth, NULL, move_mobile_task, wT);						
											//move_mobile_task(wT);
											//break;
										//}
									}
								}
							}
					}					
				}
				//The dest worker ignore my request
				else{
					printWorkerLoadReport(w_l_t->w_loads, numprocs);
				
					///if(mobility == 1)
					///	pthread_create(&w_estimator_pth, NULL, worker_estimator, w_l_t);
				}
				/*
				struct worker_task * wT = w_tasks->next;
				for(;wT!=0;wT=wT->next){
					//if((move_command[0] == wT->task_id) && (wT->move_status != 1)){
						//wT->move = 1;
						//wT->go_to = move_command[1];
						
						//pthread_create(&wT->moving_pth, NULL, move_mobile_task, wT);						
						//move_mobile_task(wT);
						//break;
					//}
				}		*/		
			}
			///Update the worker loads and add my load and then circulate to the next worker
			else if(w_msg_code == UPDATE_LOAD_REPORT_REQUEST){
				//double load_agent_t2 = MPI_Wtime();
				//double load_overhead_append = MPI_Wtime();
					
				//int source_worker = 0;
				//MPI_Recv(&source_worker, 1, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);
				//printf("[%d]Waiting for starting moving...(Move task from %d to me)...\n", rank, source_worker);
				recvWorkerLoadReport(w_l_t->w_loads, w, w_msg_code);
				//w_l_t->w_loads[rank-1] = w_l_t->w_local_loads;
				
				//printf("[%d]. printWorkerLoadReport. before \n", rank);
				//printWorkerLoadReport(w_l_t->w_loads, numprocs);
				
				//printWorkerLoad(w_l_t->w_local_loads);
				
				w_l_t->w_loads[rank-1].w_load_no++;
				w_l_t->w_loads[rank-1].w_load_avg_1 = w_l_t->w_local_loads.w_load_avg_1;
				w_l_t->w_loads[rank-1].w_load_avg_2 = w_l_t->w_local_loads.w_load_avg_2;
				w_l_t->w_loads[rank-1].estimated_load = w_l_t->w_local_loads.estimated_load;
				w_l_t->w_loads[rank-1].w_running_procs = w_l_t->w_local_loads.w_running_procs;
				w_l_t->w_loads[rank-1].w_cpu_uti_1 = w_l_t->w_local_loads.w_cpu_uti_1;
				w_l_t->w_loads[rank-1].w_cpu_uti_2 = w_l_t->w_local_loads.w_cpu_uti_2;
				w_l_t->w_loads[rank-1].locked = w_l_t->w_local_loads.locked;
				
				//printf("[%d]. printWorkerLoadReport. after \n", rank);
				//printWorkerLoadReport(w_l_t->w_loads, numprocs);
				
				//printf("[%d]. w_l_t->w_loads: %p, workers_load_report: %p\n", rank,w_l_t->w_loads,workers_load_report);
				//if(rank+1 == numprocs)
				//	circulateWorkerLoadReport(workers_load_report, 0, numprocs);
				//else
				if(rank+1 < numprocs)
					circulateWorkerLoadReport(workers_load_report, rank+1, numprocs);
				else	
					circulateWorkerLoadReport(workers_load_report, 0, numprocs);
				//printf("[%d]. WORKER_LOAD_AGENT: %.6f\n", rank, MPI_Wtime() - load_agent_t2);
				//printf("[%d]. Load Overhead APPENDING: %.6f \n", rank, (MPI_Wtime() - load_overhead_append));
			}
			///Start executing the task from the master
			else if(w_msg_code == TASK_FROM_MASTER){
				double task_net_t = MPI_Wtime();
				//printf("[%d]. task_net_t: %f\n", rank, task_net_t);
				
				printf("[%d]. Worker is receiving a task...\n", rank);
				struct worker_task * w_task = (struct worker_task*)malloc(sizeof(struct worker_task));
				w_task->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));
				
				//printf("[%d]. Worker is receiving a task...\n", rank);
				
				w_task->m_task->move_stats.start_move_time = MPI_Wtime();				
				
				recvMobileTask(w_task->m_task, w, M_TO_W, w_msg_code);
				
				printf("[%d]. Worker finished receiving a task...\n", rank);
				
				//printf("[%d]. task_net_t: %f\n", rank, MPI_Wtime() - task_net_t);
				
				//printf("[%d]. start_move_time: %f\n", rank, w_task->m_task->move_stats.start_move_time);
				
				w_task->m_task->move_stats.net_time = task_net_t - w_task->m_task->move_stats.start_move_time;				
				
				//PRINT MOVE TIMES
				//printf("[%d]. Send time: %f, recv ping time: %f, recv task time: %f, R: %.2f\n", rank, w_task->m_task->task_move_time, w_task->m_task->task_ping_time, MPI_Wtime(), w_task->m_task->task_move_R);
				//printf("[%d]. RECV ping time: %f, recv task time: %f, R: %.2f\n", rank, w_task->m_task->task_ping_time - w_task->m_task->task_move_time, MPI_Wtime() - w_task->m_task->task_move_time, w_task->m_task->task_move_R);
				//
				//calculatingMoveTime(w_task->m_task, w_l_t->w_local_loads);
				
				//printf("[%d]. tafter calculatingMoveTime: %f\n", rank, w_task->m_task->move_stats.end_move_time - w_task->m_task->move_stats.start_move_time);
				
				w_task->m_task->task_fun = worker;
				
				//shared_data	
				/*			
				w_task->m_task->shared = malloc(shared_data_len * shared_data_size);				
				w_task->m_task->shared_len = shared_data_len;
				w_task->m_task->shared_unit_size = shared_data_size;
				int sh_i = 0;
				for(sh_i = 0; sh_i < shared_data_len * shared_data_size; sh_i++){
						*((char*)(w_task->m_task->shared + sh_i)) = *((char*)(shared_data + sh_i));
				}
				*/				
				//
				//Set Shared values				
				if(shared_data != NULL){
					w_task->m_task->shared = shared_data;
					w_task->m_task->shared_len = w_shared_totalsize/shared_data_size;
					w_task->m_task->shared_unit_size = shared_data_size;	
				}
								
				w_task->task_id = w_task->m_task->m_task_id;
				
				printf("[%d]. Ready[id: %d]...\n", rank, w_task->task_id);
				
				w_task->w_task_start = MPI_Wtime();
				w_task->w_l_load = NULL;
				w_task->move = 0;
				w_task->go_move = 0;
				w_task->go_to = 0;
				w_task->move_status = 0;
				w_task->estimating_move = NULL;
				w_task->moving_pth = 0;
				w_task->local_R = 0;
				
				printf("[%d]. Ready...\n", rank);
				
				printf("[%d]. Ready...[%p]\n", rank, w_tasks);

				w_tasks = newWorkerTask(w_tasks, w_task);	
				
				printf("[%d]. Ready...\n", rank);
				
				//Set Thread affinity for each task
				//cpu_set_t cpuset;

				//CPU_ZERO(&cpuset);
				//CPU_SET((w_task->task_id % 8), &cpuset);
				/*
				#define handle_error_en(en, msg) \
				do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
               */
               
               printf("[%d] Ready to create a thread for the task: %d\n", rank, w_task->task_id);	

				pthread_create(&w_task->task_pth, NULL, workerMobileTask, w_task->m_task);	
				/*
				int s = pthread_setaffinity_np(w_task->task_pth, sizeof(cpu_set_t), &cpuset);
				if (s != 0)
					printf("pthread_setaffinity_np\n");	
					*/
				printf("[%d] creating a thread with: %lu\n", rank, w_task->task_pth);			
			}else if(w_msg_code == MOBILITY_REQUEST_FROM_WORKER){
				//int num_tasks = -1;
				//MPI_Recv(&num_tasks, 1, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);				
				int*msg_numTasks_load_no = (int*)malloc(sizeof(int)*2);
				MPI_Recv(msg_numTasks_load_no, 2, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);
								
				printWorkerLoadReport(w_l_t->w_loads, numprocs);
				
				printf("[%d]Receiving a load report from worker: %d with moving %d tasks (LOAD_NO: R: %d, L: %d)...\n", rank, w, *msg_numTasks_load_no, *(msg_numTasks_load_no+1), w_l_t->w_loads[rank-1].w_load_no);	
											
				int num_task = *msg_numTasks_load_no;
				
				//sendMobileConfirmationToWorker(w, num_task);

				if(w_l_t->hold.on_hold == 0){
					printf("[%d]--- requested tasks: %d, permitted_tasks: %d\n", rank, num_task, num_task);
					w_l_t->hold.on_hold = 1;
					w_l_t->hold.holded_on = num_task;
					w_l_t->hold.holded_from = w;
					w_l_t->hold.hold_time = MPI_Wtime();
					w_l_t->w_local_loads.locked = 1;					
					
					//sendMobileConfirmationToWorker(w_l_t->w_local_loads, w, num_tasks);
					//sendMobileConfirmationToWorker(w_l_t->w_local_loads, w, *(msg_numTasks_load_no), *(msg_numTasks_load_no+1));
					sendMobileConfirmationToWorker(w, num_task);
				}else{
					printf("[%d]--- requested ignored...\n", rank);
					sleep(1);
					sendMobileConfirmationToWorker(w, 0);
				}
			}
			///Confirmation upon successfully receiving the mobile task
			else if(w_msg_code == MOBILITY_CONFIRMATION_FROM_WORKER){
				int t_id = -1;
				MPI_Recv(&t_id, 1, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);				
				printf("[%d]Receiving a confirmation for moving the task %d...\n", rank, t_id);
				
				//Update the load no on local
				w_l_t->w_loads[rank-1].w_load_no++;

				struct worker_task * wT = w_tasks->next;
				for(;wT!=0;wT=wT->next){
					if(t_id == wT->task_id){
						if(wT->move == 1 && wT->go_move == 1 && wT->move_status == 0){
							wT->move_status = 1;							
							break;
						}
					}
				}				
			}
			///Receive the task for the source worker
			else if(w_msg_code == TASK_FROM_WORKER){
				printf("[%d] Starting receiving the task from %d...\n", rank, w);
					
				struct worker_task * w_task = (struct worker_task*)malloc(sizeof(struct worker_task));
				w_task->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));
				
				double task_net_t = MPI_Wtime();
				
				recvMobileTask(w_task->m_task, w, W_TO_W, w_msg_code);
				
				printf("[%d]. task_net_t: %f\n", rank, MPI_Wtime() - task_net_t);
				
				printf("[%d] Receiving mobile task succefully from %d...(TIME: %f)\n", rank, w, MPI_Wtime());
				
				w_task->m_task->task_fun = worker;
				//
				w_task->m_task->shared = shared_data;
									
				w_task->task_id = w_task->m_task->m_task_id;
				w_task->w_task_start = MPI_Wtime();
				w_task->w_l_load = NULL;
				w_task->move = 0;
				w_task->go_move = 0;
				w_task->go_to = 0;
				w_task->move_status = 0;	
				w_task->estimating_move = NULL;		
				w_task->moving_pth = 0;	
				w_task->local_R = 0;

				w_tasks = newWorkerTask(w_tasks, w_task);	
				
				///printMobileTask(w_task->m_task);				
				
				//Sending a confirmation to the source worker...
				sendMobileConfirmation(w_task->task_id, w, 0);
				//Update the load no on local
				w_l_t->w_loads[rank-1].w_load_no++;
				w_l_t->hold.holded_on--;
				printf("[%d] hold info: %d, %d\n", rank, w_l_t->hold.on_hold, w_l_t->hold.holded_on);
				if(w_l_t->hold.holded_on == 0){
					w_l_t->hold.on_hold = 0;
					w_l_t->hold.holded_on = 0;
					w_l_t->hold.hold_time = 0;
					w_l_t->w_local_loads.locked = 0;
				}
				pthread_create(&w_task->task_pth, NULL, workerMobileTask, w_task->m_task);	
				//printf("[%d] The moved task is starting at %f\n", rank, MPI_Wtime());
				//printf("[%d] creating a thread for : %zu\n", rank, w_task->task_pth);

				//sleep(5);
			}else if(w_msg_code == SHARED_DATA_FROM_MASTER){	
				//printf("[%d]. Ready B shared_data: %p\n", rank, shared_data);	
				shared_data = recvSharedData(shared_data, &w_shared_totalsize, w, w_msg_code);
				//printf("[%d]. Ready F shared_data: %p\n", rank, shared_data);
			}
		}while(w_msg_code != 2);
		
		/*
		int tasks_111 =0;
		struct worker_task * wT = w_tasks->next;
		for(;wT!=0;wT=wT->next){
			tasks_111++;
		}
		printf("[%d]. Ready tasks_111: %d, w_tasks: %lx\n", rank, tasks_111, w_tasks);
		//free(w_tasks);
		tasks_111 =0;
		wT = w_tasks->next;
		struct worker_task * wT_tmp = wT;
		for(;wT!=0;){
			tasks_111++;
			
			wT_tmp = wT;			
			free(wT_tmp->w_l_load);
			free(wT_tmp->m_task);
			free(wT_tmp->estimating_move);
			wT=wT->next;
			free(wT_tmp);
		}
		free(w_tasks->next);
		w_tasks->next = NULL;
		printf("[%d]. Ready tasks_1112: %d, Ready shared_data: %p\n", rank, tasks_111, shared_data);
		free(shared_data);
		*/
		//printf("[%d]. Ready tasks_1113: %d, Ready shared_data: %p\n", rank, tasks_111, shared_data);
		
		//printf("[%d]. Ready tasks_1114: %d, Ready shared_data: %p\n", rank, tasks_111, shared_data);
		//free(workers_load_report);
		
		//w_tasks->next = NULL;
		//free(w_l_t);
	}
	
	printf("Worker/Master[%d] is at the barrier\n",rank);
	
	MPI_Barrier(MPI_COMM_WORLD);	
		
	printf("Worker/Master[%d] is finished...\n",rank);
	///sleep(5);	
}
