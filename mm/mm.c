#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//MPI & PThread
#include <mpi.h>
#include <pthread.h>

#include "../hwfarm/hwfarm.h"


///This function is written by the user countains :
///inputData : the data should be processed by each worker(one task)
///inputLen : the length of data in one task
///result : the data resulting from processing the input data(result of one task)
///outputLen : the length of the result
///process the data(task)
/*
void doProcessing(  void *inputData,int inputLen,
					void *sharedData,int sharedDataLen,
					void *result,int outputLen,
					void* state,int stateSize, 					
					chFM checkForMobility){
*/	
void doProcessing(  hwfarm_task_data* t_data, chFM checkForMobility){
	
	void * mat_a = t_data->input_data;
	void * mat_b = t_data->shared_data;	
	void * result = t_data->output_data;

	//int *i = ((int*)t_data->state_data);
	int *i = (t_data->counter);
	int *i_max = (t_data->counter_max);
	int *j = ((int*)t_data->state_data)+1;
	int *k = ((int*)t_data->state_data)+2;
	int *c = ((int*)t_data->state_data)+3;	
	int *rows_a = ((int*)t_data->state_data)+4;
	int *cols_b = ((int*)t_data->state_data)+5;
	int *mat_s = ((int*)t_data->state_data)+6;
	
	//printf("W[%d]: input_data: %lx\n", rank, t_data->input_data);
	//printf("W[%d]: shared_data: %lx\n", rank, t_data->shared_data);
	//printf("W[%d]: output_data: %lx\n", rank, t_data->output_data);
	//printf("W[%d]: counter: %lx\n", rank, t_data->counter);
	//printf("W[%d]: counter_max: %lx\n", rank, t_data->counter_max);
	
	//double mpi_time = MPI_Wtime();
	
	if(0){
		int iii=0;
		for(iii=0;iii<7;iii++)
			printf("%d\n", *(((int*)t_data->state_data) + iii));
			
		//print A matrix
		printf("Printing A @ 'worker%d': \n-------------------------------------\n", rank);
		int ii=0,jj=0,kk=0;
		for(ii=0; ii<*rows_a; ii++){
			for(jj=0; jj<*mat_s; jj++){
				printf("%6.0f ", *(((double*)t_data->input_data) + (kk++)));
			}
			printf("\n");
		}

		//printf("Printing SHARED [%lx] @ WORKER %d: \n-------------------------------------\n", t_data->shared_data, rank);
		printf("Printing SHARED @ WORKER %d: \n-------------------------------------\n", rank);
		kk=0;
		for(ii=0; ii<*mat_s; ii++){
			for(jj=0; jj<*mat_s; jj++){
				printf("%6.0f ", *(((double*)t_data->shared_data) + (kk++)));
			}
			printf("\n");
		}		

	}

	//while((*i) < (*rows_a)){
	//printf("Start Counter @ WORKER %d: (i: %d, max: %d)\n-------------------------------------\n", rank, *i, *i_max);
	while((*i) < (*i_max)){
	//while(t_data->counter < t_data->counter_max){
		while((*j) < (*cols_b)){
			*(((double*)result) + *c) = 0;
			double mat_res = 0;
			while((*k) < (*mat_s)){
				int shift_a = ((*i) * (*mat_s));
				int shift_b = ((*j) * (*mat_s));
				shift_a = shift_a + *k;
				shift_b = shift_b + *k;
				double mat_a_item = *(((double*)mat_a) + shift_a);
				double mat_b_item = *(((double*)mat_b) + shift_b);				
				mat_res = mat_res +  (mat_a_item * mat_b_item);
				//*(((double*)result) + *c) = mat_res;
				//*(((double*)result) + *c) = *(((double*)result) + *c) + (*(((double*)mat_a) + (*k + ((*i) * (*mat_s))))) * (*(((double*)mat_b) + (*k + ((*j) * (*mat_s)))));
				(*k)++;
			}
			*(((double*)result) + *c) = mat_res;
			(*c)++;
			(*j)++;
			(*k) = 0;
		}
		(*i)++;
		(*j) = 0;
		(*k) = 0;
		
		/*
		if((*i % 100) == 0){
			
			char *res4 = (char*)malloc(sizeof(char)*200);
	
			char *command = (char*)malloc(sizeof(char)*200);
			
			sprintf(command, "cat /proc/%u/task/%u/stat", getpid(), tid);
			
			systemCall(command, res4);
			printf("[worker: %d] - {%u} P-ID: \n%s\n", rank, tid, res4);
			
		}
		*/
		
		//printf("LOOP Counter @ WORKER %d: (i: %d, max: %d)\n", rank, *i, *i_max);
		//if((*i % 10) == 0)
		//	printf("Counter @ WORKER %d: (i: %d, max: %d, %.3f)\n-------------------------------------\n", rank, *t_data->counter, *t_data->counter_max, MPI_Wtime() - mpi_time);
		
		checkForMobility();
	}
	
	if(0){
		printf("Printing RESULT @ WORKER %d: \n-------------------------------------\n", rank);
		int ii=0,jj=0,kk=0;
		for(ii=0; ii<*rows_a; ii++){
			for(jj=0; jj<*mat_s; jj++){
				printf("%.0f ", *(((double*)result) + (kk++)));
			}
			printf("\n");
		}
	}
	
}


