/***********************************************************************
*          HWFarm Skeleton using C, MPI and PThread.
*             Turkey Alsalkini & Greg Michaelson
*      Heriot-Watt University, Edinburgh, United Kingdom.
***********************************************************************/
#define _GNU_SOURCE
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif 

#include <errno.h>
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
/******************** COMMUNICATION TAGS *******************************/
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

/******************** SKELETON CONSTANTS ******************************/
#define MSG_LIMIT 400000000
#define ESTIMATOR_BREAK_TIME 3
#define NET_LAT_THRESHOLD 1.5
#define MAX_MOVES 20       //MAX_MOVES
typedef enum {M_TO_W, W_TO_W, W_TO_M} sendType;
/******************** GLOBAL VARIABLES ********************************/
double start_time;         //holds the start time
double end_time;           //holds the end time
int isFirstCall = 1;       //@Master to check if hte skeleton is called 
float Master_FREQ = 0.0;   //Current Node Characterstics
int currentCores = 0;      //Current Node Characterstics
int moving_task = 0;       //The worker is sending a task now
int worker_sending = 0;    //The worker is moving a task now
int *masterReceiving;      //To avoid collision at the master
int *masterSending;        //To avoid collision at the master
/****************** MPI GLOBAL VARIABLES ******************************/
MPI_Status status; // store status of a MPI_Recv
MPI_Request request; //capture request of a MPI_Isend

/***************** POSIX GLOBAL VARIABLES *****************************/
//@Master
pthread_t w_load_report_th; 
pthread_t w_network_latency_th; 
//@Worker 
//Thread to run the worker load agent which collects the  
//worker load and sends it to the master
pthread_t w_load_pth;
//Thread to run the worker estimator which is responsible 
//for estimating the estimated execution time for the tasks
//running on the this worker. Then it will send the 
//move reort to the server to make the moving if necessary.
pthread_t w_estimator_pth;
//To prevent locking
pthread_mutex_t mutex_load_circulating = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_w_sending = PTHREAD_MUTEX_INITIALIZER;

/****************** SKELETON DATA STRUCTURES **************************/
//stats of the moving task to predict the mobility cost
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
   int m_task_id;                     //tag for task index
   void * input_data;                 //The input data buffer
   int input_data_length;             //The input data length
   int input_data_item_size;          //The input data unit size   
   void * shared_data;                //The shared data buffer
   int shared_data_length;            //The shared data length
   int shared_data_item_size;         //The shared data unit size
   void * output_data;                //The output data buffer
   int output_data_length;            //The output data length
   int output_data_item_size;         //The output data unit size
   int counter;                       //The main index iterations
   int counter_max;                   //The total number of iterations for one task
   void * state_data;                 //The state buffer
   int state_data_length;               //The state length
   long shift;                        //shift value from the start
   int moves;                         //The numbers of moves for this task
   int m_dest[MAX_MOVES];             //the workers who processed the task
   double m_start_time[MAX_MOVES];    //the start times at the workers who processed the task
   double m_end_time[MAX_MOVES];      //the end times at the workers who processed the task
   float m_avg_power[MAX_MOVES];      //The average computing power when the task leave the machine
   float m_work_start[MAX_MOVES];     //The start work when the task arrives
   int moving;                        //label for checking the task movement state
   int done;                          //label if the task is completed
   struct task_move_stats move_stats;		
   fp *task_fun;                      //pointer to the task function
};

//data structure to store the run-time task information at the master
struct mobile_task_report{
   int task_id;                       //Task No(ID)
   int task_status;	                  //Current task status(0: waiting; 1: on processing; 2: completed; 3: on move )
   double task_start;                 //The time of start execution
   double task_end;                   //The time of end execution
   int task_worker;                   //The current worker where this task runs
   int mobilities;                    //Number of movements for the task
   double m_dep_time[MAX_MOVES];      //The departure time from the source worker
   double m_arr_time[MAX_MOVES];      //The arrival time to the destination worker
   struct mobile_task * m_task;       //Next record
};
//data structure to store all tasks at the master
struct task_pool{
   struct mobile_task_report * m_task_report;
   struct task_pool * next;
};
//data structure to store a history of the laod state while executing this task
struct worker_local_load{
   float per;
   float sec;
   float load_avg;
   int est_load_avg;
   long double w_cpu_uti;
   int w_running_procs;
   struct worker_local_load*  next;
};
//data structure to store details of this task on the worker
struct worker_task{
   int task_id;	
   double w_task_start;						
   double w_task_end;						
   pthread_t task_pth;
   pthread_t moving_pth;
   struct mobile_task * m_task;   
   float local_R;
   int move;
   int go_move;
   int go_to;
   int move_status;
   struct worker_local_load * w_l_load;
   struct estimation * estimating_move;
   struct worker_task * next;
};
//data structure to store a the network delays at allocation time and the current time
struct network_times{
   double init_net_time;
   double cur_net_time;
};
//data structure to store the load state of all workers
struct worker_load{
   int w_rank;
   char w_name[MPI_MAX_PROCESSOR_NAME];
   int m_id;	
   int current_tasks;
   int total_task;
   int status;          //0:free, 1:working, 2: Requsting, 3: waiting, 4: envolved in moving
   int w_cores;		    //Static metric
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
//data structure to store the move report issued by the estimator agent
struct worker_move_report{
   int w_id;
   int num_of_tasks;
   int * list_of_tasks;
};
//data structure to store details when a worker becomes busy in receiving a task from a worker
struct worker_hold{
   int on_hold;          //set to indicate that this worker is on hold to complete the move from the source worker
   int holded_on;        //number of tasks which the worker is waiting for
   int holded_from;      //the worker who i am holded to
   float hold_time;      //the time of hold. for cancelation if the request timed out
};
//data structure to store refrences fro other structures in the worker
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
//data structure to store the report of the estimated times
struct estimation{
   float * estimation_costs;
   int chosen_dest;
   float gain_perc;
   int done;
   int on_dest_recalc;
};
//data structure to store the estimated times on remote workers
struct other_estimated_costs{
   int w_no;
   float * costs;
   float move_cost;
};
//data structure to store detailed report of estimated times for this task
struct estimated_cost{
   int task_no;
   float cur_EC;
   float spent_here;
   float *cur_EC_after;
   struct other_estimated_costs* other_ECs;
   int to_w;
   float to_EC;
};

/******************** GLOBAL DATA STRUCTURES **************************/
///-----Worker-------
struct worker_load_task * w_l_t = NULL;
struct worker_load * workers_load_report = NULL;
struct worker_task * w_tasks;
///-----Master-------
struct worker_load * w_load_report = 0;
struct worker_load * w_load_report_tmp = 0;

/********************** LINUX/OS FUNCTIONS ****************************/

void systemCall(char* b,char* res){
   FILE *fp1;
   char *line = (char*)malloc(130 * sizeof(char));
   fp1 = popen(b, "r");  
   if(fp1 == 0)
      perror(b);
   res[0]='\0';
   while ( fgets( line, 130, fp1))
		strcat(res, line);
   res[strlen(res)-1]='\0'; 
   pclose(fp1); 
   free(line); 
   return;
}
void systemCallNoRes(char* b){
   //Execute System call
   FILE *fp;
   fp = popen(b, "r");   
   pclose(fp);
}    
//send ping message to the selected worker
double sendPing(char * node_name){	
   char * pre_ping_command = "ping %s -c 1 | grep \"rtt\" | awk '{split($0,a,\" \"); print a[4]}' | awk '{split($0,b,\"/\"); print b[2]}'";	
   char *ping_command = (char*)malloc(sizeof(char)*200);	
   sprintf(ping_command, pre_ping_command, node_name);
   char * rtt_latency_str = (char*)malloc(sizeof(char)*20);
   systemCall(ping_command, rtt_latency_str);
   double net_latency = atof(rtt_latency_str);	
   if(net_latency != 0.0){		
      free(ping_command);
      free(rtt_latency_str);
      return net_latency;
   }else{		
      free(ping_command);
      free(rtt_latency_str);
      return 0.0;
	}		
}

/******************** LOAD STATE FUNCTIONS ****************************/

int contains(char target[],int m, char source[] ,int n){
   int i,j=0;
   for(i=0;i<n;i++){		
      if(target[j] == source[i])j++;		
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
   float total_cores_freq = 0;

   f = fopen("/proc/cpuinfo","r");	
   if(f == 0){
      perror("Could open the file :/proc/cpuinfo");
      return 0;
   }

   float core_freq = 0, min_core_freq = 0;
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
      }
   }		
   fclose(f);
   free(tmp);
   free(line);
   return min_core_freq * cores_cnt;
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
      if (nprocs < 1){
         fprintf(stderr, "Could not determine number of CPUs online:\n%s\n", 
         strerror (errno));
         return nprocs;
      }
      nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
      if (nprocs_max < 1){
         fprintf(stderr, "Could not determine number of CPUs configured:\n%s\n", 
         strerror (errno));
         return nprocs;
      }
      return nprocs;
   #else
      fprintf(stderr, "Could not determine number of CPUs");
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
   if(f == 0)
      return 0;
   int state = 0, i = 0;
   do{
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
   long double user = b[0] - a[0];
   long double nice = b[1] - a[1];
   long double system = b[2] - a[2];
   long double idle = b[3] - a[3];
   long double wa = b[4] - a[4];
   long double total = user + nice + system + idle + wa;
   long double per = 0.0;
   if(total != 0.0)		
      per = (( user + nice + system) * 100) / total;
   return per;
}

float getLoad(){
   struct sysinfo sys_info;
   if(sysinfo(&sys_info) != 0)
      perror("sysinfo");		
   return sys_info.loads[0]/65536.0;
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
   }			
	