void* initSharedData(void* shared_data_in, int argc, void ** args){
	int mat_size = *((int*)args[0]);
	
	double* shared_data = (double*)malloc(sizeof(double)*(mat_size*mat_size));

	int k = 0, i=0, j=0;
	
	//printf("mat_size: %d\n", mat_size);
	
	for (i = 0; i < mat_size; i++) {
		for (j = 0; j < mat_size; j++) {
			shared_data[k++] = i + j + 2;
		}
	}
	
	return shared_data;
}

void* initInputData(void* input_data_in, int argc, void ** args){
	int mat_size = *((int*)args[0]);
	
	double* input_data = (double*)malloc(sizeof(double)*(mat_size*mat_size));

	int k = 0, i=0, j=0;
	
	//printf("mat_size: %d\n", mat_size);
	
	for (i = 0; i < mat_size; i++) {
		for (j = 0; j < mat_size; j++) {
			input_data[k++] = i + j + 1;
		}
	}
	
	return input_data;
}

void* initStateData(void* par_data_in, int argc, void ** args){
	int mat_size = *((int*)args[0]);
	int chunk = *((int*)args[1]);
	
	int* state_data = (int*)malloc(sizeof(double)*(7));
	
	state_data[0] = 0; 
	state_data[1] = 0;
	state_data[2] = 0; 
	state_data[3] = 0;	
	state_data[4] = chunk;
	state_data[5] = mat_size;
	state_data[6] = mat_size;

	return state_data;
}

void* initOutputData(void* output_data_in, int argc, void ** args){
	int mat_size = *((int*)args[0]);
	
	double* output_data = (double*)malloc(sizeof(double)*(mat_size*mat_size));
	
	return output_data;
}
 

void printArrayA(double * arr, int size, char * name){
	printf("----------------%s-------------------\n", name);
	int k=0, j, i;
	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			printf("%6.0f",arr[k++]);
		}
		printf("\n");
	}
}

void printArrays(double * mat_a_1d, double * mat_b_1d, double * mat_c_1d, int mat_size){
	printArrayA(mat_a_1d, mat_size, "A");
	printArrayA(mat_b_1d, mat_size, "B");
	printArrayA(mat_c_1d, mat_size, "C");
	
	printf("------------------------------------\n");
}

//*********************************************************
//---------------------------------------------------------
//mpicc multicore.c -o mat -lm -lpthread
//mpirun -n 17 ./mat 10 7000 700
//mpirun -n 4 ./mat 4 4200 350 1

//mpirun -n 4 ./mat 4 4200 350 1 > runs/out_t_12_l_100_m_1_3
//---------------------------------------------------------
//*********************************************************
int main(argc,argv)
int argc;
char **argv;
{ 	
	
	///Setting user data
		if(argc!=4){  
			printf("mpirun ... <binary> <matrix-total> <matrix-chunk> <mobility>\n");
			exit(0);
		}  
		
		initHWFarm(argc,argv);
	
		printf("Worker/Master [%d] In Machine [%s]\n",rank,processor_name);		
	
		//int number_of_worker = atoi(argv[1]); 
		//problem_size: the total number of rows 
		int problem_size = atoi(argv[1]);
		//chunk: number of rows in one task
		int chunk = atoi(argv[2]);
		
		int tasks = problem_size / chunk;	
		
		//printf("workerNumber: %d, mat_size: %d, chunk: %d, tasks: %d\n",workerNumber, mat_size,chunk,tasks);
		
		double * input_data = NULL;	
		double * shared_data = NULL;	
		double * output_data = NULL;
		//int * state_data = NULL;
		
		//initial arguments
		void ** args = (void**)malloc(sizeof(int*) * 2);	
		args[0] = (void*)malloc(sizeof(int));
		args[1] = (void*)malloc(sizeof(int));
		*((int*)args[0]) = problem_size;
		*((int*)args[1]) = chunk;	
		int mobility = atoi(argv[3]);
		
		//input
		//local
		int taskDataSize = sizeof(double);
		int inputDataSize = chunk * problem_size;
		//shared
		int shared_data_size = sizeof(double);
		int shared_data_len = problem_size * problem_size;	
		
		//output
		int resultDataSize = sizeof(double);
		int outputDataSize = chunk * problem_size;			
		
		hwfarm_state main_state;
		main_state.counter = 0;
		main_state.max_counter = chunk;
		main_state.state_data = NULL;
		main_state.state_len = 0;

		if(rank == 0)
		{
			
			printf("MASTER START: %.6f\n",MPI_Wtime());		
					
			//prepare data to be sent to the workers			
			
			//Input data		
			input_data = (double*)initInputData(input_data, 1, args);	
			/*
			printf("Printing A @ Master: \n-------------------------------------\n");
			int ii=0,jj=0,kk=0;
			for(ii=0; ii<problem_size; ii++){
				for(jj=0; jj<problem_size; jj++){
					printf("%.0f ", *(((double*)input_data) + (kk++)));
				}
				printf("\n");
			}
			*/
			//Shared data	
			shared_data = (double*)initSharedData(NULL, 1, args);
			/*
			printf("Printing SHARED @ Master: \n-------------------------------------\n");
			//int ii=0,jj=0,kk=0;
			kk=0;
			for(ii=0; ii<problem_size; ii++){
				for(jj=0; jj<problem_size; jj++){
					printf("%.0f ", *(((double*)shared_data) + (kk++)));
				}
				printf("\n");
			}			
				*/
			//State
			main_state.state_data = (int*)initStateData(main_state.state_data, 2, args);
			//par
			main_state.state_len = 7 * sizeof(int);
			//main_counter.counter = 0;
			//main_counter.total_items = problem_size;
			/*
			for(ii=0;ii<7;ii++)
				printf("%d\n", *(((int*)state_data) + ii));
			*/
			//Output Data		
			output_data = (double*)initOutputData(output_data, 1, args);		

			//char *res3 = (char*)malloc(sizeof(char) * 10000);
			//printf("Start...\n");
			//systemCall("top -H -n 1",res3);
			//res3 = excel("hostname");
			//printf("[%s]\n\n", res3);
			
			//start_time = MPI_Wtime();
			//double mat_a[mat_size][mat_size];
			//double mat_b[mat_size][mat_size];
			//double mat_c[mat_size][mat_size];
			
			//int i,j,k=0;
			/*
			for (i = 0; i < mat_size; i++) {
				for (j = 0; j < mat_size; j++) {
					mat_a[i][j]= i + j + 1;
				}
			}
			
			mat_a_1d = (double*)malloc(sizeof(double)*(mat_size*mat_size));
			
			//convert 2D to 1D
			for (i = 0; i < mat_size; i++) {
				for (j = 0; j < mat_size; j++) {
					mat_a_1d[k++] = i + j + 1;
				}
			}*/		
			
			//printArrays(input_data, shared_data, output_data, mat_size);
			
			printf("TASKS: %d, PROBLEM SIZE: %d\n",tasks, problem_size);
		}
	
	hwfarm( doProcessing, tasks,
		    input_data, taskDataSize, inputDataSize, 
		    shared_data, shared_data_size, shared_data_len, 
		    output_data, resultDataSize, outputDataSize, 
		    main_state, mobility);
		    
	//findNetLatency(numprocs);
	
	if(rank == 0)
	{
		//printArrays(input_data, shared_data, output_data, problem_size);
	}
	///initSharedData(argc,argv);
	
	finalizeHWFarm();
	
	return 1;  
}