   *per = tmp_per / READ_COUNT;
   *rp = tmp_rp / READ_COUNT;	
   load_overhead_1 = MPI_Wtime();
   *load_avg = getLoad();
   load_overhead_2 = MPI_Wtime();
}

/********************** SKELETON FUNCTIONS ****************************/
struct task_pool * addTasktoPool(
    struct task_pool * pool, int task_no, void * input_data, int input_data_len, 
    int input_data_unit_size, void * output_data, int output_data_len, 
    int output_data_unit_size, int tag, int shift, void * state, 
    int state_size, int main_index, int main_index_max){
					
   struct task_pool * pl = pool;	
   if(pool == 0){
      pool = (struct task_pool *)malloc(sizeof(struct task_pool));
      pool->m_task_report = (struct mobile_task_report *)malloc(sizeof(struct mobile_task_report));
      pool->m_task_report->task_id = task_no;
      pool->m_task_report->task_status = 0;
      pool->m_task_report->task_start = 0;
      pool->m_task_report->task_end = 0;
      pool->m_task_report->task_worker = 0;
      pool->m_task_report->mobilities = 0;		
      pool->m_task_report->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));
      if(input_data != NULL){
         pool->m_task_report->m_task->input_data = input_data + shift;
         pool->m_task_report->m_task->input_data_length = input_data_len;
         pool->m_task_report->m_task->input_data_item_size = input_data_unit_size;
      }
      pool->m_task_report->m_task->output_data = output_data;
      pool->m_task_report->m_task->output_data_length = output_data_len;
      pool->m_task_report->m_task->output_data_item_size = output_data_unit_size;
      pool->m_task_report->m_task->m_task_id = tag;
      pool->m_task_report->m_task->shift = shift;
      pool->m_task_report->m_task->state_data = state;
      pool->m_task_report->m_task->state_data_length = state_size;
      pool->m_task_report->m_task->counter = main_index;
      pool->m_task_report->m_task->counter_max = main_index_max;
      pool->m_task_report->m_task->moves = 0;		
      pool->m_task_report->m_task->done = 0;		
      pool->m_task_report->m_task->moving = 0;		
      pool->next = 0;		
   }else{
      while(pl->next != 0)
         pl = pl->next;		

      pl->next = (struct task_pool *)malloc(sizeof(struct task_pool));
      pl = pl->next;		
      pl->m_task_report = (struct mobile_task_report *)malloc(sizeof(struct mobile_task_report));
      pl->m_task_report->task_id = task_no;
      pl->m_task_report->task_status = 0;
      pl->m_task_report->task_start = 0;
      pl->m_task_report->task_end = 0;
      pl->m_task_report->task_worker = 0;
      pl->m_task_report->mobilities = 0;		
      pl->m_task_report->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));
      if(input_data != NULL){
         pl->m_task_report->m_task->input_data = input_data + shift;
         pl->m_task_report->m_task->input_data_length = input_data_len;
         pl->m_task_report->m_task->input_data_item_size = input_data_unit_size;
      }
      pl->m_task_report->m_task->output_data = output_data;
      pl->m_task_report->m_task->output_data_length = output_data_len;
      pl->m_task_report->m_task->output_data_item_size = output_data_unit_size;
      pl->m_task_report->m_task->m_task_id = tag;
      pl->m_task_report->m_task->shift = shift;
      pl->m_task_report->m_task->state_data = state;
      pl->m_task_report->m_task->state_data_length = state_size;		
      pl->m_task_report->m_task->counter = main_index;
      pl->m_task_report->m_task->counter_max = main_index_max;
      pl->m_task_report->m_task->moves = 0;		
      pl->m_task_report->m_task->done = 0;		
      (pl->m_task_report->m_task->moving) = 0;		
      pl->next = 0;		
   }	
   return pool;
}

struct task_pool * create_task_pool(
     int tasks, void* input, int input_len, int input_size, void* output, 
     int output_len, int output_size,void* state, int state_size, 
     int main_index, int chunk_size){	
		 
   int task_i = 0;
   int task_shift = 0;	
   struct task_pool * pool = 0;
   while(task_i < tasks){
      task_shift = (task_i * input_len) * input_size;
      pool = addTasktoPool(pool, task_i, input, input_len, input_size, 
           output, output_len, output_size, task_i, task_shift, 
           state, state_size, main_index, chunk_size);
      task_i++;
   }	
   return pool;	
}

///This function is used to check the mobility and perform a checkpointing
///if there is a need to transfer this computation
void checkForMobility(){
   pthread_t pth_id = pthread_self();
   if(w_tasks == NULL) return;
   struct worker_task * wT = w_tasks->next;
   for(;wT!=0;wT=wT->next){
      if(pth_id == wT->task_pth)
         if((wT->move == 1) && (moving_task == 0)){	
            wT->go_move = 1;				
            while(wT->move_status == 0) usleep(1);	
            if(wT->move_status == 1)
					pthread_exit(NULL);
         }
   }
}

void printMobileTask(struct mobile_task* mt){
   printf("Task id: %d @ %d\n-----------------------------------------------\n", mt->m_task_id, rank);
   printf("Input:  (len: %d) - (u_size: %d)\n", mt->input_data_length, mt->input_data_item_size);
   printf("Output: (len: %d) - (u_size: %d)\n", mt->output_data_length, mt->output_data_item_size);
   printf("State: (u_size: %d)\n", mt->state_data_length);
   printf("Main Counter: (init: %d) - (max: %d)\n", mt->counter, mt->counter_max);

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
   int msgCount = (dataLen/limit);
   if((dataLen % limit) != 0) msgCount++;
   int msgSize;
   int i=0;
   MPI_Ssend(&msgCount, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
   for(i = 0; i<msgCount; i++){
      if(dataLen < limit)
         msgSize = dataLen;
      else
         msgSize = limit;		
      MPI_Ssend(input + (i * limit), msgSize, MPI_CHAR, proc, tag + i + 1, MPI_COMM_WORLD);
      dataLen = dataLen - msgSize;
   }
}

void recvMobileMultiMsgs(void * input, int dataLen, int limit, int source, int tag){	
   if(dataLen == 0) return;	
   int msgSize ;
   int msgCount;
   int i=0;
   MPI_Recv(&msgCount, 1 , MPI_INT, source, tag, MPI_COMM_WORLD, &status);	
   for(i = 0; i<msgCount; i++){
      if(dataLen < limit)
         msgSize = dataLen;
      else
         msgSize = limit;
      MPI_Recv(input + (i * limit), msgSize, MPI_CHAR, source, tag + i + 1, MPI_COMM_WORLD, &status);
      dataLen = dataLen - msgSize;
   }
}
 
int getTaskResultSize(struct mobile_task* w_mt){
   int task_struct_size = sizeof(struct mobile_task);
   int task_output_size = w_mt->input_data_length*w_mt->input_data_item_size;	
   return task_struct_size + task_output_size;
}
 
int getInitialTaskSize(struct mobile_task* w_mt){
   int task_struct_size = sizeof(struct mobile_task);
   int task_input_size = w_mt->input_data_length*w_mt->input_data_item_size;
   int task_state_size = w_mt->state_data_length;	
   return task_struct_size + task_input_size + task_state_size;
}

int getTotalTaskSize(struct mobile_task* w_mt){
   int task_output_size = w_mt->output_data_length*w_mt->output_data_item_size;
   int task_init_size = getInitialTaskSize(w_mt);
   return task_output_size + task_init_size;
}

void sendMobileTask(struct mobile_task* mt, int w, sendType t){
   int send_code = 0;

   if(t == W_TO_W)
      send_code = TASK_FROM_WORKER;
   else if(M_TO_W)
      send_code = TASK_FROM_MASTER;
   else if(W_TO_M)
      send_code = RESULTS_FROM_WORKER;
		
   if(t == W_TO_M)
      worker_sending = 0;	
		
   MPI_Ssend(&send_code, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
	
   if(t == W_TO_M)
      while(worker_sending == 0) usleep(1);
	
   if(t == M_TO_W){
      while(*(masterReceiving + (w) - 1) == 1) usleep(1);
      *(masterSending + w - 1) = 1;
   }
		
   if(t == W_TO_M) 	
      MPI_Ssend(&(mt->m_task_id), 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
		
   MPI_Ssend(mt, sizeof(struct mobile_task), MPI_CHAR, w, send_code, MPI_COMM_WORLD);
	
   if(t != W_TO_M) 
      sendMobileMultiMsgs(mt->input_data, mt->input_data_length*mt->input_data_item_size, MSG_LIMIT, w, send_code);
   if(t != M_TO_W)
      sendMobileMultiMsgs(mt->output_data, mt->output_data_length*mt->output_data_item_size, MSG_LIMIT, w, send_code);		
   if(t != W_TO_M) 
      sendMobileMultiMsgs(mt->state_data, mt->state_data_length, MSG_LIMIT, w, send_code);
   if(t == M_TO_W)
      *(masterSending + w - 1) = 0;
}

void sendMobileTaskM(struct mobile_task_report* mtr, int w){
   mtr->m_task->move_stats.start_move_time = MPI_Wtime();
   mtr->m_task->move_stats.R_source = Master_FREQ;
   sendMobileTask(mtr->m_task, w, M_TO_W);
   mtr->task_status = 1;
   mtr->task_start = MPI_Wtime();
   mtr->task_worker = w;
}

void* recvSharedData(void* shared_data, int * w_shared_len, int source, int send_code){
   MPI_Status status;
   int shared_len;
   MPI_Recv(&shared_len, 1, MPI_INT, source, send_code, MPI_COMM_WORLD, &status);
   *w_shared_len = shared_len;
   if(shared_len != 0){
      shared_data = (void*)malloc(shared_len);
      recvMobileMultiMsgs(shared_data, shared_len, MSG_LIMIT, source, send_code);	
      return shared_data;
   }
   return NULL;
}

void recvMobileTask(struct mobile_task* w_mt, int source, sendType t, int send_code){	
   MPI_Status status;
   void * p_input;
   void * p_state;
   if(t == W_TO_M){
      p_input = w_mt->input_data;
      p_state = w_mt->state_data;
   }

   MPI_Recv(w_mt, sizeof(struct mobile_task), MPI_CHAR, source, send_code, MPI_COMM_WORLD,&status);
   
   if(t != W_TO_M){
      //Allocating & receiving input data
      w_mt->input_data = (void*)malloc(w_mt->input_data_length*w_mt->input_data_item_size);
      recvMobileMultiMsgs(w_mt->input_data, w_mt->input_data_length*w_mt->input_data_item_size, MSG_LIMIT, source, send_code);	
   }else
      w_mt->input_data = p_input;
   //Allocating & receiving output data
   w_mt->output_data = (void*)malloc(w_mt->output_data_length*w_mt->output_data_item_size);
   if(t != M_TO_W)
      recvMobileMultiMsgs(w_mt->output_data, w_mt->output_data_length*w_mt->output_data_item_size, MSG_LIMIT, source, send_code);	
   if(t != W_TO_M){
      //Allocating & receiving states data
      w_mt->state_data = (void*)malloc(w_mt->state_data_length);
      recvMobileMultiMsgs(w_mt->state_data, w_mt->state_data_length, MSG_LIMIT, source, send_code);
   }else
      w_mt->state_data = p_state;
	
   if(t != W_TO_M){
      w_mt->moves++;								
      w_mt->m_dest[w_mt->moves-1] = rank;
   }
}
//recevie results of the task
void recvMobileTaskM(struct task_pool* t_p, int w, int msg_code){
   int task_id = -1;
   MPI_Recv(&task_id, 1, MPI_INT, w, msg_code, MPI_COMM_WORLD, &status);	
   struct task_pool * p = t_p;
   while( p != NULL){
      if(p->m_task_report->task_id == task_id){
         recvMobileTask(p->m_task_report->m_task, w, W_TO_M, msg_code);
         p->m_task_report->task_status = 2;
         p->m_task_report->task_end = MPI_Wtime();
         p->m_task_report->task_worker = 0;
         break;
      }
      p = p->next;
   }
}

void recvMsgCode(int *recv_w,int *msg_code){
   MPI_Request req;
   int msgType = -1;
   int flag = 0;
   MPI_Status status;
   ///Non-Blocking MPI_Irecv
   MPI_Irecv(&msgType, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &req);
   do {		
      MPI_Test(&req, &flag, &status);
      usleep(10);	
   } while (flag != 1);
   *recv_w = status.MPI_SOURCE;
   *msg_code = status.MPI_TAG;
}

void *workerMobileTask(void *arg){	
   struct mobile_task *w_mt = ((struct mobile_task *)arg);		
   w_mt->m_start_time[w_mt->moves-1] = MPI_Wtime();	
   int i = w_mt->counter;
   float Wd_before = ((i * 100) / (float)w_mt->counter_max);
   w_mt->m_work_start[w_mt->moves-1] = Wd_before;	
	
   hwfarm_task_data * t_data = (hwfarm_task_data *)malloc(sizeof(hwfarm_task_data));
   t_data->input_data = w_mt->input_data;
   t_data->input_len = w_mt->input_data_length;
   t_data->shared_data = w_mt->shared_data;
   t_data->shared_len = w_mt->shared_data_length;
   t_data->state_data = w_mt->state_data;
   t_data->state_len = w_mt->state_data_length;
   t_data->output_data = w_mt->output_data;
   t_data->output_len = w_mt->output_data_length;
   t_data->counter = &w_mt->counter;
   t_data->counter_max = &w_mt->counter_max;
   t_data->task_id = w_mt->m_task_id;
	
   w_mt->task_fun( t_data, checkForMobility);
   w_mt->done = 1;
   w_mt->m_end_time[w_mt->moves-1] = MPI_Wtime();

   pthread_mutex_lock( &mutex_w_sending );	
   sendMobileTask(w_mt, 0, W_TO_M);	
   pthread_mutex_unlock( &mutex_w_sending );	
	
   free(w_mt->state_data);
   free(w_mt->output_data);
   free(w_mt->input_data);
	
   return NULL;
}

float getActualRunningprocessors(float cpu_uti, int est_load_avg, int running_processes, int cores){
   float np_per = -1;
   if(cpu_uti < 75)
      np_per = cpu_uti ;
   else{
      if(running_processes <= cores)
         np_per = cpu_uti;
      else
         np_per = (cpu_uti + ((running_processes) * 100) / (cores * 1.0)) / 2;
   }	
   float base = 100 / (cores * 1.0);
   int np = (int)(np_per / base);
   if(np < (np_per / base))
      np++;
   return (np_per/base);
}

float getActualRelativePower(float P, float cpu_uti, int est_load_avg, int running_processes, int cores, int added_np, int worker){
   float np_f = getActualRunningprocessors( cpu_uti,  est_load_avg,  running_processes, cores);		
   //Add/subtract the number of process 
   if(added_np != 0)
      np_f += added_np;
   float Rhn = P/np_f;
   float MAX_R = P/cores;
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
   int data_size_1 = w_mt->move_stats.data_size;
   double net_time_1 = w_l_t->w_loads->net_times.init_net_time;
   double move_time_1 = w_mt->move_stats.move_time;	
   int data_size_2 = getTotalTaskSize(w_mt);
   double net_time_2 = w_l_t->w_loads->net_times.cur_net_time;
   float R_2_s = getRForWorker(w_l_t, rank);
   float R_2_d = getRForWorker(w_l_t, worker);	
   float R_2 = R_2_s;
   if(R_2 > R_2_d)
      R_2 = R_2_d;	
   w_mt->move_stats.R_2 = R_2;
	
   float W_DS = 1.023;
   float W_R = 1.04;
   float W_L = 0.907;   
      
   double R_effect = pow((R_1 / R_2), W_R);
   double SD_effect = pow((1.0*data_size_2 / data_size_1), W_DS);  
   
   double net_time_mobility = (net_time_2 < NET_LAT_THRESHOLD) ? NET_LAT_THRESHOLD : net_time_2;
   double net_time_a = (net_time_1 < NET_LAT_THRESHOLD) ? NET_LAT_THRESHOLD : net_time_1;
   double Net_effect = pow((net_time_mobility / net_time_a), W_L);
   
   return (R_effect * SD_effect * Net_effect) * move_time_1;
}
	
void taskOutput(struct task_pool* t_p, void* output, int outLen, int outSize){	
   int task_i = 0, output_i = 0, task_output_i = 0;
   int output_shift;
   struct task_pool * p = t_p;
   while( p != NULL){
      output_shift = (task_i * outLen) * outSize;
      for(task_output_i = 0; task_output_i < outLen*outSize; task_output_i++)
         *((char*)output + output_i++) = *((char*)p->m_task_report->m_task->output_data + task_output_i);		
      p = p->next;
      task_i++;
   }		
}

void printWorkerLoadReport(struct worker_load * report, int n){
   int i;
   printf("---------------------------------------------------------------------------------------------------------\n");
   printf("[M/W]- W | no | ts | tot_t | Locked | s | cores | cache |  CPU Freq  | l.avg1 | l.avg2 | est | uti.1 | uti.2 | run_pro [%.3f] \n", (MPI_Wtime() - startTime));
   printf("---------------------------------------------------------------------------------------------------------\n");
   for(i=1; i<n; i++){		
      printf("[%d]- %3d | %2d | %2d |  %2d   | %c |  %c  |  %2d   |  %2d   | %.2f  | "
             " %2.2f  |  %2.2f  | %2d  | %3.2Lf  |  %3.2Lf |  %d\n", 
             rank, report[i-1].w_rank, report[i-1].m_id,
             report[i-1].current_tasks, report[i-1].total_task, (report[i-1].locked == 1 ? 'Y' : 'N'),
             ((report[i-1].status == 1) ? 'B' : ((report[i-1].status == 4) ? 'M' : 'F')), report[i-1].w_cores, report[i-1].w_cache_size,
             report[i-1].w_cpu_speed, report[i-1].w_load_avg_1, report[i-1].w_load_avg_2, report[i-1].estimated_load, 
             report[i-1].w_cpu_uti_1, report[i-1].w_cpu_uti_2, report[i-1].w_running_procs);
   }
   printf("---------------------------------------------------------------------------------------------------------\n");
}

void updateWorkerStatus(struct worker_load * report, int n, int worker, int new_status){
   int i;
   for(i=1; i<n; i++)		 
      if(report[i-1].w_rank == worker)
         report[i-1].status = new_status;		
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
   updateWorkerStatusMoving(report, n, w_source, *(data+1));
}

//Get the worker who will recieve the next task
void getValidWorker(int * tasksPerWorker, int n, int *w){	
   int w_i = 0;
   for(w_i = 0; w_i < n; w_i++)
      if(tasksPerWorker[w_i] > 0){
         tasksPerWorker[w_i]--;
         *w = w_i+1;
         return;
      }	
}

struct mobile_task_report * getReadyTask( struct task_pool* t_p){
   struct task_pool * p = t_p;
   while( p != NULL){
      if(p->m_task_report->task_status == 0)					
         return p->m_task_report;
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
   struct worker_task * p = w_t_header;
   while(p->next != NULL)
      p = p->next;	
   p->next = w_t;
   p->next->next = NULL;	
   return w_t_header;
}

void sendInitialWorkerLoad(struct worker_load * l_load, int master){
   int msg_code = INIT_LOAD_FROM_WORKER;
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
   MPI_Send(&msg_code, 1, MPI_INT, master, msg_code, MPI_COMM_WORLD);
}

void sendMoveReport(int* move_report, int report_size, int target){
   int msg_code = MOVE_REPORT_FROM_WORKER;
   MPI_Send(&msg_code, 1, MPI_INT, target, msg_code, MPI_COMM_WORLD);
   MPI_Send( &report_size, 1, MPI_INT, target, msg_code, MPI_COMM_WORLD);
   MPI_Send( move_report, report_size, MPI_INT, target, msg_code, MPI_COMM_WORLD);
}

void sendMoveReportToWorker(int target, int numTasks, int load_no){
   int msg_code = MOBILITY_REQUEST_FROM_WORKER;
   MPI_Send(&msg_code, 1, MPI_INT, target, msg_code, MPI_COMM_WORLD);
   int*msg_numTasks_load_no = (int*)malloc(sizeof(int)*2);
   *msg_numTasks_load_no = numTasks;
   *(msg_numTasks_load_no + 1) = load_no;
   MPI_Send( msg_numTasks_load_no, 2, MPI_INT, target, msg_code, MPI_COMM_WORLD);
   free(msg_numTasks_load_no);
}

void sendMoveReportToWorkers(struct worker_move_report * w_m_report, struct worker_load * report){
   int i_w = 0;
   for(;i_w<numprocs-1; i_w++){
      if(i_w != rank-1){
         if((w_m_report + i_w)->num_of_tasks > 0){		
            int w = (w_m_report + i_w)->w_id;						
            sendMoveReportToWorker((w_m_report + i_w)->w_id+1, (w_m_report + i_w)->num_of_tasks, (report + w)->m_id);
         }		
      }
   }
}

//type: 1 -- mobile request recieved
//type: 2 -- mobile confirmation received
void updateMobileTaskReport(struct task_pool * t_pool, int task_no, int type, int w){
   struct task_pool * p = t_pool;
   for(;p!=NULL; p=p->next){
      if(p->m_task_report->task_id == task_no){
         if(type == 1){
            p->m_task_report->task_status = 3;
            p->m_task_report->m_dep_time[p->m_task_report->mobilities] = MPI_Wtime();
			break;
         }else if(type == 2){
            p->m_task_report->task_status = 1;
            p->m_task_report->m_arr_time[p->m_task_report->mobilities] = MPI_Wtime();
            p->m_task_report->mobilities++;
            p->m_task_report->task_worker = w;
         }
      }
   }
}

struct worker_local_load * addWorkerLocalLoad(
                 struct worker_local_load * wlLoad, 
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
      while(pl->next != NULL)
			pl = pl->next;	
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
      if(!p->m_task->done && p->move_status != 1){
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
   struct worker_task * p = w_ts->next;
   float t_per = 0;
   float t_sec = 0;
   while(p != NULL){
      if(!p->m_task->done && p->move_status != 1){
         int i = p->m_task->counter;
         t_per = ((i * 100) / (float)p->m_task->counter_max);
         t_sec = MPI_Wtime() - p->w_task_start;
         p->w_l_load = addWorkerLocalLoad(p->w_l_load, t_per, t_sec, t_load_avg, t_est_load_avg, t_cpu_uti, t_run_proc);
      }
      p = p->next;
   }
}

float* getEstimatedExecutionTime(struct worker_load_task * w_l_t, int n, struct worker_task * p , int * dest_nps, struct estimated_cost * e_c, int cur_tasks){
   struct  worker_load w_local_loads = w_l_t->w_local_loads;
   int i = p->m_task->counter;
   //The work done before
   float Wd_before = p->m_task->m_work_start[p->m_task->moves-1];
   float Wd = ((i * 100) / (float)p->m_task->counter_max) - p->m_task->m_work_start[p->m_task->moves-1];
   //The time spent here to process the work done
   float Te = MPI_Wtime() - p->w_task_start;
   //the work left
   float Wl = 100 - Wd - Wd_before;	
   //Total machine power(P=speed*cores)
   float P = w_local_loads.w_cpu_speed;
   //Number of cores for this machine
   int cores = w_local_loads.w_cores;
   float CPU_UTI = w_local_loads.w_cpu_uti_2;
   int ESTIMATED_LOAD_AVG = w_local_loads.estimated_load;
   int RUNNING_PROCESSES = w_local_loads.w_running_procs;	
   float Rhn = getActualRelativePower(P, CPU_UTI, ESTIMATED_LOAD_AVG, RUNNING_PROCESSES, cores, *(dest_nps + rank -1), rank);		
   float Rhe = p->local_R;	
   float Th = (Wl * Rhe * Te)/(Wd * Rhn);		
   //n: number of workers
   //T: Array of all estimated execution times
   float *T = (float *)malloc(sizeof(float) * n);	
   struct worker_load * w_loads = w_l_t->w_loads;	
   int i_T = 0;
   float Tn;
   for(; i_T < n; i_T++){
      if(i_T == rank-1)
			T[i_T] = Th;
      else{
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
   for(i_T = 0; i_T < n; i_T++){
      if(i_T != rank-1 && (w_loads + i_T)->locked == 0){
         (e_c->other_ECs + i_w)->w_no = i_T;	
         (e_c->other_ECs + i_w)->move_cost = getPredictedMoveTime(p->m_task, w_l_t, i_T + 1);
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
            (e_c->other_ECs + i_w)->costs[i_t] = Tn;		
         }
         i_w++;			
      }		
   }
   return T;	
}

void printEstimationCost(struct estimated_cost * e_c, int cur_tasks, int other_worker_count){	
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
         for(i_t2=0; i_t2<cur_tasks; i_t2++)
            printf("%10.3f", (((e_c + i_t)->other_ECs + i_w)->costs[i_t2]));			
         printf(" [Move Cost: %.4f]", ((e_c + i_t)->other_ECs + i_w)->move_cost);
         printf("\n");
      }
      printf("\tNew Allocation: w: %d, EC: %.3f\n", (e_c + i_t)->to_w, (e_c + i_t)->to_EC);
   }
   printf("\n************************************************\n\n");
}
//find the best mappin fot tasks based on the estimated costs
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
      for(i=0; i<w_count; i++)
         for(i_new_w=0; i_new_w<cur_tasks; i_new_w++)
            ((e_c + i_t)->other_ECs+i)->costs[i_new_w] = ((e_c + i_t)->other_ECs+i)->costs[i_new_w] + ((e_c + i_t)->other_ECs+i)->move_cost;		
   }
   int trial = 0, improved = 1;
   do{
      //Find the slowest task;
      int s_t = -1;
      float s_ec = -1;
      //To select the first task which is the slowst
      for(i_t=0; i_t<cur_tasks; i_t++){
         //to garantee that the current task has spent over than 2 sec for having good estimation cost
         if((e_c + i_t)->spent_here > 2){				
            s_t = (e_c + i_t)->task_no;
            s_ec = (e_c + i_t)->to_EC;
            break;
         }			
      }		
      //break if there is no task whose spent time here noe exceed 2 sec
      if(s_t == -1) break;		
      for(i_t=0; i_t<cur_tasks; i_t++){			
         if((e_c + i_t)->spent_here > 2){
            if((e_c + i_t)->to_EC > s_ec){
               s_ec = (e_c + i_t)->to_EC;
               s_t = (e_c + i_t)->task_no;					
            }
         }
      }				
      //Find the best new location for the slowest task
      for(i_t=0; i_t<cur_tasks; i_t++){
         if((e_c + i_t)->task_no == s_t){
            int new_w = (e_c + i_t)->to_w;
            float new_EC = s_ec;
            for(i=0; i<w_count; i++){
               int w = ((e_c + i_t)->other_ECs + i)->w_no;
               if (((e_c + i_t)->other_ECs + i)->costs[task_mapping[w]] < new_EC){
                  new_EC = ((e_c + i_t)->other_ECs + i)->costs[task_mapping[w]];
                  new_w = w;
               }
            }				
            if(new_w != (e_c + i_t)->to_w){
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
      if(improved == 1){
         //Update the task allocation details depending on the new findings
         for(i_t=0; i_t<cur_tasks; i_t++){
            if((e_c + i_t)->spent_here > 2)
               if((e_c + i_t)->task_no != s_t){
                  if((e_c + i_t)->to_w == rank -1)
                     (e_c + i_t)->to_EC = (e_c + i_t)->cur_EC_after[cur_tasks - task_mapping[(e_c + i_t)->to_w] - 1];
                  else{						
                     int i_new_ec = 0;
                     for(i_new_ec=0; i_new_ec<w_count; i_new_ec++)
                        if(((e_c + i_t)->other_ECs + i_new_ec)->w_no == task_mapping[(e_c + i_t)->to_w])
                           (e_c + i_t)->to_EC = ((e_c + i_t)->other_ECs + i_new_ec)->costs[task_mapping[(e_c + i_t)->to_w] - 1];
                  }
               }
         }
      }
      trial++;		
   }while(improved == 1);
}

void *worker_estimator(void * arg){
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
   struct worker_move_report * w_m_report = w_l_t->move_report;	
   estimator_mid = MPI_Wtime();		
   int i_d = 0;
   p = w_l_t->w_tasks->next;
   cur_tasks = 0;
   while(p != NULL){
      if(!p->m_task->done && (p->move_status != 1))
         cur_tasks++;
      p = p->next;			
   }
   p = w_l_t->w_tasks->next;
   if(cur_tasks > 0){			
      //new wights after changing the task mapping
      int * dest_nps = (int*)malloc(sizeof(int)*(numprocs-1));			
      for(i_d=0; i_d<numprocs-1; i_d++)
         *(dest_nps + i_d) = 0;				
      //init the estimation report
      int i_t = 0, i_w = 0;
      int other_worker_count = 0;
      for(i_w=0; i_w<numprocs-1; i_w++){
         if(i_w != rank-1)
            if((w_l_t->w_loads + i_w)->locked == 0)
               other_worker_count++;
      }
      struct estimated_cost * e_c = (struct estimated_cost *)malloc(sizeof(struct estimated_cost) * cur_tasks); 
      for(i_t=0; i_t<cur_tasks; i_t++){
         (e_c + i_t)->cur_EC_after = (float*) malloc(sizeof(float) * (cur_tasks - 1));
         (e_c + i_t)->other_ECs = (struct other_estimated_costs*) malloc(sizeof(struct other_estimated_costs) * other_worker_count);
         for(i_w=0; i_w<other_worker_count; i_w++){
            ((e_c + i_t)->other_ECs + i_w)->costs = (float *) malloc(sizeof(float) * cur_tasks);
         }
      }
      i_t = 0;
      int report_done = 0;
      do{
         p = w_l_t->w_tasks->next;
         i_t = 0;
         while(p != NULL){					
            if(!p->m_task->done && (p->move_status != 1)){	
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
            }						
            p = p->next;				
         }				
         estimator_mid = MPI_Wtime();	
         findBestTaskMapping(e_c, cur_tasks, other_worker_count);					
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
         report_done = 1;
      }while(report_done == 0);
      //create move report
      p = w_l_t->w_tasks->next;
      while(p != NULL){
         if(!p->m_task->done && (p->move_status != 1))
            p->estimating_move = NULL;
            p = p->next;
         }
      }
      if(cur_tasks > 0)
         sendMoveReportToWorkers(w_l_t->move_report, w_l_t->w_loads);
   return NULL;
}

void *worker_status(void *arg){
   struct worker_load_task * w_l_t = (struct worker_load_task *)(arg);		
   //Get the thread number (Thread/Process ID)
   #ifdef SYS_gettid
   pid_t tid = syscall(SYS_gettid);
   #else
   #error "SYS_gettid unavailable on this system"
   #endif	
   w_l_t->status_tid = tid;		
   struct worker_load * local_load = (struct worker_load *)malloc(sizeof(struct worker_load));		
   float load_avg = getLoad(), prev_load_avg;	
   int w_cores = getNumberOfCores();	
   float cpu_freq = getCoresFreq();		
   local_load->w_rank = rank;
   strcpy(local_load->w_name, processor_name);
   local_load->m_id = 0;	
   local_load->current_tasks = 0;
   local_load->total_task = 0;
   local_load->status = 0;
   local_load->w_cores = w_cores;		
   local_load->w_cache_size = 0;	
   local_load->w_cpu_speed = cpu_freq;	
   local_load->w_load_avg_1 = load_avg;
   local_load->w_load_avg_2 = load_avg;
   local_load->locked = 0;
   local_load->estimated_load = 0;		
   local_load->w_cpu_uti_2 = 0;

   int state_worker = getProcessState(w_l_t->worker_tid, 0, 0);
   int state_estimator = getProcessState(w_l_t->worker_tid, 1, w_l_t->estimator_tid);
   local_load->w_running_procs = getRunningProc();
	
   local_load->w_running_procs = local_load->w_running_procs - state_worker - state_estimator - 1;
   if(local_load->w_running_procs < 0)
      local_load->w_running_procs = 0;

   long double * a = (long double *)malloc(sizeof(long double) * 5);
   long double * b = (long double *)malloc(sizeof(long double) * 5);

   getCPUValues(b);
   long double per = 0;
   int rp;	
   local_load->w_cpu_uti_1 = per;
   local_load->w_cpu_uti_2 = per;	
   w_l_t->w_local_loads = *local_load;	
   sendInitialWorkerLoad(local_load, 0);	
   prev_load_avg = load_avg;	
   recordLocalR(w_tasks, cpu_freq, w_cores, per, local_load->w_running_procs);	
   int load_info_size = sizeof(int) + sizeof(int) + sizeof(float) + sizeof(long double);		
   int int_size = sizeof(int);
   int float_size = sizeof(float);	
   int est_load = -1;	
   void * d = (void*)malloc(load_info_size);
   struct worker_task * p;
   int cur_tasks=0;
   long double per_1 = per;
   long double per_2 = per;
   long double per_3 = per;
   float POWER_BASE = cpu_freq / w_cores;	
   float POWER_LIMIT = 95.0;
   float cur_arp = -1;
   float arp_1 = -1;
   float arp_2 = -1;
   while(1){		
      prev_load_avg = load_avg;		
      //Obtaining the local load
      setLocalLoadInfo(a, b, &per, &rp, &load_avg);				
      if(w_l_t){
         w_l_t->w_local_loads.w_load_avg_1 = prev_load_avg;
         w_l_t->w_local_loads.w_load_avg_2 = load_avg;
         w_l_t->w_local_loads.estimated_load = est_load;
         w_l_t->w_local_loads.w_running_procs = rp;
         w_l_t->w_local_loads.w_cpu_uti_1 = w_l_t->w_loads[rank-1].w_cpu_uti_2;
         w_l_t->w_local_loads.w_cpu_uti_2 = per;
      }
	
      per_3 = per_2;		
      per_2 = per_1;
      per_1 = per;

      *((int*)d) = rp;
      *((int*)(d + int_size)) = est_load;
      *((float*)(d + 2 * int_size)) = load_avg;
      *((long double*)(d + 2 * int_size + float_size)) = per;	
			
      recordLocalR(w_tasks, cpu_freq, w_cores, per, rp);

      //Get the cuurent running tasks						
      p = w_l_t->w_tasks->next;
      cur_tasks = 0;
      while(p != NULL){
         if(!p->m_task->done && (p->move_status != 1))
            cur_tasks++;
         p = p->next;			
      }		
      if(cur_tasks > 0){
         cur_arp = getActualRelativePower(cpu_freq, per, est_load, rp, w_cores, 0, rank);
         if((cur_arp * 100 / POWER_BASE) < POWER_LIMIT){
            if(arp_1 == -1){
               arp_1 = cur_arp;
            }else{
               if(arp_2 == -1){
                  arp_2 = cur_arp;
               }else{							
                  //TRIGGER
                  obtainLatestLoad(0);
                  arp_1 = -1;
                  arp_2 = -1;
               }
            }
         }else{
            arp_1 = -1;
            arp_2 = -1;
         }
      }		
   }
}
///Send get initial load request
void notifyMaster(int t_id, int target_w, int master){
   int send_code = MOBILITY_NOTIFICATION_FROM_WORKER;	
   int * data = (int*)malloc(sizeof(int)*2);
   *data = t_id;
   *(data + 1) = target_w;
   MPI_Send(&send_code, 1, MPI_INT, master, send_code, MPI_COMM_WORLD);	
   MPI_Send(data, 2, MPI_INT, master, send_code, MPI_COMM_WORLD);
}

void * move_mobile_task(void * arg){
   struct worker_task * w_t = (struct worker_task *)arg;
   while(w_t->go_move == 0) usleep(1);						   
   w_t->m_task->m_end_time[w_t->m_task->moves-1] = MPI_Wtime();
   pthread_mutex_lock( &mutex_w_sending );	
   notifyMaster(w_t->task_id, w_t->go_to, 0);	
   moving_task = 1;
   sendMobileTask(w_t->m_task, w_t->go_to, W_TO_W);	
   moving_task = 0;
   pthread_mutex_unlock( &mutex_w_sending );	
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
   report[source-1].m_id++;
   report[source-1].status = 1;
   if(report[source-1].current_tasks == 0)
      report[source-1].status = 0;
   report[target-1].current_tasks++;
   report[target-1].m_id++;
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
   double ping_value = sendPing(w_report->w_name);
   if(ping_value != 0.0)
      w_report->net_times.init_net_time = ping_value;
}

void getInitialWorkerLoad(struct worker_load * report, int n){
   int i=0;
   for( i=1; i<n; i++)	
      sendInitialWorkerLoadRequest(i);
   int msg_code = -1;
   int w = 0;
   for(i=1; i<n; i++){		
      MPI_Recv(&msg_code, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);		
      w = status.MPI_SOURCE;
      if(msg_code == 1){
         MPI_Recv(&report[w-1], sizeof(struct worker_load), MPI_CHAR, w, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
         getInitNetworkLatency(&report[w-1]);
      }
   }	
   printWorkerLoadReport(report, n);
}

void setWorkerLoad(struct worker_load * report, int n, int source, int tag){
   int load_info_size = 0;
   MPI_Recv(&load_info_size, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
   void * load_info = (void *)malloc(load_info_size);
   MPI_Recv(load_info, load_info_size, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
   report[source-1].m_id++;
   report[source-1].w_load_avg_1 = report[source-1].w_load_avg_2;
   report[source-1].w_load_avg_2 = *((float *)(load_info + 2 * sizeof(int)));	
   report[source-1].estimated_load = *((int *)(load_info + sizeof(int)));
   report[source-1].w_cpu_uti_1 = report[source-1].w_cpu_uti_2;
   report[source-1].w_cpu_uti_2 = *((long double *)(load_info + 2 * sizeof(int) + sizeof(float)));	
   report[source-1].w_running_procs = *((int *)load_info);	
}

void circulateWorkerLoadReportInitial(struct worker_load * report, int n){
   int i = 1;
   int send_code = UPDATE_LOAD_REPORT_REQUEST;		
   int report_size = sizeof(struct worker_load)*(n-1);		
   while(*(masterReceiving + (i) - 1) == 1) usleep(1);
   *(masterSending + i - 1) = 1;
   double load_agent_t1;
   load_agent_t1 = MPI_Wtime();
   double new_net_lat = MPI_Wtime();	
   MPI_Ssend(&send_code, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
   new_net_lat = MPI_Wtime() - new_net_lat;	
   MPI_Send(&report_size, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
   MPI_Send(report, report_size, MPI_CHAR, i, send_code, MPI_COMM_WORLD);
   *(masterSending + i - 1) = 0;
}

void sendLatestLoad(struct worker_load * report, int n, int worker){
   int i = worker;
   int send_code = LOAD_INFO_FROM_MASTER;		
   int report_size = sizeof(struct worker_load)*(n-1);	
   while(*(masterReceiving + (i) - 1) == 1) usleep(1);
   *(masterSending + i - 1) = 1;
   MPI_Send(&send_code, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
   MPI_Send(&report_size, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
   MPI_Send(report, report_size, MPI_CHAR, i, send_code, MPI_COMM_WORLD);
   *(masterSending + i - 1) = 0;
}

void circulateWorkerLoadReport(struct worker_load * report, int to_worker, int n){
   int i = to_worker;
   int send_code = UPDATE_LOAD_REPORT_REQUEST;		
   int report_size = sizeof(struct worker_load)*(n-1);	
   pthread_mutex_lock( &mutex_load_circulating );
   MPI_Send(&send_code, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
   MPI_Send(&report_size, 1, MPI_INT, i, send_code, MPI_COMM_WORLD);
   MPI_Send(report, report_size, MPI_CHAR, i, send_code, MPI_COMM_WORLD);
   pthread_mutex_unlock( &mutex_load_circulating );
}

void recvWorkerLoadReport(struct worker_load * report, int source, int tag){
   int report_size = 0;
   MPI_Recv(&report_size, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
   MPI_Recv(report, report_size, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
}

int selectWorkerToCheckLatency(struct worker_load * report, int n){
   int w = -1;
   int i;
   for(i = 0; i < n; i++){	
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
      double circ_start = MPI_Wtime();
      circulateWorkerLoadReportInitial(w_load_report, numprocs);				
      if( i++ == 3)
         sendLatestLoad(w_load_report, numprocs, 1);	
      double circ_end = MPI_Wtime();
      printf("[%d]. circ_time: %lf\n", rank, circ_end - circ_start);
	}
}

///send confirmation to worker for accepting receiving the result from the worker
void sendRecvConfirmation(int w){
   int send_code = SENDING_CONFIRMATION_FROM_MASTER;
   MPI_Send(&send_code, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
}

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
}

void sendMultiMsgs(void *input, int dataLen, int limit, int proc, int tag, MPI_Datatype dataType){
   int msgCount = (dataLen/limit);
   if((dataLen % limit) != 0) msgCount++;
   int msgSize;
   int i=0;
   MPI_Ssend(&msgCount, 1, MPI_INT, proc, tag, MPI_COMM_WORLD);
   for(i = 0; i<msgCount; i++){
      if(dataLen < limit)
         msgSize = dataLen;
      else
         msgSize = limit;   
      MPI_Ssend(input + (i * limit), msgSize, dataType, proc, tag + i + 1, MPI_COMM_WORLD);		
      dataLen = dataLen - msgSize;
   }
}

void recvMultiMsgs(void * input, int dataLen, int limit, int source, MPI_Datatype dataType, int tag){
   int msgSize ;
   int msgCount;
   int i=0;
   MPI_Recv(&msgCount, 1 , MPI_INT, source, tag, MPI_COMM_WORLD, &status);
   for(i = 0; i<msgCount; i++){
      if(dataLen < limit)
         msgSize = dataLen;
      else
         msgSize = limit;
      MPI_Recv(input + (i * limit), msgSize, dataType, source, tag + i + 1, MPI_COMM_WORLD, &status);
      dataLen = dataLen - msgSize;
   }
}
//Send the shared data to a worker
void sendSharedData(void *shared_data, int data_len, int w){	
   if(data_len != 0){
      int send_code = SHARED_DATA_FROM_MASTER;
      MPI_Ssend(&send_code, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);
      MPI_Ssend(&data_len, 1, MPI_INT, w, send_code, MPI_COMM_WORLD);	
      sendMultiMsgs( shared_data, data_len, MSG_LIMIT, w, send_code, MPI_CHAR);
   }
}
//Send the shared data to all workers
void sendSharedDataToAll(void *shared_data, int shared_data_size, int shared_data_len, int n){	
   int i=0;
   for( i=1; i<n; i++)	
      sendSharedData(shared_data, shared_data_size*shared_data_len, i);
}

void initHWFarm(int argc, char ** argv){ 
   int provided;
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);	
   MPI_Get_processor_name(processor_name, &namelen);
   startTime = MPI_Wtime();
}

void finalizeHWFarm(){	
   MPI_Finalize(); 
}

void hwfarm(fp worker,int tasks,
           void *input, int inSize,int inLen, 
           void *shared_data, int shared_data_size, int shared_data_len, 
           void *output, int outSize, int outLen, hwfarm_state main_state,
           int mobility){

   MPI_Barrier(MPI_COMM_WORLD);
   //MASTER PROCESS
   if(rank == 0){	
      start_time = MPI_Wtime();
      int m_cores = getNumberOfCores();
      if(m_cores == 0)
         m_cores = 1;
      Master_FREQ = getCoresFreq() / m_cores;
      int w=0, w_msg_code = 0, mR = 0, dest_w = -1, w_i = 0;
      if(isFirstCall){
         w_load_report = (struct worker_load *)malloc(sizeof(struct worker_load)* (numprocs - 1));
         w_load_report_tmp = (struct worker_load *)malloc(sizeof(struct worker_load)* (numprocs - 1));      
         getInitialWorkerLoad(w_load_report, numprocs);			
      }
      struct task_pool * pool = 0;
      pool = create_task_pool(tasks, input, inLen, inSize, 
                              output, outLen, outSize, 
                              main_state.state_data, main_state.state_len, 
                              main_state.counter, main_state.max_counter);											
      masterReceiving = (int *)malloc((sizeof(int) * (numprocs - 1)));
      masterSending = (int *)malloc((sizeof(int) * (numprocs - 1)));
      for(mR = 0; mR < numprocs - 1; mR++){
         *(masterReceiving + mR) = 0;
         *(masterSending + mR) = 0;
      }
		
      struct mobile_task_report * m_t_r;		
      int processing_tasks = 0;	
		
      //Distributing tasks based on load on workers based on the allocation model
      int WORKERS_COUNT = numprocs - 1;
      int * tasksPerWorker = (int *)malloc(sizeof(int) * WORKERS_COUNT);
      int dis_tasks = tasks;
      for(w_i = 0; w_i < WORKERS_COUNT; w_i++)
         tasksPerWorker[w_i] = 0;
			
      int C=0;//Total number of cores for all workers
      for(w_i = 0; w_i < WORKERS_COUNT; w_i++)
         C += w_load_report[w_i].w_cores;
      if(C > 0 && dis_tasks > 0){				
         for(w_i = 0; w_i < WORKERS_COUNT; w_i++){
            int dis_task_i = ceil(w_load_report[w_i].w_cores * dis_tasks * 1.0 / C);
            tasksPerWorker[w_i] = dis_task_i;
            dis_tasks -= dis_task_i;
            C -= w_load_report[w_i].w_cores;
            if(dis_tasks <= 0) break;
         }
      }	
      ///Send the shared data to the workers
      sendSharedDataToAll(shared_data, shared_data_size, shared_data_len, numprocs);
      ///Send the tasks to the workers
      while((m_t_r = getReadyTask(pool)) != NULL){
         ///Get the numbers of tasks allocated to each worker
         getValidWorker(tasksPerWorker, WORKERS_COUNT, &dest_w);
         if(dest_w == -1)
            break;			
         ///Send the task to the selected worker 'dest_w'		
         sendMobileTaskM(m_t_r, dest_w);			
         ///Modify the status of the worker
         modifyWorkerLoadReport(dest_w, w_load_report, numprocs, 0);
         processing_tasks++;						
      }		
		
      if(isFirstCall){
         pthread_create(&w_load_report_th, NULL, workerLoadReportFun, w_load_report);
         pthread_create(&w_network_latency_th, NULL, networkLatencyFun, w_load_report);
         isFirstCall = 0;
      }
		
      do{
         ///Receive the msg code first; depending on this code, 
         ///the type of data will be detected
         recvMsgCode(&w,&w_msg_code);
         ///Load comes from a worker
         if(w_msg_code == INIT_LOAD_FROM_WORKER){
            setWorkerLoad(w_load_report, numprocs, w, w_msg_code);
         }else if(w_msg_code == LATEST_LOAD_REQUEST){
            sendLatestLoad(w_load_report, numprocs, w);
         }else if(w_msg_code == UPDATE_LOAD_REPORT_REQUEST){				
            recvWorkerLoadReport(w_load_report_tmp, w, w_msg_code);
            int i=0;
            for(i=0; i<numprocs-1; i++){
               w_load_report[i].m_id = w_load_report_tmp[i].m_id;
               w_load_report[i].w_load_avg_1 = w_load_report_tmp[i].w_load_avg_1;
               w_load_report[i].w_load_avg_2 = w_load_report_tmp[i].w_load_avg_2;
               w_load_report[i].estimated_load = w_load_report_tmp[i].estimated_load;
               w_load_report[i].w_running_procs = w_load_report_tmp[i].w_running_procs;
               w_load_report[i].w_cpu_uti_1 = w_load_report_tmp[i].w_cpu_uti_1;
               w_load_report[i].w_cpu_uti_2 = w_load_report_tmp[i].w_cpu_uti_2;	
               w_load_report[i].locked = w_load_report_tmp[i].locked;	
            }
								
            printWorkerLoadReport(w_load_report, numprocs);
         }else if(w_msg_code == RESULTS_FROM_WORKER){
            *(masterReceiving + w - 1) = 1;
            while(*(masterSending + (w) - 1) == 1) usleep(1);
            ///Send confirmation to the worker 'w' to process 				
            sendRecvConfirmation(w);
            ///modify the load status for the sender;w: worker who 
            ///finished executing the task
            modifyWorkerLoadReport(w, w_load_report, numprocs, 1);
            printWorkerLoadReport(w_load_report, numprocs);
            processing_tasks--;
            ///Receive the task results
            recvMobileTaskM(pool, w, w_msg_code);				
            ///Unlock the receiving from that worker 'w'
            *(masterReceiving + w - 1) = 0;
            ///If there are tasks in the task pool
            if((m_t_r = getReadyTask(pool)) != NULL){					
               dest_w = w;
               sendMobileTaskM(m_t_r, dest_w);					
               modifyWorkerLoadReport(dest_w, w_load_report, numprocs, 0);
               printWorkerLoadReport(w_load_report, numprocs);	
               processing_tasks++;
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
      terminateWorkers(numprocs);				
      taskOutput(pool, output, outLen, outSize);		
      free(pool);			
      end_time = MPI_Wtime();
      printf("Total Time: %.5f\n", end_time - start_time);
		
   }else{//WORKER PROCESS
      int w = 0, w_msg_code = 0;
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
      ///Pointer to the shared data which will be accebile amongst all tasks
      void *shared_data = NULL;	
      int w_shared_totalsize = 0;			
      do{			
         ///Receive the message code from the master or from the  
         ///source worker who wants to send the task to it.
         recvMsgCode(&w, &w_msg_code);			
         ///Run the worker laod agent
         if(w_msg_code == LOAD_REQUEST_FROM_MASTER){						
            pthread_create(&w_load_pth, NULL, worker_status, w_l_t);		
         }else if(w_msg_code == TERMINATE_THE_WORKER){
            printf("[%d]. TERMINATE_THE_WORKER.\n", rank);
            break;
         }else if(w_msg_code == LOAD_INFO_FROM_MASTER){
            recvWorkerLoadReport(w_l_t->w_loads, w, w_msg_code);				
            printWorkerLoadReport(w_l_t->w_loads, numprocs);				
            if(mobility == 1)
               pthread_create(&w_estimator_pth, NULL, worker_estimator, w_l_t);
         }else if(w_msg_code == SENDING_CONFIRMATION_FROM_MASTER){
            worker_sending = 1;
         }else if(w_msg_code == MOBILITY_ACCEPTANCE_FROM_WORKER){				
            int num_tasks = 0;
            MPI_Recv(&num_tasks, 1, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);				
            if(num_tasks > 0){
               if(w_l_t->move_report != NULL){						
                  int cur_num_of_tasks = (w_l_t->move_report + w - 1)->num_of_tasks;
                  int * cur_list_of_tasks = (w_l_t->move_report + w - 1)->list_of_tasks;
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
                                 if(wT->moving_pth == 0)
                                    pthread_create(&wT->moving_pth, NULL, move_mobile_task, wT);
                                 break;
                              }
                           }
                        }
                     }
               }					
            }else{
               printWorkerLoadReport(w_l_t->w_loads, numprocs);
            }
         }else if(w_msg_code == UPDATE_LOAD_REPORT_REQUEST){
            recvWorkerLoadReport(w_l_t->w_loads, w, w_msg_code);
            w_l_t->w_loads[rank-1].m_id++;
            w_l_t->w_loads[rank-1].w_load_avg_1 = w_l_t->w_local_loads.w_load_avg_1;
            w_l_t->w_loads[rank-1].w_load_avg_2 = w_l_t->w_local_loads.w_load_avg_2;
            w_l_t->w_loads[rank-1].estimated_load = w_l_t->w_local_loads.estimated_load;
            w_l_t->w_loads[rank-1].w_running_procs = w_l_t->w_local_loads.w_running_procs;
            w_l_t->w_loads[rank-1].w_cpu_uti_1 = w_l_t->w_local_loads.w_cpu_uti_1;
            w_l_t->w_loads[rank-1].w_cpu_uti_2 = w_l_t->w_local_loads.w_cpu_uti_2;
            w_l_t->w_loads[rank-1].locked = w_l_t->w_local_loads.locked;				
            if(rank+1 < numprocs)
               circulateWorkerLoadReport(workers_load_report, rank+1, numprocs);
            else	
               circulateWorkerLoadReport(workers_load_report, 0, numprocs);
         }else if(w_msg_code == TASK_FROM_MASTER){
            double task_net_t = MPI_Wtime();
            struct worker_task * w_task = (struct worker_task*)malloc(sizeof(struct worker_task));
            w_task->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));
            w_task->m_task->move_stats.start_move_time = MPI_Wtime();				
				
            recvMobileTask(w_task->m_task, w, M_TO_W, w_msg_code);
				
            w_task->m_task->move_stats.net_time = task_net_t - w_task->m_task->move_stats.start_move_time;				

            w_task->m_task->task_fun = worker;
            //Set Shared values				
            if(shared_data != NULL){
               w_task->m_task->shared_data = shared_data;
               w_task->m_task->shared_data_length = w_shared_totalsize/shared_data_size;
               w_task->m_task->shared_data_item_size = shared_data_size;	
            }
								
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
            pthread_create(&w_task->task_pth, NULL, workerMobileTask, w_task->m_task);					
         }else if(w_msg_code == MOBILITY_REQUEST_FROM_WORKER){
            int*msg_numTasks_load_no = (int*)malloc(sizeof(int)*2);
            MPI_Recv(msg_numTasks_load_no, 2, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);
            printWorkerLoadReport(w_l_t->w_loads, numprocs);
            int num_task = *msg_numTasks_load_no;
            if(w_l_t->hold.on_hold == 0){
               w_l_t->hold.on_hold = 1;
               w_l_t->hold.holded_on = num_task;
               w_l_t->hold.holded_from = w;
               w_l_t->hold.hold_time = MPI_Wtime();
               w_l_t->w_local_loads.locked = 1;					
               sendMobileConfirmationToWorker(w, num_task);
            }else{
               sleep(1);
               sendMobileConfirmationToWorker(w, 0);
            }
         }else if(w_msg_code == MOBILITY_CONFIRMATION_FROM_WORKER){
            int t_id = -1;
            MPI_Recv(&t_id, 1, MPI_INT, w, w_msg_code, MPI_COMM_WORLD, &status);				
            w_l_t->w_loads[rank-1].m_id++;
            struct worker_task * wT = w_tasks->next;
            for(;wT!=0;wT=wT->next)
               if(t_id == wT->task_id)
                  if(wT->move == 1 && wT->go_move == 1 && wT->move_status == 0){
                     wT->move_status = 1;							
                     break;
                  }
         }else if(w_msg_code == TASK_FROM_WORKER){
            struct worker_task * w_task = (struct worker_task*)malloc(sizeof(struct worker_task));
            w_task->m_task = (struct mobile_task *)malloc(sizeof(struct mobile_task));				
            recvMobileTask(w_task->m_task, w, W_TO_W, w_msg_code);				
            w_task->m_task->task_fun = worker;
            w_task->m_task->shared_data = shared_data;									
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
            //Sending a confirmation to the source worker...
            sendMobileConfirmation(w_task->task_id, w, 0);
            //Update the load no on local
            w_l_t->w_loads[rank-1].m_id++;
            w_l_t->hold.holded_on--;
            if(w_l_t->hold.holded_on == 0){
               w_l_t->hold.on_hold = 0;
               w_l_t->hold.holded_on = 0;
               w_l_t->hold.hold_time = 0;
               w_l_t->w_local_loads.locked = 0;
            }
            pthread_create(&w_task->task_pth, NULL, workerMobileTask, w_task->m_task);					
         }else if(w_msg_code == SHARED_DATA_FROM_MASTER){	
            shared_data = recvSharedData(shared_data, &w_shared_totalsize, w, w_msg_code);
         }
      }while(w_msg_code != TERMINATE_THE_WORKER);
   }	
   MPI_Barrier(MPI_COMM_WORLD);	
}
